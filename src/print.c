#include "statdump.h"
#include <stdio.h>
#include <stdint.h>

static void print_mode_bin(unsigned mode) {
    if ((mode & 7u) == 0) { fputs("0", stdout); return; }
    for (int bit = 2; bit >= 0; --bit) {
        if (mode & (1u << bit)) {
            for (int b = bit; b >= 0; --b) putchar((mode & (1u << b)) ? '1' : '0');
            return;
        }
    }
}

void PrintTop10Table(const StatData *arr, size_t n) {
    printf("%-18s %-11s %-9s %-8s %-8s\n", "id", "count", "cost", "primary", "mode");
    printf("---------------------------------------------------------------\n");
    size_t k = (n < 10) ? n : 10;
    for (size_t i = 0; i < k; i++) {
        printf("0x%016llx %-10d % .3e %-8s ",
               (unsigned long long)(uint64_t)(int64_t)arr[i].id,
               arr[i].count,
               arr[i].cost,
               arr[i].primary ? "y" : "n");
        print_mode_bin(arr[i].mode);
        putchar('\n');
    }
}
