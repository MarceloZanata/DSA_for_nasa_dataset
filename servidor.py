#!/usr/bin/env python3
"""
Servidor local que liga a página web ao programa em C (./trabalho).

Uso:   python3 servidor.py            (no Windows: python servidor.py)
Abre:  http://127.0.0.1:8000

Toda a busca é feita pelo SEU programa em C: o servidor só monta o arquivo de
consulta, chama o executável, lê o CSV de saída e as linhas de tempo
(LOAD_MS, SEARCH_MS, ...) e devolve tudo para a página em JSON.
Só usa a biblioteca padrão do Python (nada para instalar).
"""
import csv
import glob
import io
import json
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import threading
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

RAIZ = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(RAIZ, "web")
CSV = os.path.join(RAIZ, "dados", "mortes.csv")
EXE = os.path.join(RAIZ, "trabalho.exe" if os.name == "nt" else "trabalho")
ENC = "cp1252"  # o mortes.csv está em ISO-8859-1/Windows-1252; o C trabalha em bytes

ESTRUTURAS = [
    ("pilha", "Pilha", "Último a entrar, primeiro a sair (LIFO)."),
    ("fila", "Fila", "Primeiro a entrar, primeiro a sair (FIFO)."),
    ("lista_seq", "Lista sequencial", "Vetor que cresce quando enche."),
    ("lista_enc", "Lista encadeada", "Nós ligados por ponteiros."),
    ("lista_cabeca", "Lista com nó cabeça", "Encadeada, com um nó fixo no começo."),
    ("lista_ord", "Lista ordenada", "Insere já na posição certa, usando ORDER."),
    ("lista_gen", "Lista generalizada", "Organiza em grupos e subgrupos (GROUP)."),
    ("lista_cruz", "Lista cruzada", "Matriz esparsa de dois eixos (CROSS)."),
]
IDS = {e[0] for e in ESTRUTURAS}
COLUNAS_ORDEM = {"ANO", "MES", "QT_VITIMAS", "MUNICIPIO", "REGIAO", "GR_NATUREZA", "similarity"}
COLUNAS_GRUPO = {"REGIAO", "ANO", "MES", "GR_NATUREZA"}
COLUNAS_SIM = {"QT_VITIMAS", "MES", "ANO"}


# ---------------------------------------------------------------- compilação
def compilar():
    """Compila o projeto se o executável ainda não existe."""
    if os.path.exists(EXE):
        return True
    fontes = sorted(glob.glob(os.path.join(RAIZ, "c_project", "src", "*.c")))
    cc = shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
    if not cc:
        print("ERRO: nenhum compilador C (gcc) encontrado no PATH.")
        return False
    cmd = [cc, "-std=c99", "-O2", "-Ic_project/include", "-D_DEFAULT_SOURCE",
           "-D_POSIX_C_SOURCE=200809L", "-o", EXE, *fontes, "-lm"]
    print("Compilando:", " ".join(os.path.relpath(c, RAIZ) if os.path.isabs(c) else c for c in cmd[:6]), "...")
    r = subprocess.run(cmd, cwd=RAIZ, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr)
        return False
    return True


# ---------------------------------------------------------------- metadados
def carregar_meta():
    with open(CSV, encoding=ENC, newline="") as f:
        linhas = list(csv.DictReader(f))
    uniq = lambda k: sorted({l[k] for l in linhas})
    return {
        "total_linhas": len(linhas),
        "regioes": uniq("REGIAO"),
        "anos": sorted(int(a) for a in uniq("ANO")),
        "naturezas": uniq("GR_NATUREZA"),
        "municipios": uniq("MUNICIPIO"),
        "estruturas": [{"id": i, "nome": n, "desc": d} for i, n, d in ESTRUTURAS],
    }


META = None


# ---------------------------------------------------------------- consulta
def texto_seguro(s):
    if not isinstance(s, str) or any(c in s for c in '"\n\r\0|#') or len(s) > 120:
        raise ValueError("valor inválido: %r" % (s,))
    return s


def montar_consulta(c):
    """Converte os filtros da página no formato de consulta do seu programa."""
    L = []
    for col, op, val in (
        ("ANO", ">=", c.get("ano_de")), ("ANO", "<=", c.get("ano_ate")),
        ("MES", ">=", c.get("mes_de")), ("MES", "<=", c.get("mes_ate")),
        ("QT_VITIMAS", ">=", c.get("min_vitimas")), ("QT_VITIMAS", "<=", c.get("max_vitimas")),
    ):
        if val not in (None, ""):
            L.append("FILTER %s %s %d" % (col, op, int(val)))
    for r in c.get("regioes_excluidas", []):
        L.append('FILTER REGIAO != "%s"' % texto_seguro(r))
    for n in c.get("naturezas_excluidas", []):
        L.append('FILTER GR_NATUREZA != "%s"' % texto_seguro(n))
    if c.get("municipio"):
        L.append('FILTER MUNICIPIO == "%s"' % texto_seguro(c["municipio"]))

    sim = c.get("sim")
    if sim and sim.get("ativo"):
        col = sim.get("coluna")
        if col not in COLUNAS_SIM:
            raise ValueError("coluna de similaridade inválida")
        L.append("SIM %s %g %g %g" % (col, float(sim["alvo"]), max(float(sim["tolerancia"]), 0.0001),
                                       max(float(sim.get("peso", 1)), 0)))
        L.append("ACCEPT %g" % float(sim.get("limiar", 0)))

    ordem = c.get("ordem_coluna")
    if ordem:
        if ordem not in COLUNAS_ORDEM:
            raise ValueError("coluna de ordenação inválida")
        L.append("ORDER %s %s" % (ordem, "ASC" if c.get("ordem_dir") == "ASC" else "DESC"))

    g1, g2 = c.get("grupo1"), c.get("grupo2")
    if g1 in COLUNAS_GRUPO and g2 in COLUNAS_GRUPO:
        L.append("GROUP %s %s" % (g1, g2))
        L.append("CROSS %s %s" % (g1, g2))
    return "\n".join(L) + "\n"


