#include "runtime/mk_mem.h"

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct NavPortalEntry {
    int portalId;
    float field_04;
    float field_08;
    float field_0C;
    float field_10;
    float field_14;
} NavPortalEntry;

typedef struct NavArea {
    int portalCount;
    NavPortalEntry portals[1];
} NavArea;

typedef struct KonquestNavData {
    int areaCount;
    NavArea** areas;
} KonquestNavData;

typedef struct NavTile {
    char pad00[8];
    float x;
    char pad0C[4];
    float z;
    char pad14[0x20];
    int navigationAreas[70];
    int navigationCount;
} NavTile;

typedef struct KonquestPdata {
    char pad00[0x15C];
    NavTile* navTiles;
    char pad160[0x1C];
    int navTileWidth;
    int navTileHeight;
    char pad184[0x270];
    KonquestNavData* navData;
    int* areaVisitOrder;
    int* areaSearchQueue;
    int areaQueueWrite;
} KonquestPdata;

extern KonquestPdata* konquest_pdata;
extern float __float_max;

static const float k035 = 0.35f;
static const float k05 = 0.5f;
static const float k025 = 0.25f;
static const float kNaN = -999999.0f;
static const float kOneMinus = 0.65f;
static const float kSqrtHelper = 0.5f;

void normalize_xz(Vec3* v);
int get_tile_from_position(Vec3* pos);
void* get_nav_data(int tile);
int intersect_xz_lines(const Vec3* p, const Vec3* dir, Vec3* out, float a, float b);
int get_artid_of_named_item_in_slot(int slot, const char* name);
void debug_print_message(const char* msg, ...);

void setup_per_tile_navigations(void);

static NavArea* nav_get_area(KonquestNavData* nav, int areaIndex) {
    if (areaIndex < 0 || areaIndex >= nav->areaCount) {
        return 0;
    }
    return nav->areas[areaIndex];
}

void nav_get_unit_vector_to_nav_portal(Vec3* out, Vec3* pos, int areaIndex, int portalId) {
    KonquestPdata* pdata;
    KonquestNavData* nav;
    NavArea* area;
    NavPortalEntry* portal;
    int i;
    int count;

    pdata = konquest_pdata;
    nav = pdata->navData;
    if (nav == 0 || pdata->areaSearchQueue == 0 || areaIndex < 0 || areaIndex >= nav->areaCount ||
        portalId < 0 || portalId >= nav->areaCount) {
        return;
    }
    area = nav_get_area(nav, areaIndex);
    if (area == 0) {
        return;
    }
    portal = area->portals;
    count = area->portalCount;
    for (i = 0; i < count; i++, portal++) {
        if (portal->portalId == portalId) {
            float t = portal->field_0C;
            float dx = portal->field_08 + portal->field_10 * t - pos->x;
            float dz = portal->field_04 + portal->field_14 * t - pos->z;
            float side = portal->field_10 * dx - portal->field_14 * dz;

            if (side > k035 * t || side < k025 * t) {
                out->x = portal->field_10;
                out->y = 0.0f;
                out->z = -portal->field_14;
            } else {
                out->x = dx;
                out->z = dz;
                normalize_xz(out);
                out->y = 0.0f;
            }
            return;
        }
    }
}

int nav_which_area_is_next(int fromArea, int toArea) {
    KonquestPdata* pdata;
    KonquestNavData* nav;
    int areaCount;
    int i;
    int queueIdx;
    int found;
    int walk;

    pdata = konquest_pdata;
    nav = pdata->navData;
    if (nav == 0) {
        return -1;
    }
    areaCount = nav->areaCount;
    if (nav == 0 || pdata->areaSearchQueue == 0) {
        return -1;
    }
    if (fromArea < 0 || fromArea >= areaCount) {
        return -1;
    }
    if (toArea < 0 || toArea >= areaCount) {
        return -1;
    }
    if (fromArea == toArea) {
        return fromArea;
    }
    for (i = 0; i < areaCount; i++) {
        pdata->areaSearchQueue[i] = -1;
    }
    pdata->areaSearchQueue[fromArea] = fromArea;
    queueIdx = 1;
    found = 0;
    while (queueIdx != 0 && found == 0) {
        int curArea = pdata->areaVisitOrder[queueIdx - 1];
        NavArea* area = nav_get_area(nav, curArea);
        NavPortalEntry* portal;
        int p;

        queueIdx++;
        if (area == 0) {
            continue;
        }
        portal = area->portals;
        for (p = 0; p < area->portalCount; p++, portal++) {
            int next = portal->portalId;
            if (pdata->areaSearchQueue[next] == -1) {
                pdata->areaSearchQueue[next] = curArea;
                pdata->areaVisitOrder[queueIdx - 1] = next;
                if (next == toArea) {
                    found = 1;
                    break;
                }
            }
        }
    }
    if (found == 0) {
        return -1;
    }
    walk = toArea;
    while (pdata->areaSearchQueue[walk] != fromArea) {
        walk = pdata->areaSearchQueue[walk];
    }
    return walk;
}

