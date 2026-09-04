typedef struct KonquestTime {
    int year;         /* +0x00 */
    int month;        /* +0x04, zero based */
    int day_of_month; /* +0x08, zero based */
    int day_of_week;  /* +0x0C */
    int hour;         /* +0x10 */
    int minute;       /* +0x14 */
} KonquestTime;

typedef struct KonquestTimedEvent {
    KonquestTime time;               /* +0x00 */
    unsigned int script_function;    /* +0x18 */
    unsigned int event_slot_3_script; /* +0x1C */
    void* path;                      /* +0x20 */
    int path_id;                     /* +0x24 */
} KonquestTimedEvent;

/* Soft ceiling: exact size and operations; specificity-mask load/GPR schedule. */
int is_valid_event_time(const KonquestTime* time) {
    int month;
    int day_of_week;
    int day_of_month;
    int has_year;
    unsigned int specified;

    month = time->month;
    day_of_week = time->day_of_week;
    day_of_month = time->day_of_month;
    has_year = time->year != -1;
    specified = has_year |
                (month == -1 ? 0 : 2) |
                (day_of_week == -1 ? 0 : 4) |
                (day_of_month == -1 ? 0 : 8);

    if (specified == 0xF) {
        return 0;
    }
    return time->hour != -1;
}

static KonquestTimedEvent* which_event_is_more_recent(
    const KonquestTime* current, KonquestTimedEvent* event_a,
    KonquestTimedEvent* event_b);
static int find_next_day_of_month_in_a_year(
    int day_of_month, int year, const KonquestTime* current,
    KonquestTime* result);
static int find_month_in_a_year(
    int month, int year, const KonquestTime* current, KonquestTime* result);
static int find_next_day_of_week_and_month_in_a_year(
    int day_of_week, int month, int year, const KonquestTime* current,
    KonquestTime* result);
static int find_next_day_of_month_and_month_in_a_year(
    int day_of_month, int month, int year, const KonquestTime* current,
    KonquestTime* result);
static int find_next_day_of_week_and_day_of_month_in_a_year(
    int day_of_week, int day_of_month, int year,
    const KonquestTime* current, KonquestTime* result);

KonquestTimedEvent* npc_which_event_is_more_recent(
    const KonquestTime* current, KonquestTimedEvent* event_a,
    KonquestTimedEvent* event_b) {
    if (which_event_is_more_recent(current, event_a, event_b) == event_a) {
        return event_a;
    }
    return event_b;
}

static inline unsigned int event_time_specificity(const KonquestTime* time) {
    int has_year = time->year != -1;

    return has_year | (time->month == -1 ? 0 : 2) |
           (time->day_of_week == -1 ? 0 : 4) |
           (time->day_of_month == -1 ? 0 : 8);
}

/* Soft ceiling: exact CFG/size; specificity-mask GPR allocation and schedule. */
static KonquestTimedEvent* which_event_is_more_recent(
    const KonquestTime* current, KonquestTimedEvent* event_a,
    KonquestTimedEvent* event_b) {
    int delta_a;
    int delta_b;
    int month_a;
    int day_of_week_a;
    int day_of_month_a;
    int month_b;
    int day_of_week_b;
    int day_of_month_b;
    unsigned int specificity_a;
    unsigned int specificity_b;
    int a_more_recent;

    if (event_a == 0) {
        return event_b;
    }
    if (event_b == 0) {
        return event_a;
    }

    delta_a = current->hour - event_a->time.hour;
    if (delta_a < 0) {
        delta_a += 24;
    } else if (delta_a == 0 && current->minute < event_a->time.minute) {
        delta_a += 24;
    }

    delta_b = current->hour - event_b->time.hour;
    if (delta_b < 0) {
        delta_b += 24;
    } else if (delta_b == 0 && current->minute < event_b->time.minute) {
        delta_b += 24;
    }

    if (delta_a < delta_b) {
        return event_a;
    }
    if (delta_b < delta_a) {
        return event_b;
    }
    if (event_a->time.minute > event_b->time.minute) {
        return event_a;
    }
    if (event_b->time.minute > event_a->time.minute) {
        return event_b;
    }
    month_a = event_a->time.month;
    day_of_week_a = event_a->time.day_of_week;
    day_of_month_a = event_a->time.day_of_month;
    month_b = event_b->time.month;
    day_of_week_b = event_b->time.day_of_week;
    day_of_month_b = event_b->time.day_of_month;
    specificity_a = (event_a->time.year != -1) |
                    (month_a == -1 ? 0 : 2) |
                    (day_of_week_a == -1 ? 0 : 4) |
                    (day_of_month_a == -1 ? 0 : 8);
    specificity_b = (event_b->time.year != -1) |
                    (month_b == -1 ? 0 : 2) |
                    (day_of_week_b == -1 ? 0 : 4) |
                    (day_of_month_b == -1 ? 0 : 8);
    if (specificity_a > specificity_b) {
        a_more_recent = 1;
    } else if (specificity_b > specificity_a) {
        a_more_recent = 0;
    } else {
        a_more_recent = 1;
    }
    if (a_more_recent != 0) {
        return event_a;
    }
    return event_b;
}

