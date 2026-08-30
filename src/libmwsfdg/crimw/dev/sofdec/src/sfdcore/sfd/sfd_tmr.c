#include "sofdec/sfd_timer.h"

void SFTMR_AddTsum(SfdTimerSummary* summary, long long elapsed) {
    long long minimum;
    long long maximum;
    summary->total += elapsed;
    minimum = summary->minimum;
    if (elapsed < minimum) {
        minimum = elapsed;
    }
    summary->minimum = minimum;
    maximum = summary->maximum;
    elapsed = maximum < elapsed ? elapsed : maximum;
    summary->maximum = elapsed;
    summary->count++;
}

void SFTMR_InitTsum(SfdTimerSummary* summary) {
    summary->total = 0;
    summary->minimum = 0x7FFFFFFFFFFFFFFFLL;
    summary->maximum = 0;
    summary->count = 0;
}
