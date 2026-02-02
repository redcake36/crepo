#include "statdump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

// -------------------- helpers -------------------- 

static int float_eq_rel(float a, float b, float rel_eps) {
    float diff = (a > b) ? (a - b) : (b - a);
    float scale = (a > 0 ? a : -a);
    float scale_b = (b > 0 ? b : -b);
    if (scale_b > scale) scale = scale_b;
    if (scale < 1e-12f) scale = 1.0f;
    return diff <= rel_eps * scale;
}

static int stat_eq(const StatData *a, const StatData *b) {
    return (a->id == b->id &&
        a->count == b->count &&
        float_eq_rel(a->cost, b->cost, 1e-6f) &&
        a->primary == b->primary &&
        a->mode == b->mode);
}

static int run_tool(const char *tool_path, const char *fa, const char *fb, const char *fo) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s %s %s > /dev/null", tool_path, fa, fb, fo);
    int rc = system(cmd);
    return (rc == 0);
}

int run_tool_with_args(const char *tool, char *args[], int argc) {
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd), "%s", tool);
    for (int i = 0; i < argc; i++) {
        snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " %s", args[i]);
    }
    strncat(cmd, " > /dev/null", sizeof(cmd) - strlen(cmd) - 1);
    int rc = system(cmd);
    return (rc == 0);
}


static int load_and_check_exact(const char *fo, const StatData *exp, size_t nexp) {
    StatData *out = NULL; size_t nout = 0;
    if (LoadDump(fo, &out, &nout) != SD_OK) return 0;
    int ok = (nout == nexp);
    if (ok) {
        for (size_t i = 0; i < nexp; i++) {
            if (!stat_eq(&out[i], &exp[i])) { ok = 0; break; }
        }
    }
    free(out);
    return ok;
}

static int is_sorted_by_cost(const StatData *arr, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (arr[i-1].cost > arr[i].cost) return 0;
    }
    return 1;
}

// -------------------- test cases -------------------- 

typedef struct {
    const char *name;
    int (*fn)(const char *tool_path);
} TestCase;

// Case 1 from task
static const StatData case_1_in_a[2] =
{{.id = 90889, .count = 13, .cost = 3.567f, .primary = 0, .mode=3 },
 {.id = 90089, .count = 1,  .cost = 88.90f, .primary = 1, .mode=0 }};

static const StatData case_1_in_b[2] =
{{.id = 90089, .count = 13,   .cost = 0.011f,   .primary = 0, .mode=2 },
 {.id = 90189, .count = 1000, .cost = 1.00003f, .primary = 1, .mode=2}};

static const StatData case_1_out[3] =
{{.id = 90189, .count = 1000, .cost = 1.00003f, .primary = 1, .mode = 2 },
 {.id = 90889, .count = 13,   .cost = 3.567f,   .primary = 0, .mode = 3 },
 {.id = 90089, .count = 14,   .cost = 88.911f,  .primary = 0, .mode = 2 }};

