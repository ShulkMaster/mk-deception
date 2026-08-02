#ifndef LIBMKPARTICLE_METRICS_H
#define LIBMKPARTICLE_METRICS_H

typedef struct PfxMetricsCounters {
    int values[8];
} PfxMetricsCounters;

typedef struct PfxMetrics {
    char filename[0x100];
    int frame_active;
    int frame_count;
    int frame_limit;
    PfxMetricsCounters* frames;
} PfxMetrics;

typedef struct PfxMetricsInterface {
    void* (*open)(const char* path);
    void* (*begin_write)(void);
    void (*write)(void* handle, const void* data, int size);
    void (*close)(void* handle);
    void (*begin_counter)(void);
    int (*end_counter)(int valid);
} PfxMetricsInterface;

void pfxmetrics_set_interface(PfxMetricsInterface* interface);
void pfxmetrics_event(PfxMetrics* metrics, int event);
void pfxmetrics_flush(PfxMetrics* metrics);
int pfxmetrics_estimate_size(int frame_count);
PfxMetrics* pfxmetrics_set_mem(PfxMetrics* metrics, int frame_count);
void pfxmetrics_init(PfxMetrics* metrics, const char* filename);
PfxMetricsCounters* pfxmetrics_get_current(PfxMetrics* metrics);
void pfxmetrics_begin_frame(void);

#endif
