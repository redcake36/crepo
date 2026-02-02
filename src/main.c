#include "statdump.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <in_a> <in_b> <out>\n", argv[0]);
        return 2;
    }

    StatData *a = NULL, *b = NULL;
    size_t na = 0, nb = 0;

    SdStatus st = LoadDump(argv[1], &a, &na);
    if (st != SD_OK) { fprintf(stderr, "LoadDump(%s): %s\n", argv[1], SdStatusStr(st)); return 1; }

    st = LoadDump(argv[2], &b, &nb);
    if (st != SD_OK) { fprintf(stderr, "LoadDump(%s): %s\n", argv[2], SdStatusStr(st)); free(a); return 1; }

    StatData *j = NULL;
    size_t nj = 0;
    st = JoinDump(a, na, b, nb, &j, &nj);
    free(a); free(b);
    if (st != SD_OK) { fprintf(stderr, "JoinDump: %s\n", SdStatusStr(st)); return 1; }

    SortDump(j, nj);
    PrintTop10Table(j, nj);

    st = StoreDump(argv[3], j, nj);
    free(j);
    if (st != SD_OK) { fprintf(stderr, "StoreDump(%s): %s\n", argv[3], SdStatusStr(st)); return 1; }

    return 0;
}
