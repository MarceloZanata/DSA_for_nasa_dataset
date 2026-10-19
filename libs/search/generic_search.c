#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include "generic_item.h"
#include "generic_search.h"

#define SIM_EPS 1e-9   /* tolerancia para comparar similaridade com o limiar */

typedef enum { OP_EQ, OP_NE, OP_LE, OP_GE, OP_LT, OP_GT } CmpOp;

typedef struct {
    char *col;
    CmpOp op;
    char *target;       /* texto original do alvo (comparacao de strings) */
    double num;         /* alvo interpretado como numero */
    bool is_num;
    bool b;             /* alvo interpretado como booleano */
    bool is_bool;
} FilterRule;

typedef struct {
    char *col;
    double target;
    double tolerance;   /* > 0 */
    double weight;      /* >= 0 */
} SimRule;

struct query_ {
    FilterRule *filters;
    int filter_count, filter_cap;
    SimRule *sims;
    int sim_count, sim_cap;
    double accept_threshold;
    bool has_accept;
    char *order_key;
    bool order_desc;
    char *group1, *group2;
    char *cross1, *cross2;
};

/* ------------------------------------------------------------------ */
/* Utilitarios                                                         */
/* ------------------------------------------------------------------ */

static void *xrealloc(void *ptr, size_t n) {
    void *p = realloc(ptr, n ? n : 1);
    if (!p) { fprintf(stderr, "erro: sem memoria\n"); exit(EXIT_FAILURE); }
    return p;
}

static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *) xrealloc(NULL, n);
    memcpy(p, s, n);
    return p;
}

static void set_str(char **dst, const char *s) {
    char *copy = s ? dup_str(s) : NULL;
    free(*dst);
    *dst = copy;
}

static bool equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static bool parse_double(const char *s, double *out) {
    if (!s || *s == '\0') return false;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0' || !isfinite(v)) return false;
    *out = v;
    return true;
}

/* ------------------------------------------------------------------ */
/* Tokenizacao flexivel ('|' ou espacos; aspas duplas protegem espacos) */
/* ------------------------------------------------------------------ */

static bool is_sep(char c) { return c == '|' || isspace((unsigned char)c); }

/* Proximo token (altera a linha in-place). NULL se acabou. */
static char *next_token(char **cursor) {
    char *p = *cursor;
    while (*p && is_sep(*p)) p++;
    if (*p == '\0') { *cursor = p; return NULL; }

    char *start;
    if (*p == '"') {
        start = ++p;
        while (*p && *p != '"') p++;
    } else {
        start = p;
        while (*p && !is_sep(*p)) p++;
    }
    if (*p) *p++ = '\0';
    *cursor = p;
    return start;
}

/* Resto da linha sem separadores nas pontas; remove aspas externas. */
static char *rest_of_line(char **cursor) {
    char *p = *cursor;
    while (*p && is_sep(*p)) p++;
    size_t n = strlen(p);
    while (n > 0 && is_sep(p[n - 1])) n--;
    p[n] = '\0';
    if (n >= 2 && p[0] == '"' && p[n - 1] == '"') { p[n - 1] = '\0'; p++; }
    *cursor = p + strlen(p);
    return p;
}

static bool parse_op(const char *s, CmpOp *op) {
    if (strcmp(s, "==") == 0) *op = OP_EQ;
    else if (strcmp(s, "!=") == 0) *op = OP_NE;
    else if (strcmp(s, "<=") == 0) *op = OP_LE;
    else if (strcmp(s, ">=") == 0) *op = OP_GE;
    else if (strcmp(s, "<") == 0)  *op = OP_LT;
    else if (strcmp(s, ">") == 0)  *op = OP_GT;
    else return false;
    return true;
}

/* ------------------------------------------------------------------ */
/* Construcao da Query                                                 */
/* ------------------------------------------------------------------ */

static void add_filter(Query *q, const char *col, CmpOp op, const char *target) {
    if (q->filter_count >= q->filter_cap) {
        q->filter_cap = q->filter_cap ? q->filter_cap * 2 : 8;
        q->filters = (FilterRule *) xrealloc(q->filters, (size_t) q->filter_cap * sizeof(FilterRule));
    }
    FilterRule *r = &q->filters[q->filter_count++];
    r->col = dup_str(col);
    r->op = op;
    r->target = dup_str(target);
    r->num = 0.0;
    r->is_num = parse_double(target, &r->num);
    r->b = false;
    r->is_bool = false;
    if (equals_ignore_case(target, "true") || strcmp(target, "1") == 0) {
        r->is_bool = true; r->b = true;
    } else if (equals_ignore_case(target, "false") || strcmp(target, "0") == 0) {
        r->is_bool = true; r->b = false;
    }
}

