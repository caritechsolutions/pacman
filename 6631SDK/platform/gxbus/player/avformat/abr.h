#ifndef AVFORMAT_ABR_H
#define AVFORMAT_ABR_H

#include <stdlib.h>

typedef enum ABRAlgorithm {
    AlwaysFirst,
    SimpleThroughput,
    BBA_0
} ABRAlgorithm;

typedef struct ABRContext {
    int max_history_len;
    float* throughput_history;
    ABRAlgorithm algorithm;

    // BBA algorithm needed(seconds)
    int buffer_max;
    int buffer_r;
    int buffer_c;
} ABRContext;

void abr_add_metric(ABRContext* ac, float tpt, int buffer_max, int buffer_r, int buffer_c);

int  abr_get_stream(ABRContext* ac, int* bandwidth, int size); // if return -1, that means no-change.

#endif /* AVFORMAT_ABR_H */
