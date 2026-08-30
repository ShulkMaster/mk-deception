#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "sofdec/uty_math.h"

/* Stream-time adjustment state is TU-owned by the Sofdec timer path. */
typedef struct SFTST_Time {
    long long value;
    long long scale;
} SFTST_Time;

typedef struct SFTST_Work {
    int test_enabled;
    int paused;
    int reset_history;
    int adjust_enabled;
    int average_count;
    int average_index;
    int errors[60];
    SFTST_Time input_time;
    SFTST_Time sample_time;
    SFTST_Time output_time;
    SFTST_Time tolerance;
    SFTST_Time excess_error;
    SFTST_Time adjustment_start;
    SFTST_Time adjustment_offset;
    long long previous_sample;
    long long adjusted_time;
    long long maximum_time;
    int adjustment_count;
    int positive_adjustments;
    int negative_adjustments;
    int history_resets;
    int excess_resets;
    int average;
    int adjusted_average;
    int front_max;
    int front_min;
    int rear_max;
    int rear_min;
    int field_0x1BC;
} SFTST_Work;

/* MWCC emits these zero-initialized definitions in reverse declaration order. */
int gap_06_804AC8B4_bss;
SFTST_Work* sftst_last;
char* sftst_debout_write;
char* sftst_debout_round;
char* sftst_debout_buf;
int sftst_debout_siz;

extern int sfadxt_stat;

typedef struct SFTST_Header {
    char text[274];
} SFTST_Header;

static const SFTST_Header sftst_header = {{
    "tst, help_time_sec, help_time_msec, help_time_64, help_time, mt_max, master_time, out_time,  mt_ot, mtmax_ot,  diff_l_max, diff_l_min, diff_a_max, tst->diff_a_min, pastat, adjmode, resethist, excesserr, adj_limit, adj_front, adj_rear,  movave_1st, movave_2nd,  adxt_stat \n\n"
}};
static const char sftst_format[] =
    "%p, %ld, %ld, %08lX%08lX, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld \n";

static inline long long scale_value(long long value, const SFTST_Time* ratio)
{
    return value * ratio->value / ratio->scale;
}

static inline long long current_output(const SFTST_Work* work,
                                       const SFTST_Time* master,
                                       const SFTST_Time* sample)
{
    if (work->previous_sample == -1) {
        return 0;
    }
    return work->adjusted_time +
           scale_value(sample->value - work->previous_sample, master);
}

void SFTST_Calc(SFTST_Work* work, SFTST_Time* master,
                const SFTST_Time* sample, SFTST_Time* output)
{
    long long predicted;
    long long error;
    long long limit;
    long long correction;
    int sum;
    int i;
    int index;

    if (sample->scale == 1 || work->test_enabled == 0) {
        *output = *master;
        return;
    }
    master->value = master->value < work->maximum_time
                        ? work->maximum_time : master->value;
    if (work->paused == 1) {
        work->reset_history = 0;
    } else if (work->reset_history == 0) {
        if (master->value > work->maximum_time) {
            work->reset_history = 1;
            if (work->previous_sample == -1) {
                if (work->adjustment_start.value == -1) {
                    work->adjusted_time =
                        master->value +
                        scale_value(master->scale, &work->adjustment_start);
                } else {
                    work->adjusted_time =
                        master->value +
                        scale_value(master->scale, &work->adjustment_offset);
                }
                work->previous_sample = sample->value;
                memset(work->errors, 0, sizeof(work->errors));
                work->average_index = 0;
                work->history_resets++;
            }
        } else if (work->adjust_enabled == 0) {
            work->reset_history = 1;
        }
    }
    work->maximum_time = work->maximum_time < master->value
                             ? master->value : work->maximum_time;
    predicted = current_output(work, master, sample);
    if (work->reset_history == 0) {
        if (work->maximum_time < predicted) {
            if (work->adjust_enabled != 0) {
                work->previous_sample = sample->value;
                work->adjusted_time = work->maximum_time;
            } else {
                work->previous_sample = sample->value;
                work->adjusted_time = work->output_time.value;
            }
            work->adjustment_count++;
        }
    } else if (work->adjust_enabled == 1) {
        error = master->value - predicted;
        if (error < 0) error = -error;
        limit = scale_value(master->scale, &work->excess_error);
        if (error > limit) {
            work->previous_sample = sample->value;
            work->adjusted_time = master->value;
            memset(work->errors, 0, sizeof(work->errors));
            work->average_index = 0;
            work->history_resets++;
            work->excess_resets++;
        } else {
            index = work->average_index;
            work->average_index = index + 1;
            work->errors[index % work->average_count] =
                (int)(master->value - predicted);
            sum = 0;
            for (i = 0; i < work->average_count; i++) sum += work->errors[i];
            work->average = sum / work->average_count;
            work->adjusted_average = work->average;
            limit = scale_value(master->scale, &work->tolerance);
            if ((long long)(work->average < 0 ? -work->average : work->average) > limit) {
                if ((long long)work->average > limit) {
                    correction = (2LL * work->average) / limit - 1;
                    work->positive_adjustments += (int)correction;
                } else {
                    correction = (2LL * work->average) / limit + 1;
                    work->negative_adjustments -= (int)correction;
                }
                work->previous_sample = sample->value;
                correction = correction * limit / 2;
                work->adjusted_time = predicted + correction;
                for (i = 0; i < work->average_count; i++) work->errors[i] -= (int)correction;
                sum = 0;
                for (i = 0; i < work->average_count; i++) sum += work->errors[i];
                work->adjusted_average = sum / work->average_count;
            }
        }
    }
    predicted = current_output(work, master, sample);
    output->value = predicted;
    output->scale = master->scale;
    if (output->value < work->output_time.value) *output = work->output_time;
    work->input_time = *master;
    work->sample_time = *sample;
    work->output_time = *output;
    i = (int)(master->value - output->value);
    if (work->reset_history == 0) {
        work->front_max = work->front_max < i ? i : work->front_max;
        work->front_min = i < work->front_min ? i : work->front_min;
    } else {
        work->rear_max = work->rear_max < i ? i : work->rear_max;
        work->rear_min = i < work->rear_min ? i : work->rear_min;
    }
    sftst_last = work;
    if (sftst_debout_buf != 0) {
        char message[268];
        int length;
        int milliseconds = UTY_MulDiv(1000, (int)work->sample_time.value,
                                      (int)work->sample_time.scale);
        long long seconds = work->sample_time.value / work->sample_time.scale;

        length = sprintf(message, sftst_format, work, (int)seconds, milliseconds,
                         (int)(work->sample_time.value >> 32),
                         (int)work->sample_time.value,
                         (int)(work->sample_time.value & 0x7fffffff),
                         (int)work->maximum_time, (int)work->input_time.value,
                         (int)work->output_time.value,
                         (int)(work->input_time.value - work->output_time.value),
                         (int)(work->maximum_time - work->output_time.value),
                         work->front_max, work->front_min, work->rear_max,
                         work->rear_min, work->paused, work->reset_history,
                         work->history_resets, work->excess_resets,
                         work->adjustment_count, work->positive_adjustments,
                         work->negative_adjustments, work->average,
                         work->adjusted_average, sfadxt_stat);
        strcpy(sftst_debout_write, message);
        sftst_debout_write += length;
        if (sftst_debout_write >=
            sftst_debout_buf + (sftst_debout_siz - 0x400)) {
            sftst_debout_write = sftst_debout_round;
        }
    }
}