static void add_sim(Query *q, const char *col, double target, double tol, double weight) {
    if (q->sim_count >= q->sim_cap) {
        q->sim_cap = q->sim_cap ? q->sim_cap * 2 : 8;
        q->sims = (SimRule *) xrealloc(q->sims, (size_t) q->sim_cap * sizeof(SimRule));
    }
    SimRule *r = &q->sims[q->sim_count++];
    r->col = dup_str(col);
    r->target = target;
    r->tolerance = tol;
    r->weight = weight;
}

static void parse_line(Query *q, char *line, int lineno) {
    char *cur = line;
    char *cmd = next_token(&cur);
    if (!cmd || cmd[0] == '#') return;

    bool ok = false;

    if (equals_ignore_case(cmd, "FILTER")) {
        char *col = next_token(&cur);
        char *opt = next_token(&cur);
        char *target = rest_of_line(&cur);
        CmpOp op = OP_EQ;
        if (col && opt && *target && parse_op(opt, &op)) {
            add_filter(q, col, op, target);
            ok = true;
        }
    } else if (equals_ignore_case(cmd, "SIM")) {
        char *col = next_token(&cur);
        char *t = next_token(&cur);
        char *tol = next_token(&cur);
        char *w = next_token(&cur);
        double tv, tolv, wv;
        if (col && parse_double(t, &tv) && parse_double(tol, &tolv) &&
            parse_double(w, &wv) && tolv > 0.0 && wv >= 0.0) {
            add_sim(q, col, tv, tolv, wv);
            ok = true;
        }
    } else if (equals_ignore_case(cmd, "ACCEPT")) {
        double v;
        if (parse_double(next_token(&cur), &v)) {
            q->accept_threshold = v;
            q->has_accept = true;
            ok = true;
        }
    } else if (equals_ignore_case(cmd, "ORDER")) {
        char *col = next_token(&cur);
        char *dir = next_token(&cur);
        bool desc = false;
        bool dir_ok = true;
        if (dir) {
            if (equals_ignore_case(dir, "DESC")) desc = true;
            else if (!equals_ignore_case(dir, "ASC")) dir_ok = false;
        }
        if (col && dir_ok) {
            set_str(&q->order_key, col);
            q->order_desc = desc;
            ok = true;
        }
    } else if (equals_ignore_case(cmd, "GROUP") || equals_ignore_case(cmd, "CROSS")) {
        bool is_group = equals_ignore_case(cmd, "GROUP");
        char *k1 = next_token(&cur);
        char *k2 = next_token(&cur);
        if (k1) {
            set_str(is_group ? &q->group1 : &q->cross1, k1);
            set_str(is_group ? &q->group2 : &q->cross2, k2);   /* k2 pode ser NULL */
            ok = true;
        }
    } else {
        fprintf(stderr, "aviso (consulta, linha %d): comando desconhecido '%s'\n", lineno, cmd);
        return;
    }

    if (!ok) {
        fprintf(stderr, "aviso (consulta, linha %d): argumentos invalidos para %s\n", lineno, cmd);
    }
}

Query* parse_query_file(const char *filename) {
    if (!filename) return NULL;
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    Query *q = (Query *) calloc(1, sizeof(Query));
    if (!q) { fclose(f); return NULL; }

    char *line;
    int lineno = 0;
    while ((line = csv_ler_linha(f)) != NULL) {   /* leitor de linha de tamanho livre */
        lineno++;
        char *text = line;
        if (lineno == 1 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
            (unsigned char)text[2] == 0xBF) {
            text += 3;                              /* BOM UTF-8 */
        }
        parse_line(q, text, lineno);
        free(line);
    }

    fclose(f);
    return q;
}

void query_free(Query *q) {
    if (!q) return;
    for (int i = 0; i < q->filter_count; i++) {
        free(q->filters[i].col);
        free(q->filters[i].target);
    }
    free(q->filters);
    for (int i = 0; i < q->sim_count; i++) free(q->sims[i].col);
    free(q->sims);
    free(q->order_key);
    free(q->group1); free(q->group2);
    free(q->cross1); free(q->cross2);
    free(q);
}

