#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include "generic_item.h"
#include "generic_search.h"

#define MAX_LINE_LEN 2048

typedef struct {
    char col[64];
    char op[8];
    char target[128];
} FilterRule;

typedef struct {
    char col[64];
    double target;
    double tolerance;
    double weight;
} SimRule;

struct query_ {
    FilterRule filters[32];
    int filter_count;
    SimRule sims[32];
    int sim_count;
    double accept_threshold;
    bool has_accept;
};

bool eval_filter_rule(const Item *item, const FilterRule *rule) {
    if (!item || !rule) return false;

    int col_idx = item_get_column_index(item, rule->col);
    if (col_idx == -1) return false;

    DataType type = item_get_value_type(item, col_idx);
    if (type == TYPE_NULL) return false;

    if (type == TYPE_BOOL) {
        bool val_b = item_get_value_bool(item, col_idx);
        bool target_b = (strcasecmp(rule->target, "true") == 0 || strcmp(rule->target, "1") == 0);
        if (strcmp(rule->op, "==") == 0) return val_b == target_b;
        if (strcmp(rule->op, "!=") == 0) return val_b != target_b;
    }

    double num_val = 0.0;
    if (item_get_value_as_double(item, col_idx, &num_val)) {
        double target_num = atof(rule->target);
        if (strcmp(rule->op, "==") == 0) return num_val == target_num;
        if (strcmp(rule->op, "!=") == 0) return num_val != target_num;
        if (strcmp(rule->op, "<=") == 0) return num_val <= target_num;
        if (strcmp(rule->op, ">=") == 0) return num_val >= target_num;
        if (strcmp(rule->op, "<")  == 0) return num_val < target_num;
        if (strcmp(rule->op, ">")  == 0) return num_val > target_num;
    }

    if (type == TYPE_STRING) {
        const char *str_val = item_get_value_string(item, col_idx);
        if (str_val != NULL) {
            int cmp = strcmp(str_val, rule->target);
            if (strcmp(rule->op, "==") == 0) return cmp == 0;
            if (strcmp(rule->op, "!=") == 0) return cmp != 0;
        }
    }

    return false;
}

double calculate_field_similarity(const Item *item, const SimRule *rule) {
    if (!item || !rule || rule->tolerance <= 0.0) return 0.0;

    int col_idx = item_get_column_index(item, rule->col);
    if (col_idx == -1) return 0.0;

    double x = 0.0;
    if (!item_get_value_as_double(item, col_idx, &x)) return 0.0;

    double sim = 1.0 - (fabs(x - rule->target) / rule->tolerance);
    return (sim > 0.0) ? sim : 0.0;
}

Query* parse_query_file(const char *filename) {
    if (!filename) return NULL;
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    Query *q = (Query*) malloc(sizeof(Query));
    if (!q) { fclose(f); return NULL; }

    q->filter_count = 0;
    q->sim_count = 0;
    q->accept_threshold = 0.0;
    q->has_accept = false;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        char *cmd = strtok(line, "|");
        if (!cmd) continue;

        if (strcmp(cmd, "FILTER") == 0) {
            if (q->filter_count >= 32) continue;
            char *col = strtok(NULL, "|");
            char *op = strtok(NULL, "|");
            char *target = strtok(NULL, "|");
            if (col && op && target) {
                strncpy(q->filters[q->filter_count].col, col, sizeof(q->filters[0].col) - 1);
                strncpy(q->filters[q->filter_count].op, op, sizeof(q->filters[0].op) - 1);
                strncpy(q->filters[q->filter_count].target, target, sizeof(q->filters[0].target) - 1);
                q->filter_count++;
            }
        } else if (strcmp(cmd, "SIM") == 0) {
            if (q->sim_count >= 32) continue;
            char *col = strtok(NULL, "|");
            char *target = strtok(NULL, "|");
            char *tol = strtok(NULL, "|");
            char *weight = strtok(NULL, "|");
            if (col && target && tol && weight) {
                strncpy(q->sims[q->sim_count].col, col, sizeof(q->sims[0].col) - 1);
                q->sims[q->sim_count].target = atof(target);
                q->sims[q->sim_count].tolerance = atof(tol);
                q->sims[q->sim_count].weight = atof(weight);
                q->sim_count++;
            }
        } else if (strcmp(cmd, "ACCEPT") == 0) {
            char *val = strtok(NULL, "|");
            if (val) {
                q->accept_threshold = atof(val);
                q->has_accept = true;
            }
        }
    }

    fclose(f);
    return q;
}

Item** execute_query(Item **dataset, int total_rows, const Query *q, int *out_count) {
    if (!out_count) return NULL;
    if (!dataset || total_rows <= 0 || !q) {
        *out_count = 0;
        return NULL;
    }

    Item **results = (Item**) malloc(total_rows * sizeof(Item*));
    if (!results) { *out_count = 0; return NULL; }

    int match_count = 0;
    for (int i = 0; i < total_rows; i++) {
        Item *item = dataset[i];
        if (!item) continue;

        bool pass_filters = true;
        for (int f = 0; f < q->filter_count; f++) {
            if (!eval_filter_rule(item, &q->filters[f])) {
                pass_filters = false;
                break;
            }
        }
        if (!pass_filters) continue;

        if (q->sim_count > 0) {
            double weighted_sum = 0.0;
            double total_weight = 0.0;

            for (int s = 0; s < q->sim_count; s++) {
                double sim = calculate_field_similarity(item, &q->sims[s]);
                weighted_sum += sim * q->sims[s].weight;
                total_weight += q->sims[s].weight;
            }

            double total_sim = (total_weight > 0) ? (weighted_sum / total_weight) : 0.0;
            if (q->has_accept && total_sim < q->accept_threshold) continue;
        }

        results[match_count++] = item;
    }

    *out_count = match_count;
    return results;
}

void query_free(Query *q) {
    if (q) free(q);
}