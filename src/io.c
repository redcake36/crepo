#include "statdump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SD_MAGIC 0x504D4453u // 'SDMP' | file identifier
#define SD_VERSION 1u // file format version

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t nrecords;
} __attribute__((packed)) SdHeader;

typedef struct {
    int64_t  id;
    int32_t  count;
    float    cost;
    uint8_t  primary;
    uint8_t  mode;
} __attribute__((packed)) SdRecord;

static SdStatus write_all(FILE *f, const void *p, size_t sz) {
    return (fwrite(p, 1, sz, f) == sz) ? SD_OK : SD_ERR_IO;
}
static SdStatus read_all(FILE *f, void *p, size_t sz) {
    return (fread(p, 1, sz, f) == sz) ? SD_OK : SD_ERR_IO;
}

SdStatus StoreDump(const char *path, const StatData *arr, size_t n) {
    if (!path || (!arr && n != 0)) return SD_ERR_INVAL;

    FILE *f = fopen(path, "wb");
    if (!f) return SD_ERR_IO;

    SdHeader h = { SD_MAGIC, SD_VERSION, (uint32_t)n };
    SdStatus st = write_all(f, &h, sizeof(h));
    if (st != SD_OK) { fclose(f); return st; }

    for (size_t i = 0; i < n; i++) {
        SdRecord r;
        r.id = (int64_t)arr[i].id;
        r.count = (int32_t)arr[i].count;
        r.cost = arr[i].cost;
        r.primary = (uint8_t)(arr[i].primary ? 1 : 0);
        r.mode = (uint8_t)(arr[i].mode & 0x7u);

        st = write_all(f, &r, sizeof(r));
        if (st != SD_OK) { fclose(f); return st; }
    }

    if (fclose(f) != 0) return SD_ERR_IO;
    return SD_OK;
}

SdStatus LoadDump(const char *path, StatData **out_arr, size_t *out_n) {
    if (!path || !out_arr || !out_n) return SD_ERR_INVAL;
    *out_arr = NULL; *out_n = 0;

    FILE *f = fopen(path, "rb");
    if (!f) return SD_ERR_IO;

    SdHeader h;
    SdStatus st = read_all(f, &h, sizeof(h));
    if (st != SD_OK) { fclose(f); return st; }

    if (h.magic != SD_MAGIC || h.version != SD_VERSION) {
        fclose(f);
        return SD_ERR_FMT;
    }

    size_t n = (size_t)h.nrecords;
    StatData *arr = (n == 0) ? NULL : (StatData*)calloc(n, sizeof(StatData));
    if (n != 0 && !arr) { fclose(f); return SD_ERR_OOM; }

    for (size_t i = 0; i < n; i++) {
        SdRecord r;
        st = read_all(f, &r, sizeof(r));
        if (st != SD_OK) { free(arr); fclose(f); return st; }

        arr[i].id = (long)r.id;
        arr[i].count = (int)r.count;
        arr[i].cost = r.cost;
        arr[i].primary = (unsigned)(r.primary ? 1 : 0);
        arr[i].mode = (unsigned)(r.mode & 0x7u);
    }

    fclose(f);
    *out_arr = arr;
    *out_n = n;
    return SD_OK;
}

const char* SdStatusStr(SdStatus s) {
    switch (s) {
        case SD_OK: return "OK";
        case SD_ERR_IO: return "I/O error";
        case SD_ERR_FMT: return "Format error";
        case SD_ERR_OOM: return "Out of memory";
        case SD_ERR_INVAL: return "Invalid argument";
        default: return "Unknown";
    }
}