/* ------------------------------------------------------------------ */
/* Avaliacao unitaria                                                  */
/* ------------------------------------------------------------------ */

static bool cmp_int(CmpOp op, int c) {
    switch (op) {
        case OP_EQ: return c == 0;
        case OP_NE: return c != 0;
        case OP_LE: return c <= 0;
        case OP_GE: return c >= 0;
        case OP_LT: return c < 0;
        case OP_GT: return c > 0;
    }
    return false;
}

static bool cmp_double(CmpOp op, double a, double b) {
    switch (op) {
        case OP_EQ: return a == b;
        case OP_NE: return a != b;
        case OP_LE: return a <= b;
        case OP_GE: return a >= b;
        case OP_LT: return a < b;
        case OP_GT: return a > b;
    }
    return false;
}

static bool eval_filter(const ITEM *item, const FilterRule *r) {
    int idx = item_get_column_index(item, r->col);
    if (idx < 0) return false;

    switch (item_get_value_type(item, idx)) {
        case TYPE_BOOL: {
            if (!r->is_bool) return false;
            bool v = item_get_value_bool(item, idx);
            if (r->op == OP_EQ) return v == r->b;
            if (r->op == OP_NE) return v != r->b;
            return false;                           /* <, > etc. nao fazem sentido */
        }
        case TYPE_INT:
        case TYPE_FLOAT: {
            double v;
            if (!r->is_num || !item_get_value_as_double(item, idx, &v)) return false;
            return cmp_double(r->op, v, r->num);
        }
        case TYPE_STRING: {
            const char *s = item_get_value_string(item, idx);
            if (!s) return false;
            return cmp_int(r->op, strcmp(s, r->target));
        }
        default:                                    /* TYPE_NULL */
            return false;
    }
}

static double field_similarity(const ITEM *item, const SimRule *r) {
    int idx = item_get_column_index(item, r->col);
    double x;
    if (idx < 0 || !item_get_value_as_double(item, idx, &x)) return 0.0;

    double d = x - r->target;
    if (d < 0.0) d = -d;
    double s = 1.0 - d / r->tolerance;
    return s > 0.0 ? s : 0.0;
}

bool query_avaliar_item(const ITEM *item, const Query *q, double *out_similarity) {
    if (out_similarity) *out_similarity = 0.0;
    if (!item || !q) return false;

    for (int i = 0; i < q->filter_count; i++) {
        if (!eval_filter(item, &q->filters[i])) return false;
    }

    if (q->sim_count == 0) {
        if (out_similarity) *out_similarity = 1.0;
        return true;
    }

    double weighted_sum = 0.0, total_weight = 0.0;
    for (int i = 0; i < q->sim_count; i++) {
        weighted_sum += field_similarity(item, &q->sims[i]) * q->sims[i].weight;
        total_weight += q->sims[i].weight;
    }
    double total = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
    if (out_similarity) *out_similarity = total;

    if (q->has_accept && total + SIM_EPS < q->accept_threshold) return false;
    return true;
}

/* ------------------------------------------------------------------ */
/* Getters de organizacao                                              */
/* ------------------------------------------------------------------ */

static void copy_key(char *dst, size_t max_len, const char *src) {
    if (!dst || max_len == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= max_len) n = max_len - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

const char* query_get_order_key(const Query *q, bool *out_desc) {
    if (out_desc) *out_desc = q ? q->order_desc : false;
    return q ? q->order_key : NULL;
}

bool query_order_is_similarity(const Query *q) {
    return q && q->order_key && equals_ignore_case(q->order_key, "similarity");
}

void query_get_group_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len) {
    copy_key(out_k1, max_len, q ? q->group1 : NULL);
    copy_key(out_k2, max_len, q ? q->group2 : NULL);
}

void query_get_cross_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len) {
    copy_key(out_k1, max_len, q ? q->cross1 : NULL);
    copy_key(out_k2, max_len, q ? q->cross2 : NULL);
}

bool query_has_group(const Query *q) { return q && q->group1 && q->group2; }
bool query_has_cross(const Query *q) { return q && q->cross1 && q->cross2; }