/*
 * Soft ceiling: does_event_a_trump_event_b ~57.88% at the exact 236-byte
 * retail size. Both specificity masks have the same field loads,
 * booleanization, combination, unsigned compare, and returns; the low fuzzy
 * score is a whole-function GPR/scheduling cascade. A bounded 12,650-variant
 * permutation search improved scores only with artificial wrappers or dead
 * branches, which were rejected.
 */
int does_event_a_trump_event_b(
    const KonquestTimedEvent* event_a, const KonquestTimedEvent* event_b) {
    int month_a;
    int day_of_week_a;
    int day_of_month_a;
    int month_b;
    int day_of_week_b;
    int day_of_month_b;
    unsigned int specificity_a;
    unsigned int specificity_b;

    month_a = event_a->time.month;
    day_of_week_a = event_a->time.day_of_week;
    day_of_month_a = event_a->time.day_of_month;
    month_b = event_b->time.month;
    day_of_week_b = event_b->time.day_of_week;
    day_of_month_b = event_b->time.day_of_month;
    specificity_a = (event_a->time.year != -1) |
                    (month_a == -1 ? 0 : 2) |
                    (day_of_week_a == -1 ? 0 : 4) |
                    (day_of_month_a == -1 ? 0 : 8);
    specificity_b = (event_b->time.year != -1) |
                    (month_b == -1 ? 0 : 2) |
                    (day_of_week_b == -1 ? 0 : 4) |
                    (day_of_month_b == -1 ? 0 : 8);
    if (specificity_a > specificity_b) {
        return 1;
    }
    return specificity_b <= specificity_a;
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

/* Soft ceiling: equivalent final-minute GPR load ordering remains. */
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

static inline void advance_days(KonquestTime* time, int days) {
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

static inline void advance_months(KonquestTime* time, int months) {
    time->month += months;
    time->day_of_week = (time->day_of_week + months * 2) % 7;
    if (time->month >= 12) {
        time->year += time->month / 12;
        time->month %= 12;
    }
}

/*
 * Soft ceiling: complete 16-case algorithm; eight-byte emission delta plus
 * register/scheduling residue and one equivalent CR-setting subtract.
 */
int calc_next_occurrence_of_event(
    KonquestTime* result, const KonquestTime* event_time,
    const KonquestTime* current) {
    KonquestTime next;
    unsigned int specified;
    int amount;
    int years_to_add;
    int event_is_later;
    int success;

    if (result == 0 || current == 0 || event_time == 0) {
        return 0;
    }

    next = *current;
    if (event_time->hour > next.hour) {
        event_is_later = 1;
    } else if (event_time->hour == next.hour &&
               event_time->minute > next.minute) {
        event_is_later = 1;
    } else {
        event_is_later = 0;
    }
    if (event_is_later == 0) {
        ++next.day_of_week;
        if (next.day_of_week >= 7) {
            next.day_of_week = 0;
        }
        ++next.day_of_month;
        if (next.day_of_month >= 30) {
            next.day_of_month = 0;
            ++next.month;
            if (next.month >= 12) {
                next.month = 0;
                ++next.year;
            }
        }
    }
    next.hour = event_time->hour;
    next.minute = event_time->minute;

    specified = event_time_specificity(event_time);

    switch (specified) {
    case 0:
        *result = next;
        return 1;
    case 1:
        *result = next;
        if (event_time->year < result->year) {
            success = 0;
        } else if (event_time->year == result->year) {
            success = 1;
        } else {
            result->year = event_time->year;
            result->month = 0;
            result->day_of_week = (event_time->year * 3) % 7;
            result->day_of_month = 0;
            success = 1;
        }
        return success != 0;
    case 2:
        *result = next;
        if (next.month != event_time->month) {
            amount = event_time->month - result->month;
            if (amount < 0) {
                amount += 12;
            }
            advance_months(result, amount);
            result->day_of_week =
                (result->year * 3 + result->month * 2) % 7;
            result->day_of_month = 0;
        }
        return 1;
    case 3:
        return find_month_in_a_year(
                   event_time->month, event_time->year, &next, result) != 0;
    case 4:
        *result = next;
        amount = event_time->day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        advance_days(result, amount);
        return 1;
    case 5:
        *result = next;
        if (event_time->year < result->year) {
            return 0;
        } else if (event_time->year == result->year) {
            amount = event_time->day_of_week - result->day_of_week;
            if (amount < 0) {
                amount += 7;
            }
            advance_days(result, amount);
            if (result->year != event_time->year) {
                return 0;
            }
        } else {
            result->year = event_time->year;
            result->month = 0;
            result->day_of_month = 0;
            result->day_of_week = (event_time->year * 3) % 7;
            amount = event_time->day_of_week - result->day_of_week;
            if (amount < 0) {
                amount += 7;
            }
            advance_days(result, amount);
        }
        return 1;
    case 6:
        *result = next;
        amount = event_time->day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        advance_days(result, amount);
        amount = event_time->month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        if (amount > 0) {
            amount =
                ((30 - result->day_of_month + (amount - 1) * 30 + 6) / 7) *
                7;
            advance_days(result, amount);
        }
        return 1;
    case 7:
        return find_next_day_of_week_and_month_in_a_year(
                   event_time->day_of_week, event_time->month,
                   event_time->year, &next, result) != 0;
    case 8:
        *result = next;
        amount = event_time->day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        advance_days(result, amount);
        return 1;
    case 9:
        return find_next_day_of_month_in_a_year(
                   event_time->day_of_month, event_time->year, &next,
                   result) != 0;
    case 10:
        *result = next;
        amount = event_time->day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        advance_days(result, amount);
        amount = event_time->month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        advance_months(result, amount);
        return 1;
    case 11:
        return find_next_day_of_month_and_month_in_a_year(
                   event_time->day_of_month, event_time->month,
                   event_time->year, &next, result) != 0;
    case 12:
        *result = next;
        amount = event_time->day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        advance_days(result, amount);
        amount = event_time->day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        if (amount & 1) {
            amount = (amount + 7) / 2;
        } else {
            amount /= 2;
        }
        advance_months(result, amount);
        return 1;
    case 13:
        return find_next_day_of_week_and_day_of_month_in_a_year(
                   event_time->day_of_week, event_time->day_of_month,
                   event_time->year, &next, result) != 0;
    case 14:
        *result = next;
        amount = event_time->day_of_month - result->day_of_month;
        if (amount < 0) {
            amount += 30;
        }
        advance_days(result, amount);
        amount = event_time->month - result->month;
        if (amount < 0) {
            amount += 12;
        }
        advance_months(result, amount);
        amount = event_time->day_of_week - result->day_of_week;
        if (amount < 0) {
            amount += 7;
        }
        years_to_add = amount / 3;
        if (amount % 3 == 2) {
            years_to_add = (amount + 7) / 3;
        } else if (amount % 3 == 1) {
            years_to_add = (amount + 14) / 3;
        }
        result->year += years_to_add;
        result->day_of_week =
            (result->day_of_week + years_to_add * 3) % 7;
        return 1;
    case 15:
        return 0;
    }
    return 0;
}

static int find_next_day_of_month_in_a_year(
    int day_of_month, int year, const KonquestTime* current,
    KonquestTime* result) {
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
        if (result->year != year) {
            return 0;
        }
    } else {
        result->year = year;
        result->month = 0;
        result->day_of_week = (year * 3 + day_of_month) % 7;
        result->day_of_month = day_of_month;
    }
    return 1;
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
 * Soft ceiling: stack-frame and temporary GPR allocation remain
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
 * Soft ceiling: stack-frame and temporary GPR allocation remain
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
 * Soft ceiling: stack-frame and temporary GPR allocation remain
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