def executar(cfg, base, resultado, top_k, tmp):
    """Roda ./trabalho uma vez e devolve (estatísticas, linhas, comando)."""
    if base not in IDS or resultado not in IDS:
        raise ValueError("estrutura desconhecida")
    consulta = os.path.join(tmp, "consulta.txt")
    saida = os.path.join(tmp, "saida.csv")
    with open(consulta, "w", encoding=ENC, newline="\n") as f:
        f.write(montar_consulta(cfg))
    limit = max(0, min(int(cfg.get("limit") or 0), 10**7))
    cmd = [EXE, "--reader", "mortes", "--input", CSV, "--base", base, "--result", resultado,
           "--query", consulta, "--output", saida, "--top-k", str(max(0, int(top_k))),
           "--limit", str(limit)]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    if r.returncode != 0:
        raise RuntimeError(r.stderr.strip() or "o programa em C falhou")
    est = {}
    for linha in r.stdout.splitlines():
        if "=" in linha:
            k, v = linha.split("=", 1)
            try:
                est[k.strip()] = float(v) if "." in v else int(v)
            except ValueError:
                est[k.strip()] = v.strip()
    with open(saida, encoding=ENC, newline="") as f:
        linhas = list(csv.reader(f))
    cab, dados = linhas[0], linhas[1:]
    mostra = " ".join(["./trabalho", "--reader mortes", "--input dados/mortes.csv",
                       "--base", base, "--result", resultado, "--query consulta.txt",
                       "--output saida.csv", "--top-k", str(top_k), "--limit", str(limit)])
    return est, cab, dados, mostra


def api_run(cfg):
    base, res = cfg.get("base", "lista_enc"), cfg.get("resultado", "lista_ord")
    top_k = int(cfg.get("top_k") or 0)
    with tempfile.TemporaryDirectory() as tmp:
        est, cab, linhas_top, cmd = executar(cfg, base, res, top_k, tmp)
        # Segunda execução (sem top-k) só para alimentar os gráficos com TODOS os encontrados.
        if top_k == 0:
            todos = linhas_top
        else:
            _, _, todos, _ = executar(cfg, base, res, 0, tmp)
    return {
        "comando": cmd,
        "consulta": montar_consulta(cfg),
        "stats": est,
        "colunas": cab,
        "top": linhas_top,
        "todos": todos,
    }


def api_benchmark(cfg):
    """Mede cada estrutura como base (resultado fixo) e como resultado (base fixa). 3 repetições, mediana."""
    REP = 3
    chaves = ("LOAD_MS", "SEARCH_MS", "RESULT_INSERT_MS", "WRITE_MS")
    out = {"como_base": [], "como_resultado": [], "fixa_resultado": "fila", "fixa_base": "lista_enc"}
    top_k = int(cfg.get("top_k") or 0)
    with tempfile.TemporaryDirectory() as tmp:
        for modo, lista, fixa in (("como_base", "base", "fila"), ("como_resultado", "resultado", "lista_enc")):
            for sid, nome, _ in ESTRUTURAS:
                amostras = {k: [] for k in chaves}
                for _ in range(REP):
                    b, r = (sid, fixa) if lista == "base" else (fixa, sid)
                    est, *_ = executar(cfg, b, r, top_k, tmp)
                    for k in chaves:
                        amostras[k].append(float(est.get(k, 0)))
                med = {k: round(statistics.median(v), 3) for k, v in amostras.items()}
                med["id"], med["nome"] = sid, nome
                out[modo].append(med)
    return out


# ---------------------------------------------------------------- HTTP
class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=WEB, **k)

    def log_message(self, fmt, *args):
        if "/api/" in (args[0] if args else ""):
            sys.stderr.write("  %s\n" % (fmt % args))

    def _json(self, obj, status=200):
        corpo = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(corpo)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(corpo)

    def do_GET(self):
        if self.path == "/api/meta":
            return self._json(META)
        return super().do_GET()

    def do_POST(self):
        try:
            n = int(self.headers.get("Content-Length", 0))
            cfg = json.loads(self.rfile.read(n) or b"{}")
            if self.path == "/api/run":
                return self._json(api_run(cfg))
            if self.path == "/api/benchmark":
                return self._json(api_benchmark(cfg))
            self._json({"erro": "rota inexistente"}, 404)
        except (ValueError, TypeError, KeyError) as e:
            self._json({"erro": "Pedido inválido: %s" % e}, 400)
        except Exception as e:  # noqa
            self._json({"erro": str(e)}, 500)


def main():
    global META
    if not os.path.exists(CSV):
        sys.exit("Não achei dados/mortes.csv. Rode o servidor dentro da pasta do projeto.")
    if not compilar():
        sys.exit("Não foi possível compilar o programa em C.")
    META = carregar_meta()
    porta = int(os.environ.get("PORTA", "8000"))
    srv = ThreadingHTTPServer(("127.0.0.1", porta), Handler)
    url = "http://127.0.0.1:%d" % porta
    print("Servidor no ar em", url, " (Ctrl+C para parar)")
    if os.environ.get("NAO_ABRIR") != "1":
        threading.Timer(0.6, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nEncerrado.")


if __name__ == "__main__":
    main()