void SFTST_GoNextFrame(SFTST_Work* work, const SFTST_Time* elapsed)
{
    if (work->adjust_enabled == 0)
        work->output_time.value += scale_value(work->output_time.scale, elapsed);
}

void SFTST_SetAdjFlg(SFTST_Work* work, int value) { work->adjust_enabled = value; }
void SFTST_Pause(SFTST_Work* work, int value) { work->paused = value; }
void SFTST_SetMovaveRange(SFTST_Work* work, int value)
{
    if (value > 0) work->average_count = value;
}
void SFTST_SetAdjPoff(SFTST_Work* work, const SFTST_Time* value) { work->adjustment_offset = *value; }
void SFTST_SetAdjStart(SFTST_Work* work, const SFTST_Time* value) { work->adjustment_start = *value; }
void SFTST_SetExcessErr(SFTST_Work* work, const SFTST_Time* value) { work->excess_error = *value; }
void SFTST_SetTolerance(SFTST_Work* work, const SFTST_Time* value) { work->tolerance = *value; }
void SFTST_SetTstFlg(SFTST_Work* work, int value) { work->test_enabled = value; }

void SFTST_Create(SFTST_Work* work)
{
    memset(work, 0, sizeof(*work));
    work->test_enabled = 1;
    work->paused = 0;
    work->reset_history = 0;
    work->adjust_enabled = 1;
    work->average_count = 10;
    memset(work->errors, 0, sizeof(work->errors));
    work->average_index = 0;
    work->history_resets++;
    work->input_time.value = 0;
    work->input_time.scale = 1;
    work->sample_time.value = 0;
    work->sample_time.scale = 1;
    work->output_time.value = 0;
    work->output_time.scale = 1;
    work->tolerance.value = 0x412b;
    work->tolerance.scale = 1000000;
    work->excess_error.value = 200000;
    work->excess_error.scale = 1000000;
    work->adjustment_start.value = -0x412b;
    work->adjustment_start.scale = 1000000;
    work->adjustment_offset.value = -0x412b;
    work->adjustment_offset.scale = 1000000;
    work->previous_sample = -1;
    work->adjusted_time = 0;
    work->maximum_time = 0;
    work->adjustment_count = 0;
    work->positive_adjustments = 0;
    work->negative_adjustments = 0;
    work->history_resets = 0;
    work->excess_resets = 0;
    work->average = 0;
    work->adjusted_average = 0;
    work->front_max = 0;
    work->front_min = 0;
    work->rear_max = 0;
    work->rear_min = 0;
    {
        SFTST_Header header = sftst_header;

        if (sftst_debout_buf != 0) {
            memset(sftst_debout_buf, 0, sftst_debout_siz);
            sftst_debout_write = sftst_debout_buf;
            strcpy(sftst_debout_buf, header.text);
            sftst_debout_write += strlen(header.text);
            sftst_debout_round = sftst_debout_write;
        }
    }
}