static int test_case_1(const char *tool) {
    const char *fa = "t_case1_a.bin";
    const char *fb = "t_case1_b.bin";
    const char *fo = "t_case1_out.bin";

    if (StoreDump(fa, case_1_in_a, 2) != SD_OK) return 0;
    if (StoreDump(fb, case_1_in_b, 2) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;
    return load_and_check_exact(fo, case_1_out, 3);
}

// Case 2: both empty arrays -> empty output
static int test_empty_empty(const char *tool) {
    const char *fa = "t_empty_a.bin";
    const char *fb = "t_empty_b.bin";
    const char *fo = "t_empty_out.bin";

    if (StoreDump(fa, NULL, 0) != SD_OK) return 0;
    if (StoreDump(fb, NULL, 0) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;

    StatData *out = NULL; size_t nout = 0;
    if (LoadDump(fo, &out, &nout) != SD_OK) return 0;
    free(out);
    return (nout == 0);
}

// Case 3: one empty, one non-empty -> sorted copy
static const StatData case_3_in_b[4] =
{{.id=3, .count=1, .cost=9.0f, .primary=1, .mode=1},
 {.id=1, .count=1, .cost=2.0f, .primary=1, .mode=0},
 {.id=2, .count=1, .cost=5.0f, .primary=0, .mode=7},
 {.id=4, .count=1, .cost=1.0f, .primary=1, .mode=2}};

static const StatData case_3_out[4] =
{{.id=4, .count=1, .cost=1.0f, .primary=1, .mode=2},
 {.id=1, .count=1, .cost=2.0f, .primary=1, .mode=0},
 {.id=2, .count=1, .cost=5.0f, .primary=0, .mode=7},
 {.id=3, .count=1, .cost=9.0f, .primary=1, .mode=1}};

static int test_one_empty(const char *tool) {
    const char *fa = "t_oneempty_a.bin";
    const char *fb = "t_oneempty_b.bin";
    const char *fo = "t_oneempty_out.bin";

    if (StoreDump(fa, NULL, 0) != SD_OK) return 0;
    if (StoreDump(fb, case_3_in_b, 4) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;
    return load_and_check_exact(fo, case_3_out, 4);
}

// Case 4: duplicates inside the same array + across arrays
static const StatData case_4_in_a[5] =
{
 {.id=10, .count=1, .cost=1.0f, .primary=1, .mode=1},
 {.id=10, .count=2, .cost=2.0f, .primary=1, .mode=3},
 {.id=20, .count=1, .cost=1.5f, .primary=0, .mode=2},
 {.id=10, .count=3, .cost=3.0f, .primary=0, .mode=0},
 {.id=30, .count=5, .cost=0.5f, .primary=1, .mode=7},
};

static const StatData case_4_in_b[3] =
{
 {.id=20, .count=2, .cost=2.5f, .primary=1, .mode=7},
 {.id=40, .count=1, .cost=4.0f, .primary=1, .mode=1},
 {.id=10, .count=4, .cost=4.0f, .primary=1, .mode=2},
};

// Expected aggregation:
// id=10: count 1+2+3+4 =10, cost 1+2+3+4=10, primary -> 0, mode max(1,3,0,2)=3
// id=20: count 1+2=3, cost 1.5+2.5=4.0, primary ->0, mode max(2,7)=7
// id=30: unchanged
// id=40: unchanged
// Sort by cost: (0.5 id30), (4.0 id20), (4.0 id40), (10.0 id10)

static const StatData case_4_out[4] =
{
 {.id=30, .count=5,  .cost=0.5f,  .primary=1, .mode=7},
 {.id=20, .count=3,  .cost=4.0f,  .primary=0, .mode=7},
 {.id=40, .count=1,  .cost=4.0f,  .primary=1, .mode=1},
 {.id=10, .count=10, .cost=10.0f, .primary=0, .mode=3},
};

static int test_dups_within_and_across(const char *tool) {
    const char *fa = "t_dups_a.bin";
    const char *fb = "t_dups_b.bin";
    const char *fo = "t_dups_out.bin";

    if (StoreDump(fa, case_4_in_a, 5) != SD_OK) return 0;
    if (StoreDump(fb, case_4_in_b, 3) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;
    return load_and_check_exact(fo, case_4_out, 4);
}

// Case 5: mode max + primary AND logic isolated check 
static const StatData case_5_in_a[2] =
{
 {.id=7, .count=1, .cost=1.0f, .primary=1, .mode=1},
 {.id=7, .count=1, .cost=1.0f, .primary=1, .mode=6},
};
static const StatData case_5_in_b[1] =
{
 {.id=7, .count=1, .cost=1.0f, .primary=0, .mode=2},
};
// total: count=3 cost=3 primary=0 mode=max(1,6,2)=6 
static const StatData case_5_out[1] =
{
 {.id=7, .count=3, .cost=3.0f, .primary=0, .mode=6},
};

static int test_primary_mode_rules(const char *tool) {
    const char *fa = "t_rules_a.bin";
    const char *fb = "t_rules_b.bin";
    const char *fo = "t_rules_out.bin";

    if (StoreDump(fa, case_5_in_a, 2) != SD_OK) return 0;
    if (StoreDump(fb, case_5_in_b, 1) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;
    return load_and_check_exact(fo, case_5_out, 1);
}

// Case 6: large randomized
static int test_large_random_sanity(const char *tool) {
    const char *fa = "t_large_a.bin";
    const char *fb = "t_large_b.bin";
    const char *fo = "t_large_out.bin";

    const size_t na = 50000;
    const size_t nb = 50000;
    StatData *a = (StatData*)calloc(na, sizeof(StatData));
    StatData *b = (StatData*)calloc(nb, sizeof(StatData));
    if (!a || !b) { free(a); free(b); return 0; }

    // collisions: id in [0..19999]
    for (size_t i = 0; i < na; i++) {
        a[i].id = (long)(rand() % 20000u);
        a[i].count = (int)(rand() % 5u);
        a[i].cost = (float)rand() / (float)RAND_MAX * 1000.0f;
        a[i].primary = (unsigned)(rand() & 1u);
        a[i].mode = (unsigned)(rand() & 7u);
    }
    for (size_t i = 0; i < nb; i++) {
        b[i].id = (long)(rand() % 20000u);
        b[i].count = (int)(rand() % 5u);
        b[i].cost = (float)rand() / (float)RAND_MAX * 1000.0f;
        b[i].primary = (unsigned)(rand() & 1u);
        b[i].mode = (unsigned)(rand() & 7u);
    }

    int ok = 1;
    if (StoreDump(fa, a, na) != SD_OK) ok = 0;
    if (ok && StoreDump(fb, b, nb) != SD_OK) ok = 0;

    free(a); free(b);

    if (ok && !run_tool(tool, fa, fb, fo)) ok = 0;

    StatData *out = NULL; size_t nout = 0;
    if (ok && LoadDump(fo, &out, &nout) != SD_OK) ok = 0;

    if (ok && !is_sorted_by_cost(out, nout)) ok = 0;

    if (ok) {
        for (size_t i = 1; i < nout; i++) {
            if (out[i].id == out[i-1].id) { 
                break;
            }
        }

        StatData *cpy = (nout ? (StatData*)malloc(nout * sizeof(StatData)) : NULL);
        if (nout && !cpy) ok = 0;
        if (ok && nout) {
            memcpy(cpy, out, nout * sizeof(StatData));
            
            int cmp_id(const void *pa, const void *pb) {
                const StatData *x = (const StatData*)pa;
                const StatData *y = (const StatData*)pb;
                if (x->id < y->id) return -1;
                if (x->id > y->id) return 1;
                return 0;
            }
            qsort(cpy, nout, sizeof(StatData), cmp_id);
            for (size_t i = 1; i < nout; i++) {
                if (cpy[i].id == cpy[i-1].id) { ok = 0; break; }
            }
        }
        free(cpy);
    }

    free(out);
    return ok;
}

// Case 7: wrong args number
static int test_invalid_arguments(const char *tool) {
    char *invalid_args[] = {"statdump_tool", "input_a.bin"};
    int result = run_tool_with_args(tool, invalid_args, 2);

    if (result == 0) {
        printf("Error: Incorrect number of arguments.\n");
    }
    return (result == 0);
}

// Case 8: read & write non existing file
static int test_file_read_error(const char *tool) {
    const char *invalid_file = "invalid_file.bin";

    StatData *out = NULL;
    size_t nout = 0;
    int result = LoadDump(invalid_file, &out, &nout);
    if (result == SD_ERR_IO) {
        printf("Error: File read failed.\n");
    }

    return (result == SD_ERR_IO);
}

// Case 9: read no header file
static int test_corrupted_file(const char *tool) {
    const char *corrupted_file = "corrupted_file.bin";

    StatData corrupted_data = {.id = 1, .count = 1, .cost = 1.0f, .primary = 1, .mode = 1};
    FILE *file = fopen(corrupted_file, "wb");
    // only one record
    fwrite(&corrupted_data, sizeof(corrupted_data), 1, file);
    fclose(file);

    StatData *out = NULL;
    size_t nout = 0;
    int result = LoadDump(corrupted_file, &out, &nout);
    if (result == SD_ERR_FMT) {
        printf("Error: File format corrupted.\n");
    }

    return (result == SD_ERR_FMT);
}

// Case 10 for table form
static const StatData case_11_in_b[11] =
{{.id=1, .count=1, .cost=7.0f, .primary=1, .mode=1},
 {.id=2, .count=2, .cost=8.0f, .primary=1, .mode=2},
 {.id=3, .count=3, .cost=9.0f, .primary=1, .mode=3},
 {.id=4, .count=4, .cost=10.0f, .primary=1, .mode=4},
 {.id=5, .count=5, .cost=11.0f, .primary=1, .mode=5},
 {.id=6, .count=6, .cost=12.0f, .primary=1, .mode=6},
 {.id=7, .count=7, .cost=13.0f, .primary=1, .mode=7},
 {.id=8, .count=8, .cost=14.0f, .primary=1, .mode=1},
 {.id=9, .count=9, .cost=15.0f, .primary=1, .mode=2},
 {.id=10, .count=10, .cost=16.0f, .primary=1, .mode=3},
 {.id=11, .count=11, .cost=17.0f, .primary=1, .mode=4}};

static const StatData case_11_out[11] =
{{.id=1, .count=1, .cost=7.0f, .primary=1, .mode=1},
 {.id=2, .count=2, .cost=8.0f, .primary=1, .mode=2},
 {.id=3, .count=3, .cost=9.0f, .primary=1, .mode=3},
 {.id=4, .count=4, .cost=10.0f, .primary=1, .mode=4},
 {.id=5, .count=5, .cost=11.0f, .primary=1, .mode=5},
 {.id=6, .count=6, .cost=12.0f, .primary=1, .mode=6},
 {.id=7, .count=7, .cost=13.0f, .primary=1, .mode=7},
 {.id=8, .count=8, .cost=14.0f, .primary=1, .mode=1},
 {.id=9, .count=9, .cost=15.0f, .primary=1, .mode=2},
 {.id=10, .count=10, .cost=16.0f, .primary=1, .mode=3},
 {.id=11, .count=11, .cost=17.0f, .primary=1, .mode=4}};

static int test_eleven_records(const char *tool) {
    const char *fa = "t_ele_a.bin";
    const char *fb = "t_ele_b.bin";
    const char *fo = "t_ele_out.bin";

    if (StoreDump(fa, NULL, 0) != SD_OK) return 0;
    if (StoreDump(fb, case_11_in_b, 11) != SD_OK) return 0;

    if (!run_tool(tool, fa, fb, fo)) return 0;
    return load_and_check_exact(fo, case_11_out, 11);
}

// -------------------- runner -------------------- 

static int run_one(int num, const char *tool, const TestCase *tc) {
    int ok = tc->fn(tool);
    if (!ok) fprintf(stderr, "%d [FAIL] %s\n", num+1, tc->name);
    else printf("%d [OK]   %s\n", num+1, tc->name);
    return ok;
}

int main(int argc, char **argv) {
    const char *tool = (argc >= 2) ? argv[1] : "./statdump_tool";

    srand((unsigned int)time(NULL));
    const TestCase tests[] = {
        {"case_1_spec",test_case_1},
        {"empty_empty",test_empty_empty},
        {"one_empty_sorted_copy", test_one_empty},
        {"dups_within_and_across", test_dups_within_and_across},
        {"primary_mode_rules", test_primary_mode_rules},
        {"large_random_sanity", test_large_random_sanity},
        {"invalid_arguments", test_invalid_arguments},
        {"file_read_error", test_file_read_error},
        {"corrupted_file", test_corrupted_file},
        {"eleven_records", test_eleven_records}
    };

    clock_t t0 = clock();

    int all_ok = 1;
    int passed_tests = 0; 
    size_t num_tests = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < num_tests; i++) {
        if (run_one(i, tool, &tests[i])) {
            passed_tests++;
        } else {
            all_ok = 0;
        }
    }

    clock_t t1 = clock();
    double ms = 1000.0 * (double)(t1 - t0) / (double)CLOCKS_PER_SEC;

    printf("Tests passed: %d / %zu\n", passed_tests, num_tests);
    if (all_ok) {
        printf("All tests passed. Time: %.3f ms\n", ms);
        return 0;
    } else {
        fprintf(stderr, "Some tests FAILED. Time: %.3f ms\n", ms);
        return 1;
    }
}