static int unit_vector_to_area(NavArea* area, Vec3* pos, float* minDist, Vec3* nearPt, Vec3* farPt,
                               float* bestT) {
    NavPortalEntry* portal;
    int i;
    int count;
    int found;

    *bestT = kNaN;
    found = 1;
    count = area->portalCount;
    portal = area->portals;
    for (i = 0; i < count; i++, portal++) {
        float t = portal->field_08 * pos->z + portal->field_04;
        float side = t + portal->field_0C;

        if (side > k05) {
            if (side < *bestT) {
                nearPt->x = portal->field_04;
                nearPt->z = portal->field_08;
                *bestT = side;
            }
            if (side > *minDist) {
                farPt->x = portal->field_04;
                farPt->z = portal->field_08;
                *minDist = side;
            }
        }
    }
    if (found == 0) {
        return 0;
    }
    return 1;
}

void nav_get_unit_vector_to_closest_area(Vec3* out, Vec3* pos) {
    KonquestNavData* nav;
    float bestNear;
    float bestFar;
    float bestMin;
    float bestMid;
    float bestFarZ;
    float bestNearZ;
    float ratio;
    float invLen;
    float scale;
    int areaIndex;
    int count;

    nav = konquest_pdata->navData;
    if (nav == 0) {
        out->x = kNaN;
        out->y = kNaN;
        out->z = kNaN;
        return;
    }
    bestMin = __float_max;
    count = nav->areaCount;
    for (areaIndex = 0; areaIndex < count; areaIndex++) {
        float minDist;
        float bestT;
        Vec3 nearPt;
        Vec3 farPt;
        int ok;

        minDist = __float_max;
        bestT = __float_max;
        ok = unit_vector_to_area(nav->areas[areaIndex], pos, &minDist, &nearPt, &farPt, &bestT);
        if (ok == 0) {
            out->x = kNaN;
            out->y = kNaN;
            out->z = kNaN;
            return;
        }
        if (bestT < bestMin) {
            bestMin = bestT;
            bestNear = nearPt.x;
            bestNearZ = nearPt.z;
            bestMid = farPt.x;
            bestFar = farPt.z;
            bestFarZ = farPt.z;
        }
    }
    ratio = bestMid / bestMin;
    bestFar = bestFar * ratio;
    bestFarZ = bestFarZ * ratio;
    bestNear = bestNear + (bestMid - bestNear) * kOneMinus;
    bestNearZ = bestNearZ + (bestFarZ - bestNearZ) * kOneMinus;
    invLen = bestNear * bestNear + bestNearZ * bestNearZ;
    if (invLen >= k05) {
        scale = kSqrtHelper;
    } else {
        scale = kSqrtHelper;
    }
    out->x = -(bestNear * scale);
    out->z = -(bestNearZ * scale);
    out->y = kNaN;
}

