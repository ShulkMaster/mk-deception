#ifndef GAME_KONQUEST_TIME_H
#define GAME_KONQUEST_TIME_H

typedef struct KonquestTime {
    int year;
    int month;
    int day_of_month;
    int day_of_week;
    int hour;
    int minute;
} KonquestTime;

typedef struct KonquestTimedEvent {
    KonquestTime time;
    unsigned int script_function;
    unsigned int event_slot_3_script;
    void* path;
    int path_id;
} KonquestTimedEvent;

typedef char KonquestTimeSizeCheck[sizeof(KonquestTime) == 0x18 ? 1 : -1];
typedef char KonquestTimedEventSizeCheck[sizeof(KonquestTimedEvent) == 0x28 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

int is_valid_event_time(const KonquestTime* time);
KonquestTimedEvent* npc_which_event_is_more_recent(
    const KonquestTime* current, KonquestTimedEvent* event_a,
    KonquestTimedEvent* event_b);
int does_event_a_trump_event_b(
    const KonquestTimedEvent* event_a, const KonquestTimedEvent* event_b);
int is_time_a_equal_to_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b);
int is_time_a_greater_than_time_b(
    const KonquestTime* time_a, const KonquestTime* time_b);
int calc_next_occurrence_of_event(
    KonquestTime* result, KonquestTime* event_time,
    const KonquestTime* current);
void add_minutes_to_time(KonquestTime* time, int minutes);
void add_hours_to_time(KonquestTime* time, int hours);
void add_days_to_time(KonquestTime* time, int days);
void increment_day(KonquestTime* time);
void add_months_to_time(KonquestTime* time, int months);
void add_years_to_time(KonquestTime* time, int years);

#ifdef __cplusplus
}
#endif

#endif
