#include "statdump.h"
#include <stdlib.h>

static int cmp_cost(const void *pa, const void *pb) {
    const StatData *a = (const StatData*)pa;
    const StatData *b = (const StatData*)pb;
    if (a->cost < b->cost) return -1;
    if (a->cost > b->cost) return 1;
    return 0;
}

void SortDump(StatData *arr, size_t n) {
    if (!arr || n == 0) return;
    qsort(arr, n, sizeof(StatData), cmp_cost);
}