void nav_get_unit_vector_to_area(int areaIndex, Vec3* out, Vec3* pos) {
    KonquestNavData* nav;
    NavArea* area;
    float minDist;
    float bestT;
    Vec3 nearPt;
    Vec3 farPt;
    float ratio;
    float scale;
    int ok;

    nav = konquest_pdata->navData;
    if (areaIndex < 0 || areaIndex >= nav->areaCount) {
        out->x = kNaN;
        out->y = kNaN;
        out->z = kNaN;
        return;
    }
    area = nav_get_area(nav, areaIndex);
    if (area == 0) {
        out->x = kNaN;
        out->y = kNaN;
        out->z = kNaN;
        return;
    }
    minDist = __float_max;
    bestT = __float_max;
    ok = unit_vector_to_area(area, pos, &minDist, &nearPt, &farPt, &bestT);
    if (ok == 0) {
        out->x = kNaN;
        out->y = kNaN;
        out->z = kNaN;
        return;
    }
    ratio = nearPt.x / bestT;
    nearPt.x = nearPt.x * ratio + (farPt.x - nearPt.x) * kOneMinus;
    nearPt.z = nearPt.z * ratio + (farPt.z - nearPt.z) * kOneMinus;
    scale = kSqrtHelper;
    out->x = -(nearPt.x * scale);
    out->z = -(nearPt.z * scale);
    out->y = kNaN;
}

int nav_what_area_is_point_in(Vec3* pos, int hintArea) {
    KonquestPdata* pdata;
    KonquestNavData* nav;
    int tile;
    void* tileNav;
    int areaIndex;

    pdata = konquest_pdata;
    nav = pdata->navData;
    if (nav == 0) {
        return -1;
    }
    if (hintArea < 0) {
        tile = get_tile_from_position(pos);
        if (tile < 0) {
            return -1;
        }
        tileNav = get_nav_data(tile);
        if (tileNav == 0) {
            return -1;
        }
        hintArea = ((int*)tileNav)[0];
    }
    if (hintArea < 0 || hintArea >= nav->areaCount) {
        return -1;
    }
    for (areaIndex = 0; areaIndex < nav->areaCount; areaIndex++) {
        NavArea* area = nav->areas[areaIndex];
        NavPortalEntry* portal;
        int i;

        if (area == 0) {
            continue;
        }
        portal = area->portals;
        for (i = 0; i < area->portalCount; i++, portal++) {
            float t = portal->field_08 * pos->z + portal->field_04;
            if (t > k05 && t < portal->field_0C) {
                return areaIndex;
            }
        }
    }
    return hintArea;
}

void konquest_nav_init(void) {
    KonquestPdata* pdata;
    KonquestNavData* nav;
    int areaCount;

    pdata = konquest_pdata;
    nav = pdata->navData;
    if (nav == 0) {
        return;
    }
    areaCount = nav->areaCount;
    if (pdata->areaVisitOrder == 0) {
        pdata->areaVisitOrder = (int*)get_mem(areaCount * 4);
    }
    if (pdata->areaSearchQueue == 0) {
        pdata->areaSearchQueue = (int*)get_mem(areaCount * 4);
    }
    pdata->areaQueueWrite = 0;
    setup_per_tile_navigations();
}

void setup_per_tile_navigations(void) {
    typedef struct NavBoundary {
        float x;
        float z;
        float line;
    } NavBoundary;
    typedef struct NavPolygon {
        int count;
        NavBoundary boundaries[1];
    } NavPolygon;
    Vec3 intersections[16];
    Vec3 previous;
    Vec3 first;
    KonquestNavData* nav;
    int areaIndex;
    int tileIndex;
    int tileCount;

    nav = konquest_pdata->navData;
    tileCount = konquest_pdata->navTileWidth * konquest_pdata->navTileHeight;
    for (areaIndex = 0; areaIndex < nav->areaCount; areaIndex++) {
        NavPolygon* polygon = (NavPolygon*)nav_get_area(nav, areaIndex);
        int count = polygon->count;

        if (count <= 15) {
            NavBoundary* boundary = polygon->boundaries;
            int i;

            previous.x = first.x = boundary->x;
            previous.y = first.y = 0.0f;
            previous.z = first.z = boundary->z;
            for (i = 1; i < count; i++) {
                Vec3 current;

                boundary++;
                current.x = boundary->x;
                current.y = 0.0f;
                current.z = boundary->z;
                intersect_xz_lines(&previous, &current, &intersections[i],
                                   polygon->boundaries[i - 1].line, boundary->line);
                previous = current;
            }
            intersect_xz_lines(&previous, &first, &intersections[0],
                               polygon->boundaries[count - 1].line,
                               polygon->boundaries[0].line);
        }
        for (tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            NavTile* tile = &konquest_pdata->navTiles[tileIndex];
            (void)tile;
        }
    }
}
