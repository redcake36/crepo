#include "statdump.h"
#include <stdlib.h>
#include <string.h>

static int cmp_id(const void *pa, const void *pb) {
    const StatData *a = (const StatData*)pa;
    const StatData *b = (const StatData*)pb;
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static StatData fold_two(const StatData *x, const StatData *y) {
    StatData r = *x;
    r.count = x->count + y->count;
    r.cost  = x->cost + y->cost;
    r.primary = (unsigned)((x->primary && y->primary) ? 1 : 0);
    r.mode = (unsigned)((x->mode > y->mode) ? x->mode : y->mode);
    return r;
}

SdStatus JoinDump(const StatData *a, size_t na,
                  const StatData *b, size_t nb,
                  StatData **out_arr, size_t *out_n) {
    if (!out_arr || !out_n) return SD_ERR_INVAL;
    if ((!a && na) || (!b && nb)) return SD_ERR_INVAL;

    *out_arr = NULL; *out_n = 0;

    size_t n = na + nb;
    if (n == 0) return SD_OK;

    StatData *tmp = (StatData*)malloc(n * sizeof(StatData));
    if (!tmp) return SD_ERR_OOM;

    if (na) memcpy(tmp, a, na * sizeof(StatData));
    if (nb) memcpy(tmp + na, b, nb * sizeof(StatData));

    qsort(tmp, n, sizeof(StatData), cmp_id);

    size_t w = 0;
    for (size_t i = 0; i < n; ) {
        StatData acc = tmp[i];
        size_t j = i + 1;
        while (j < n && tmp[j].id == acc.id) {
            acc = fold_two(&acc, &tmp[j]);
            j++;
        }
        tmp[w++] = acc;
        i = j;
    }

    StatData *out = (StatData*)realloc(tmp, w * sizeof(StatData));
    if (!out) out = tmp;

    *out_arr = out;
    *out_n = w;
    return SD_OK;
}
