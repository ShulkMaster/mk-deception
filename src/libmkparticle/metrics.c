#include "libmkparticle/metrics.h"
#include "runtime/cstring.h"

static PfxMetricsInterface metrics_interface = {0};
static int counter_offset[6] = {0, 4, 12, 8, 20, 16};
static char string_base[] = ".ppd\0PFX Metrics File\x1a";

static PfxMetricsCounters last_frame_data;
static PfxMetricsCounters current_frame_data;

/* Soft ceiling: metrics.o ~99.96% -- pfxmetrics_get_current is instruction-exact;
 * only its compiler-generated default_buffer relocation suffix differs. */
void pfxmetrics_set_interface(PfxMetricsInterface* interface) {
    memcpy(&metrics_interface, interface, sizeof(PfxMetricsInterface));
}

void pfxmetrics_event(PfxMetrics* metrics, int event) {
    PfxMetricsCounters* counters;
    int offset;
    int counter;
    int type;
    int nonzero_counter;

    if (event != 0x4005 && metrics != 0 && metrics->frame_active != 0) {
        metrics->frame_count++;
        if (metrics->frame_count >= metrics->frame_limit) {
            pfxmetrics_flush(metrics);
        }
        metrics->frame_active = 0;
    }

    counters = pfxmetrics_get_current(metrics);
    counter = event & 0xF;
    offset = counter_offset[counter];
    nonzero_counter = counter == 0 ? 0 : 1;
    type = event & 0xFF00;
    switch (type) {
    case 0x1000:
        metrics_interface.begin_counter(nonzero_counter);
        break;
    case 0x2000:
        /* Retail's table contains byte offsets, so preserve its indexed store. */
        *(int*)((char*)counters + offset) =
            metrics_interface.end_counter(nonzero_counter);
        break;
    }

    if (event == 0x1000) {
        memset(counters, 0, sizeof(PfxMetricsCounters));
    }
    if (event == 0x2000) {
        current_frame_data.values[0] += counters->values[0];
        current_frame_data.values[0] += counters->values[5];
        current_frame_data.values[0] += counters->values[4];
    }
    if (event == 0x4005) {
        current_frame_data.values[6] += counters->values[6];
        current_frame_data.values[7] += counters->values[7];
        if (metrics != 0) {
            metrics->frame_active = 1;
        }
    }
    if (event == 0x4002) {
        pfxmetrics_flush(metrics);
    }
}

void pfxmetrics_flush(PfxMetrics* metrics) {
    void* handle;

    handle = metrics_interface.begin_write(metrics->filename);
    if (handle == 0) {
        metrics->frame_count = 0;
    } else {
        metrics_interface.write(handle, metrics->frames,
                                metrics->frame_count * sizeof(PfxMetricsCounters));
        metrics_interface.close(handle);
        metrics->frame_count = 0;
    }
}

int pfxmetrics_estimate_size(int frame_count) {
    if (frame_count != 0) {
        return frame_count * sizeof(PfxMetricsCounters) + 0x110;
    }
    return 0;
}

PfxMetrics* pfxmetrics_set_mem(PfxMetrics* metrics, int frame_count) {
    if (frame_count == 0) {
        return 0;
    }
    metrics->frames = (PfxMetricsCounters*)(metrics + 1);
    metrics->frame_limit = frame_count;
    return metrics;
}

void pfxmetrics_init(PfxMetrics* metrics, const char* filename) {
    void* handle;
    char* header;
    int version;

    strcpy(metrics->filename, filename);
    strcat(metrics->filename, string_base);
    handle = metrics_interface.open(metrics->filename);
    if (handle != 0) {
        header = string_base + 5;
        version = 2;
        metrics_interface.write(handle, header, strlen(header));
        metrics_interface.write(handle, &version, 4);
        metrics_interface.close(handle);
    }
    metrics->frame_count = 0;
    metrics->frame_active = 0;
}

#pragma scheduling on
PfxMetricsCounters* pfxmetrics_get_current(PfxMetrics* metrics) {
    static PfxMetricsCounters default_buffer;

    if (metrics == 0) {
        return &default_buffer;
    }
    return &metrics->frames[metrics->frame_count];
}
#pragma scheduling reset

void pfxmetrics_begin_frame(void) {
    memcpy(&last_frame_data, &current_frame_data,
           sizeof(PfxMetricsCounters));
    memset(&current_frame_data, 0, sizeof(PfxMetricsCounters));
}
