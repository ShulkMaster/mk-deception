#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "sofdec/sfd_player_types.h"
#include "sofdec/uty_math.h"

/* Stream-time adjustment state is embedded in SfdTimerState's +0x2F0 work block. */
typedef SfdTimerTestTime SFTST_Time;
typedef SfdTimerTestWork SFTST_Work;

int sftst_debout_siz = 0;
char* sftst_debout_buf = 0;
char* sftst_debout_round = 0;
char* sftst_debout_write = 0;
SFTST_Work* sftst_last = 0;
int gap_06_804AC8B4_bss;

extern int sfadxt_stat;

typedef struct SFTST_Header {
    char text[274];
} SFTST_Header;

static const SFTST_Header sftst_header = {{
    "tst, help_time_sec, help_time_msec, help_time_64, help_time, mt_max, master_time, out_time,  mt_ot, mtmax_ot,  diff_l_max, diff_l_min, diff_a_max, tst->diff_a_min, pastat, adjmode, resethist, excesserr, adj_limit, adj_front, adj_rear,  movave_1st, movave_2nd,  adxt_stat \n\n"
}};
static const char sftst_format[] =
    "%p, %ld, %ld, %08lX%08lX, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld \n";

static inline long long scale_value(SFTST_Time* ratio, long long value)
{
    return value * ratio->value / ratio->scale;
}

static inline void reset_history(SFTST_Work* work)
{
    memset(work->errors, 0, sizeof(work->errors));
    work->average_index = 0;
    work->history_resets++;
}

static inline int sum_history(SFTST_Work* work)
{
    int i;
    int sum;

    sum = 0;
    for (i = 0; i < work->average_count; i++) {
        sum += work->errors[i];
    }
    return sum;
}

static inline void subtract_history(SFTST_Work* work, int value)
{
    int i;

    for (i = 0; i < work->average_count; i++) {
        work->errors[i] -= value;
    }
}

/* TODO: [near miss] 98.493940%; remaining debug sprintf/64-bit register
 * coloring and BSS relocation residue have no clean-C source lever. */
void SFTST_Calc(SFTST_Work* work, SFTST_Time* master,
                SFTST_Time* sample, SFTST_Time* output)
{
    long long estimated;
    long long quotient;
    long long adjustment;
    long long absolute_difference;
    long long difference;
    long long average;
    long long absolute_average;
    long long tolerance;
    long long step;
    int index;
    int difference_narrow;
    long long predicted;
    long long excess;
    char message[0x100];
    int length;

    if (sample->scale == 1 || work->test_enabled == 0) {
        *output = *master;
        return;
    }
    master->value = work->maximum_time > master->value
                        ? work->maximum_time : master->value;
    if (work->paused == 1) {
        work->reset_history = 0;
    } else if (work->reset_history == 0) {
        if (master->value > work->maximum_time) {
            work->reset_history = 1;
            if (work->previous_sample == -1) {
                adjustment = scale_value(&work->adjustment_start,
                                         master->scale);
            } else {
                adjustment = scale_value(&work->adjustment_offset,
                                         master->scale);
            }
            work->previous_sample = sample->value;
            work->adjusted_time = master->value + adjustment;
            reset_history(work);
        } else if (work->adjust_enabled == 0) {
            work->reset_history = 1;
        }
    }
    work->maximum_time = work->maximum_time > master->value
                             ? work->maximum_time : master->value;
    if (work->previous_sample == -1) {
        estimated = 0;
    } else {
        estimated = work->adjusted_time +
                    master->scale * (sample->value - work->previous_sample) /
                        sample->scale;
    }
    if (work->reset_history == 0) {
        if (work->maximum_time < estimated) {
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
        absolute_difference =
            ((difference = master->value - estimated) < 0) ? -difference : difference;
        excess = scale_value(&work->excess_error, master->scale);
        if (excess < absolute_difference) {
            work->previous_sample = sample->value;
            work->adjusted_time = master->value;
            reset_history(work);
            work->excess_resets++;
        } else {
            index = work->average_index;
            work->average_index = index + 1;
            work->errors[index % work->average_count] =
                (int)difference;
            average = sum_history(work) / work->average_count;
            work->average = (int)average;
            work->adjusted_average = (int)average;
            tolerance = master->scale * work->tolerance.value /
                        work->tolerance.scale;
            absolute_average = average < 0 ? -average : average;
            if (tolerance < absolute_average) {
                if (tolerance < average) {
                    quotient = average * 2 / tolerance - 1;
                    work->positive_adjustments += (int)quotient;
                } else {
                    quotient = average * 2 / tolerance + 1;
                    work->negative_adjustments += (int)-quotient;
                }
                adjustment = quotient;
                step = adjustment * tolerance / 2;
                work->previous_sample = sample->value;
                work->adjusted_time = estimated + step;
                subtract_history(work, (int)step);
                work->adjusted_average = sum_history(work) /
                                          work->average_count;
            }
        }
    }
    if (work->previous_sample == -1) {
        predicted = 0;
    } else {
        predicted = work->adjusted_time +
                    master->scale * (sample->value - work->previous_sample) /
                        sample->scale;
    }
    output->value = predicted;
    output->scale = master->scale;
    if (output->value < work->output_time.value) *output = work->output_time;
    work->input_time = *master;
    work->sample_time = *sample;
    work->output_time = *output;
    difference_narrow = (int)(master->value - output->value);
    if (work->reset_history == 0) {
        work->front_max = work->front_max > difference_narrow
                              ? work->front_max : difference_narrow;
        work->front_min = work->front_min < difference_narrow
                              ? work->front_min : difference_narrow;
    } else {
        work->rear_max = work->rear_max > difference_narrow
                             ? work->rear_max : difference_narrow;
        work->rear_min = work->rear_min < difference_narrow
                             ? work->rear_min : difference_narrow;
    }
    sftst_last = work;
    if (sftst_debout_buf != 0) {
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

void SFTST_GoNextFrame(SFTST_Work* work, SFTST_Time* elapsed)
{
    if (work->adjust_enabled == 0)
        work->output_time.value += scale_value(elapsed, work->output_time.scale);
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
