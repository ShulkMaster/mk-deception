#ifndef GAME_KONQUEST_OBJECTS_H
#define GAME_KONQUEST_OBJECTS_H

#include "game/konquest.h"

typedef struct KonquestChildObject KonquestChildObject;
struct KonquestChildObject {
    MkHdr hdr;
    struct KonquestUidObject* owner;
    int matrix_index;
    union {
        struct KonquestChildDefinition* definition;
        struct KonquestSobjBinding* binding;
        struct KonquestSobjInfo* info;
    };
    Vec position;
    Vec angles;
    Vec base_position;
    Vec base_angles;
    int state;
    MkHdr* state_object;
    unsigned int state_object_instance;
    KonquestWaypoint* path_waypoints;
};

void konquest_open_door_sobj(KonquestChildObject* door, int remain_open);

#endif
