typedef struct KonquestTime {
    int year;         /* +0x00 */
    int month;        /* +0x04, zero based */
    int day_of_month; /* +0x08, zero based */
    int day_of_week;  /* +0x0C */
    int hour;         /* +0x10 */
    int minute;       /* +0x14 */
} KonquestTime;

typedef struct KonquestEvent KonquestEvent;
typedef struct KonquestNpc KonquestNpc;

int is_valid_event_time(const KonquestTime* time) {
    unsigned int specified;

    specified = 0;
    if (time->year != -1) {
        specified |= 1;
    }
    if (time->month != -1) {
        specified |= 2;
    }
    if (time->day_of_month != -1) {
        specified |= 4;
    }
    if (time->day_of_week != -1) {
        specified |= 8;
    }

    if (specified == 0xF) {
        return 0;
    }
    return time->hour != -1;
}

static KonquestEvent* which_event_is_more_recent(
    KonquestNpc* npc, KonquestEvent* event_a, KonquestEvent* event_b);

KonquestEvent* npc_which_event_is_more_recent(
    KonquestNpc* npc, KonquestEvent* event_a, KonquestEvent* event_b) {
    if (which_event_is_more_recent(npc, event_a, event_b) == event_a) {
        return event_a;
    }
    return event_b;
}

int is_time_a_equal_to_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b) {
    if (time_a->year != time_b->year) {
        return 0;
    }
    if (time_a->month != time_b->month) {
        return 0;
    }
    if (time_a->day_of_month != time_b->day_of_month) {
        return 0;
    }
    if (time_a->hour != time_b->hour) {
        return 0;
    }
    return time_a->minute == time_b->minute;
}

/* Matching: 99.318184% - equivalent GPR load ordering remains. */
int is_time_a_greater_than_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b) {
    if (time_a->year > time_b->year) {
        return 1;
    }
    if (time_a->year < time_b->year) {
        return 0;
    }
    if (time_a->month > time_b->month) {
        return 1;
    }
    if (time_a->month < time_b->month) {
        return 0;
    }
    if (time_a->day_of_month > time_b->day_of_month) {
        return 1;
    }
    if (time_a->day_of_month < time_b->day_of_month) {
        return 0;
    }
    if (time_a->hour > time_b->hour) {
        return 1;
    }
    if (time_a->hour < time_b->hour) {
        return 0;
    }
    return time_b->minute < time_a->minute;
}

static int find_month_in_a_year(
    int month, int year, const KonquestTime* current, KonquestTime* result) {
    int months_to_add;

    *result = *current;
    if (year < result->year) {
        return 0;
    }

    if (year == result->year) {
        *result = *current;
        if (current->month != month) {
            months_to_add = month - result->month;
            if (months_to_add < 0) {
                months_to_add += 12;
            }
            result->month += months_to_add;
            result->day_of_week =
                (result->day_of_week + months_to_add * 2) % 7;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
            result->day_of_week =
                (result->year * 3 + result->month * 2) % 7;
            result->day_of_month = 0;
        }
        if (result->year != year) {
            return 0;
        }
    } else {
        result->year = year;
        result->month = month;
        result->day_of_week = (year * 3 + month * 2) % 7;
        result->day_of_month = 0;
    }
    return 1;
}

/*
 * Matching: 94.71368% - stack-frame and temporary GPR allocation remain
 * in the first-day construction branch.
 */
static int find_next_day_of_week_and_month_in_a_year(
    int day_of_week, int month, int year,
    const KonquestTime* current, KonquestTime* result) {
    int amount;
    int days_to_add;

    *result = *current;
    if (year < result->year) {
        return 0;
    }

    if (year == result->year) {
        *result = *current;
        amount = day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        if (amount > 0) {
            days_to_add =
                ((30 - result->day_of_month + (amount - 1) * 30 + 6) / 7) * 7;
            result->day_of_month += days_to_add;
            result->day_of_week =
                (result->day_of_week + days_to_add) % 7;
            if (result->day_of_month >= 30) {
                result->month += result->day_of_month / 30;
                result->day_of_month %= 30;
                if (result->month >= 12) {
                    result->year += result->month / 12;
                    result->month %= 12;
                }
            }
        }
        if (result->year != year) {
            return 0;
        }
    } else {
        KonquestTime first_day;

        first_day.year = year;
        first_day.month = 0;
        first_day.day_of_month = 0;
        first_day.day_of_week = (year * 3) % 7;
        first_day.hour = current->hour;
        first_day.minute = current->minute;

        *result = first_day;

        amount = day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        if (amount > 0) {
            days_to_add =
                ((30 - result->day_of_month + (amount - 1) * 30 + 6) / 7) * 7;
            result->day_of_month += days_to_add;
            result->day_of_week =
                (result->day_of_week + days_to_add) % 7;
            if (result->day_of_month >= 30) {
                result->month += result->day_of_month / 30;
                result->day_of_month %= 30;
                if (result->month >= 12) {
                    result->year += result->month / 12;
                    result->month %= 12;
                }
            }
        }
    }
    return 1;
}

/*
 * Matching: 94.71354% - stack-frame and temporary GPR allocation remain
 * in the first-day construction branch.
 */
