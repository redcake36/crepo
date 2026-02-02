#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StatData {
    long id;
    int count;
    float cost;
    unsigned int primary:1;
    unsigned int mode:3;
} StatData;

typedef enum {
    SD_OK = 0,
    SD_ERR_IO,
    SD_ERR_FMT,
    SD_ERR_OOM,
    SD_ERR_INVAL
} SdStatus;

// I/O 
SdStatus StoreDump(const char *path, const StatData *arr, size_t n);
SdStatus LoadDump(const char *path, StatData **out_arr, size_t *out_n);

// Processing
SdStatus JoinDump(const StatData *a, size_t na,
                  const StatData *b, size_t nb,
                  StatData **out_arr, size_t *out_n);

void SortDump(StatData *arr, size_t n);

// Output formatting
void PrintTop10Table(const StatData *arr, size_t n);

// Helpers
const char* SdStatusStr(SdStatus s);

#ifdef __cplusplus
}
#endif