static int find_next_day_of_month_and_month_in_a_year(
    int day_of_month, int month, int year,
    const KonquestTime* current, KonquestTime* result) {
    int amount;

    *result = *current;
    if (year < result->year) {
        return 0;
    }

    if (year == result->year) {
        *result = *current;
        amount = day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        result->month += amount;
        result->day_of_week = (result->day_of_week + amount * 2) % 7;
        if (result->month >= 12) {
            result->year += result->month / 12;
            result->month %= 12;
        }
        if (result->year != year) {
            return 0;
        }
    } else {
        KonquestTime first_day;

        first_day.year = year;
        first_day.month = 0;
        first_day.day_of_month = 0;
        first_day.day_of_week = (year * 3) % 7;
        first_day.hour = current->hour;
        first_day.minute = current->minute;

        *result = first_day;

        amount = day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        result->month += amount;
        result->day_of_week = (result->day_of_week + amount * 2) % 7;
        if (result->month >= 12) {
            result->year += result->month / 12;
            result->month %= 12;
        }
    }
    return 1;
}

/*
 * Matching: 95.12019% - stack-frame and temporary GPR allocation remain
 * in the first-day construction branch.
 */
static int find_next_day_of_week_and_day_of_month_in_a_year(
    int day_of_week, int day_of_month, int year,
    const KonquestTime* current, KonquestTime* result) {
    int amount;
    int months_to_add;

    *result = *current;
    if (year < result->year) {
        return 0;
    }

    if (year == result->year) {
        *result = *current;
        amount = day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        if (amount & 1) {
            months_to_add = (amount + 7) / 2;
        } else {
            months_to_add = amount / 2;
        }
        result->month += months_to_add;
        result->day_of_week =
            (result->day_of_week + months_to_add * 2) % 7;
        if (result->month >= 12) {
            result->year += result->month / 12;
            result->month %= 12;
        }
        if (result->year != year) {
            return 0;
        }
    } else {
        KonquestTime first_day;

        first_day.year = year;
        first_day.month = 0;
        first_day.day_of_month = 0;
        first_day.day_of_week = (year * 3) % 7;
        first_day.hour = current->hour;
        first_day.minute = current->minute;

        *result = first_day;

        amount = day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        result->day_of_month += amount;
        result->day_of_week = (result->day_of_week + amount) % 7;
        if (result->day_of_month >= 30) {
            result->month += result->day_of_month / 30;
            result->day_of_month %= 30;
            if (result->month >= 12) {
                result->year += result->month / 12;
                result->month %= 12;
            }
        }

        amount = day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        if (amount & 1) {
            months_to_add = (amount + 7) / 2;
        } else {
            months_to_add = amount / 2;
        }
        result->month += months_to_add;
        result->day_of_week =
            (result->day_of_week + months_to_add * 2) % 7;
        if (result->month >= 12) {
            result->year += result->month / 12;
            result->month %= 12;
        }
    }
    return 1;
}

void add_minutes_to_time(KonquestTime* time, int minutes) {
    int days;

    time->minute += minutes;
    if (time->minute >= 60) {
        time->hour += time->minute / 60;
        if (time->hour >= 24) {
            days = time->hour / 24;
            time->day_of_month += days;
            time->day_of_week = (time->day_of_week + days) % 7;
            if (time->day_of_month >= 30) {
                time->month += time->day_of_month / 30;
                time->day_of_month %= 30;
                if (time->month >= 12) {
                    time->year += time->month / 12;
                    time->month %= 12;
                }
            }
            time->hour %= 24;
        }
        time->minute %= 60;
    }
}

void add_hours_to_time(KonquestTime* time, int hours) {
    int days;

    time->hour += hours;
    if (time->hour >= 24) {
        days = time->hour / 24;
        time->day_of_month += days;
        time->day_of_week = (time->day_of_week + days) % 7;
        if (time->day_of_month >= 30) {
            time->month += time->day_of_month / 30;
            time->day_of_month %= 30;
            if (time->month >= 12) {
                time->year += time->month / 12;
                time->month %= 12;
            }
        }
        time->hour %= 24;
    }
}

void add_days_to_time(KonquestTime* time, int days) {
    time->day_of_month += days;
    time->day_of_week = (time->day_of_week + days) % 7;
    if (time->day_of_month >= 30) {
        time->month += time->day_of_month / 30;
        time->day_of_month %= 30;
        if (time->month >= 12) {
            time->year += time->month / 12;
            time->month %= 12;
        }
    }
}

void increment_day(KonquestTime* time) {
    ++time->day_of_week;
    if (time->day_of_week >= 7) {
        time->day_of_week = 0;
    }
    ++time->day_of_month;
    if (time->day_of_month >= 30) {
        time->day_of_month = 0;
        ++time->month;
        if (time->month >= 12) {
            time->month = 0;
            ++time->year;
        }
    }
}

void add_months_to_time(KonquestTime* time, int months) {
    time->month += months;
    time->day_of_week = (time->day_of_week + months * 2) % 7;
    if (time->month >= 12) {
        time->year += time->month / 12;
        time->month %= 12;
    }
}

void add_years_to_time(KonquestTime* time, int years) {
    time->year += years;
    time->day_of_week = (time->day_of_week + years * 3) % 7;
}
