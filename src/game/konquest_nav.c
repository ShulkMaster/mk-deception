#include "math/mk_math.h"
#include "platform/io.h"
#include "runtime/asset.h"
#include "runtime/mk_mem.h"

union NavFloatBits {
    float value;
    unsigned int bits;
};

struct NavPortalEntry {
    int adjacentArea;
    float x;
    float z;
    float length;
    float normalX;
    float normalZ;
};
typedef char NavPortalEntrySizeCheck[
    sizeof(struct NavPortalEntry) == 0x18 ? 1 : -1];

struct NavBoundary {
    float x;
    float z;
    float offset;
};
typedef char NavBoundarySizeCheck[sizeof(struct NavBoundary) == 0x0C ? 1 : -1];

struct NavArea {
    int boundaryCount;
    struct NavBoundary boundaries[1];
};

struct NavPortalList {
    int count;
    struct NavPortalEntry entries[1];
};

struct KonquestNavData {
    int areaCount;
    struct NavArea* areas[1];
};

struct NavTile {
    char pad00[8];
    float x;
    char pad0C[4];
    float z;
    char pad14[0x20];
    int navigationAreas[70];
    int navigationCount;
};
typedef char NavTileSizeCheck[sizeof(struct NavTile) == 0x150 ? 1 : -1];

struct KonquestPdata {
    char pad00[0x15C];
    struct NavTile* navTiles;
    char pad160[0x1C];
    int navTileWidth;
    int navTileHeight;
    char pad184[0x270];
    struct KonquestNavData* navData;
    char pad3F8[4];
    int* areaPredecessors;
    int* areaQueue;
};
typedef char KonquestPdataSizeCheck[
    sizeof(struct KonquestPdata) == 0x404 ? 1 : -1];

extern struct KonquestPdata* konquest_pdata;
extern float __float_max[];

static const char konquestNavStrings[] =
    "NAV\0No navigation data for this konquest level";

int get_tile_from_position(const Vec* position);

static void setup_per_tile_navigations(void);

static inline struct NavArea* nav_get_area(struct KonquestNavData* nav, int areaIndex) {
    int areaCount = nav->areaCount;

    if (areaIndex < 0 || areaIndex >= areaCount) {
        return 0;
    }
    return nav->areas[areaIndex];
}

static inline struct NavPortalList* nav_get_portals(struct NavArea* area) {
    struct NavBoundary* boundaries = area->boundaries;
    int boundaryCount = area->boundaryCount;
    return (struct NavPortalList*)(boundaries + boundaryCount);
}

static inline float nav_inverse_sqrt(float lengthSquared) {
    float inverseLength;

    if (lengthSquared <= 0.0f) {
        inverseLength = 0.0f;
    } else {
        union NavFloatBits estimateBits;
        union NavFloatBits value;
        float estimate;
        float product;
        float correction;

        value.value = lengthSquared;
        estimateBits.bits = 0x5F375A00U - (value.bits >> 1);
        estimate = estimateBits.value;
        product = estimate * (lengthSquared * estimate);
        correction = 3.0f - product;
        inverseLength = 0.0625f * estimate * correction *
                        -(correction * (product * correction) - 12.0f);
    }
    return inverseLength;
}

static inline int nav_begin_area_search(int startArea) {
    struct KonquestNavData* nav = konquest_pdata->navData;
    int areaCount;
    int i;

    if (nav == 0) {
        return -1;
    }
    areaCount = nav->areaCount;
    if (konquest_pdata->areaQueue == 0 ||
        konquest_pdata->areaPredecessors == 0) {
        return -1;
    }
    if (startArea < 0 || startArea >= areaCount) {
        return -1;
    }
    for (i = 0; i < areaCount; i++) {
        konquest_pdata->areaPredecessors[i] = -1;
    }
    konquest_pdata->areaPredecessors[startArea] = startArea;
    return areaCount;
}

static inline int nav_area_contains_point(struct NavArea* area, const Vec* position) {
    struct NavBoundary* boundary = area->boundaries;
    int i;

    for (i = 0; i < area->boundaryCount; i++) {
        float distance = boundary->z * position->z;
        float boundaryX = boundary->x;
        float limit = 0.35f + boundary->offset;

        boundary++;
        if (boundaryX * position->x + distance > limit) {
            return 0;
        }
    }
    return 1;
}

static inline int nav_find_area_in_tile(const Vec* position) {
    int tileIndex = get_tile_from_position(position);
    struct NavTile* tile;
    int i;
    int areaOffset;

    if (tileIndex < 0) {
        return -1;
    }
    i = 0;
    areaOffset = 0;
    tile = &konquest_pdata->navTiles[tileIndex];
    while (i < tile->navigationCount) {
        int areaIndex = tile->navigationAreas[areaOffset];
        struct NavArea* area = nav_get_area(konquest_pdata->navData, areaIndex);

        if (nav_area_contains_point(area, position)) {
            return areaIndex;
        }
        i++;
        areaOffset++;
    }
    return -1;
}

/* TODO: [near miss] 96.14286%; boundary owner/cursor and geometry loads agree;
 * list preheader association and FP homes remain. */
void nav_get_unit_vector_to_nav_portal(Vec* out, Vec* pos, int areaIndex, int portalId) {
    struct KonquestPdata* pdata = konquest_pdata;
    struct KonquestNavData* nav = pdata->navData;
    struct NavArea* area;
    struct NavPortalList* portals;
    struct NavPortalEntry* portal;
    int areaCount = nav->areaCount;
    int remaining;

    if (pdata->areaQueue == 0 || pdata->areaPredecessors == 0) {
        return;
    }
    if (areaIndex < 0 || areaIndex >= areaCount) {
        return;
    }
    if (portalId < 0 || portalId >= areaCount) {
        return;
    }
    area = nav_get_area(nav, areaIndex);
    portals = nav_get_portals(area);
    remaining = portals->count;
    portal = portals->entries;
    while (remaining > 0) {
        if (portal->adjacentArea == portalId) {
            float dx;
            float dz;
            float normalX;
            float normalZ;
            float length;
            float halfLength;
            float quarterLength;
            float side;

            length = portal->length;
            halfLength = 0.5f * length;
            dz = portal->z;
            normalZ = portal->normalZ;
            quarterLength = 0.25f * length;
            dx = portal->x;
            normalX = portal->normalX;
            dz = normalZ * halfLength + dz;
            dx = normalX * halfLength + dx;
            dz -= pos->z;
            dx -= pos->x;
            side = normalX * dx + normalZ * dz;

            if (side >= quarterLength || side <= -0.25f * length) {
                out->x = dx;
                out->z = dz;
                normalize_xz(out);
                out->y = 0.0f;
            } else {
                out->x = normalZ;
                out->y = 0.0f;
                out->z = -normalX;
            }
            return;
        }
        portal++;
        remaining--;
    }
}

/* TODO: [near miss] 97.14050%; typed boundary-array owner improves portal-tail setup;
 * remaining BFS register/address lowering differs. */
int nav_which_area_is_next(int fromArea, int toArea) {
    int areaCount;
    struct NavPortalEntry* portal;
    int i;
    int queuedCount;
    int found;
    int result;
    int nextArea;

    found = 0;
    areaCount = nav_begin_area_search(fromArea);
    if (toArea < 0 || toArea >= areaCount) {
        return -1;
    }
    if (fromArea == toArea) {
        return fromArea;
    }

    i = 0;
    queuedCount = 0;
    konquest_pdata->areaQueue[queuedCount++] = fromArea;
    while (queuedCount != i && found == 0) {
        struct NavArea* area;
        struct NavPortalList* portals;
        int portalCount;
        int portalIndex;
        int currentArea;

        currentArea = konquest_pdata->areaQueue[i++];
        area = nav_get_area(konquest_pdata->navData, currentArea);
        portals = nav_get_portals(area);
        portalCount = portals->count;
        portal = portals->entries;
        for (portalIndex = 0; portalIndex < portalCount;
             portalIndex++, portal++) {
            nextArea = portal->adjacentArea;

            if (konquest_pdata->areaPredecessors[nextArea] == -1) {
                konquest_pdata->areaQueue[queuedCount++] = nextArea;
                konquest_pdata->areaPredecessors[nextArea] = currentArea;
            }
            if (nextArea == toArea) {
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        return -1;
    }
    do {
        result = toArea;
        toArea = konquest_pdata->areaPredecessors[toArea];
    } while (toArea != fromArea);
    return result;
}

static struct NavArea* unit_vector_to_area(struct NavArea* area, Vec* nearestNormal,
                                    float* nearestDistance,
                                    Vec* farthestNormal,
                                    float* farthestDistance,
                                    Vec* position);

void nav_get_unit_vector_to_closest_area(Vec* out, Vec* pos) {
    struct KonquestNavData* nav;
    struct NavArea* area;
    Vec nearestNormal;
    Vec farthestNormal;
    float farthestDistance;
    float nearestDistance;
    float selectedNearZ;
    float selectedNearX;
    float selectedFarZ;
    float selectedFarX;
    float selectedNearestDistance;
    float selectedFarthestDistance;
    float ratio;
    float blend;
    float lengthSquared;
    float inverseLength;
    int count;
    int areaIndex;

    selectedNearZ = 0.0f;
    nav = konquest_pdata->navData;
    selectedNearX = 0.0f;
    selectedNearestDistance = __float_max[0];
    selectedFarthestDistance = __float_max[0];
    selectedFarZ = 0.0f;
    selectedFarX = 0.0f;
    if (nav == 0) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    count = nav->areaCount;
    area = (struct NavArea*)&nav->areas[count];
    for (areaIndex = 0; areaIndex < count; areaIndex++) {
        area = unit_vector_to_area(area, &nearestNormal, &nearestDistance,
                                   &farthestNormal, &farthestDistance, pos);
        if (area == 0) {
            out->x = out->y = out->z = 0.0f;
            return;
        }
        if (farthestDistance < selectedFarthestDistance) {
            selectedFarthestDistance = farthestDistance;
            selectedNearestDistance = nearestDistance;
            selectedNearX = nearestNormal.x;
            selectedNearZ = nearestNormal.z;
            selectedFarX = farthestNormal.x;
            selectedFarZ = farthestNormal.z;
        }
    }
    ratio = selectedNearestDistance / selectedFarthestDistance;
    selectedNearZ *= ratio;
    blend = 1.0f - ratio;
    selectedNearX *= ratio;
    selectedNearZ += selectedFarZ * blend;
    selectedNearX += selectedFarX * blend;
    lengthSquared = selectedNearX * selectedNearX + selectedNearZ * selectedNearZ;
    inverseLength = nav_inverse_sqrt(lengthSquared);
    out->x = selectedNearX * inverseLength;
    out->z = selectedNearZ * inverseLength;
    out->x = -out->x;
    out->z = -out->z;
    out->y = 0.0f;
}

void nav_get_unit_vector_to_area(int areaIndex, Vec* out, Vec* pos) {
    struct KonquestNavData* nav;
    struct NavArea* area;
    float nearestDistance;
    float farthestDistance;
    Vec farthestNormal;
    Vec nearestNormal;
    float ratio;
    float inverseLength;
    float lengthSquared;

    nav = konquest_pdata->navData;
    area = nav_get_area(nav, areaIndex);
    if (area == 0) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    if (unit_vector_to_area(area, &nearestNormal, &nearestDistance,
                            &farthestNormal, &farthestDistance, pos) == 0) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    ratio = nearestDistance / farthestDistance;
    nearestNormal.x *= ratio;
    nearestNormal.z *= ratio;
    nearestNormal.x += farthestNormal.x * (1.0f - ratio);
    nearestNormal.z += farthestNormal.z * (1.0f - ratio);
    lengthSquared = nearestNormal.x * nearestNormal.x +
                    nearestNormal.z * nearestNormal.z;
    inverseLength = nav_inverse_sqrt(lengthSquared);
    out->x = nearestNormal.x * inverseLength;
    out->z = nearestNormal.z * inverseLength;
    out->x = -out->x;
    out->z = -out->z;
    out->y = 0.0f;
}

/* TODO: [near miss] 91.67%; FPR coloring plus a joined return where retail splits the returns. */
static struct NavArea* unit_vector_to_area(struct NavArea* area, Vec* nearestNormal,
                                    float* nearestDistance,
                                    Vec* farthestNormal,
                                    float* farthestDistance,
                                    Vec* position) {
    int inside;
    struct NavBoundary* boundary;
    int i;
    int count;

    boundary = area->boundaries;
    inside = 1;
    *farthestDistance = 0.0f;
    *nearestDistance = __float_max[0];
    nearestNormal->x = nearestNormal->y = nearestNormal->z = 0.0f;
    farthestNormal->x = farthestNormal->y = farthestNormal->z = 0.0f;
    count = area->boundaryCount;
    for (i = 0; i < count; i++) {
        float boundaryZ = boundary->z;
        float distance = boundaryZ * position->z;
        float boundaryX = boundary->x;
        float limit = 0.35f + boundary->offset;

        boundary++;
        distance = boundaryX * position->x + distance - limit;

        if (distance > 0.0f) {
            inside = 0;
            if (distance > *farthestDistance) {
                farthestNormal->x = boundaryX;
                farthestNormal->z = boundaryZ;
                *farthestDistance = distance;
            }
            if (distance < *nearestDistance) {
                nearestNormal->x = boundaryX;
                nearestNormal->z = boundaryZ;
                *nearestDistance = distance;
            }
        }
    }
    count = ((struct NavPortalList*)boundary)->count;
    boundary = (struct NavBoundary*)((struct NavPortalList*)boundary)->entries;
    boundary = (struct NavBoundary*)((struct NavPortalEntry*)boundary + count);
    if (inside != 0) {
        return 0;
    }
    return (struct NavArea*)boundary;
}

/* TODO: [near miss] 97.07229%; typed boundary-array owner improves portal-tail setup;
 * area-count reload, boundary-test FP homes and portal induction remain. */
int nav_what_area_is_point_in(Vec* pos, int hintArea) {
    struct KonquestNavData* nav;
    struct NavArea* area;
    struct NavPortalList* portals;
    struct NavPortalEntry* portal;
    int areaCount;
    int portalIndex;

    nav = konquest_pdata->navData;
    if (nav == 0) {
        return -1;
    }
    if (hintArea < 0) {
        return nav_find_area_in_tile(pos);
    }
    area = nav_get_area(nav, hintArea);
    if (nav_area_contains_point(area, pos)) {
        return hintArea;
    }
    areaCount = nav_begin_area_search(hintArea);
    if (areaCount < 0) {
        return -1;
    }
    portals = nav_get_portals(area);
    portal = portals->entries;
    for (portalIndex = 0; portalIndex < portals->count;
         portalIndex++, portal++) {
        int adjacentAreaIndex = portal->adjacentArea;
        struct NavArea* adjacentArea = nav_get_area(konquest_pdata->navData,
                                             adjacentAreaIndex);

        if (nav_area_contains_point(adjacentArea, pos)) {
            return adjacentAreaIndex;
        }
    }
    return nav_find_area_in_tile(pos);
}

void konquest_nav_init(void) {
    unsigned int artId;
    struct KonquestNavData* nav;
    int allocationSize;

    artId = get_artid_of_named_item_in_slot(0x60029, konquestNavStrings, 0);
    if (artId != 0) {
        nav = get_nav_data(0x60029, artId);
        if (nav != 0) {
            konquest_pdata->navData = nav;
            allocationSize = nav->areaCount * (int)sizeof(int);
            konquest_pdata->areaQueue = get_mem(allocationSize);
            if (konquest_pdata->areaQueue != 0) {
                konquest_pdata->areaPredecessors = get_mem(allocationSize);
                if (konquest_pdata->areaPredecessors != 0) {
                    setup_per_tile_navigations();
                }
            }
        } else {
            debug_print_message(&konquestNavStrings[4]);
        }
    }
}

/* TODO: [near miss] 95.81%; shared exits/counting agree; bound copies and induction/FP/GPR setup differ. */
static void setup_per_tile_navigations(void) {
    static int most_navigation_per_tile;
    Vec intersections[15];
    Vec tileCorners[4];
    int cornerOutside[4];
    Vec current;
    Vec first;
    Vec previous;
    int areaCount;
    int areaIndex;
    int boundaryCount;
    int tileCount;
    int tileIndex;

    areaIndex = 0;
    tileCount = konquest_pdata->navTileWidth * konquest_pdata->navTileHeight;
    areaCount = konquest_pdata->navData->areaCount;
    for (; areaIndex < areaCount; areaIndex++) {
        struct NavArea* area = nav_get_area(konquest_pdata->navData, areaIndex);

        boundaryCount = area->boundaryCount;

        if (boundaryCount <= 15) {
            struct NavBoundary* boundary = area->boundaries;
            float firstOffset;
            float previousOffset;
            int boundaryIndex;

            previous.x = boundary->x;
            previous.y = 0.0f;
            previous.z = boundary->z;
            previousOffset = boundary->offset;
            first.x = previous.x;
            first.y = previous.y;
            first.z = previous.z;
            firstOffset = previousOffset;
            boundary++;
            for (boundaryIndex = 1; boundaryIndex < boundaryCount;
                 boundaryIndex++) {
                float currentOffset;
                Vec* intersection;

                current.x = boundary->x;
                current.y = 0.0f;
                current.z = boundary->z;
                currentOffset = boundary->offset;
                boundary++;
                intersection = intersections;
                intersection += boundaryIndex;
                intersect_xz_lines(&previous, &current,
                                   intersection,
                                   previousOffset, currentOffset);
                previous.x = current.x;
                previous.y = current.y;
                previous.z = current.z;
                previousOffset = currentOffset;
            }
            intersect_xz_lines(&previous, &first, &intersections[0],
                               previousOffset, firstOffset);
        }

        tileIndex = 0;
        while (tileIndex < tileCount) {
            struct NavTile* tile = &konquest_pdata->navTiles[tileIndex];
            float minX = tile->x - 30.6f;
            float maxX = tile->x + 30.6f;
            float minZ = tile->z - 30.6f;
            float maxZ = tile->z + 30.6f;
            int allVerticesInsideTile = 1;
            int allLeft = 1;
            int allRight = 1;
            int allBelow = 1;
            int allAbove = 1;
            int allTileCornersInsideArea = 1;
            int boundaryIndex;
            int vertexIndex;
            struct NavBoundary* boundary;

            tileCorners[0].x = minX;
            tileCorners[0].y = 0.0f;
            tileCorners[0].z = minZ;
            tileCorners[1].x = minX;
            tileCorners[1].y = 0.0f;
            tileCorners[1].z = maxZ;
            tileCorners[2].x = maxX;
            tileCorners[2].y = 0.0f;
            tileCorners[2].z = maxZ;
            tileCorners[3].x = maxX;
            tileCorners[3].y = 0.0f;
            tileCorners[3].z = minZ;

            for (vertexIndex = 0; vertexIndex < boundaryCount; vertexIndex++) {
                if (intersections[vertexIndex].x < minX) {
                    allRight = 0;
                    allVerticesInsideTile = 0;
                } else if (intersections[vertexIndex].x > maxX) {
                    allLeft = 0;
                    allVerticesInsideTile = 0;
                } else {
                    allLeft = 0;
                    allRight = 0;
                }
                if (intersections[vertexIndex].z < minZ) {
                    allAbove = 0;
                    allVerticesInsideTile = 0;
                } else if (intersections[vertexIndex].z > maxZ) {
                    allBelow = 0;
                    allVerticesInsideTile = 0;
                } else {
                    allAbove = 0;
                    allBelow = 0;
                }
            }

            if (!(allLeft || allRight || allBelow || allAbove)) {
                if (allVerticesInsideTile) {
                    if (tile->navigationCount < 70) {
                        tile->navigationAreas[tile->navigationCount] = areaIndex;
                        tile->navigationCount++;
                        if (tile->navigationCount > most_navigation_per_tile) {
                            most_navigation_per_tile = tile->navigationCount;
                        }
                    }
                } else {
                    boundary = area->boundaries;
                    for (boundaryIndex = 0; boundaryIndex < boundaryCount;
                         boundaryIndex++) {
                        float boundaryX = boundary->x;
                        float boundaryZ = boundary->z;
                        float boundaryOffset = boundary->offset;
                        int allCornersOutside = 1;
                        int allCornersInside = 1;
                        int tileCorner;

                        boundary++;
                        for (tileCorner = 0; tileCorner < 4; tileCorner++) {
                            float planeDistance = boundaryX * tileCorners[tileCorner].x +
                                                  boundaryZ * tileCorners[tileCorner].z;

                            if (planeDistance > boundaryOffset) {
                                cornerOutside[tileCorner] = 1;
                                allCornersInside = 0;
                                allTileCornersInsideArea = 0;
                            } else {
                                cornerOutside[tileCorner] = 0;
                                allCornersOutside = 0;
                            }
                        }
                        if (allCornersOutside) {
                            goto next_tile;
                        }
                        if (!allCornersInside) {
                            int previousCorner = 3;
                            int cornerIndex;

                            for (cornerIndex = 0; cornerIndex < 4;
                                 cornerIndex++) {
                                if (cornerOutside[previousCorner] !=
                                    cornerOutside[cornerIndex]) {
                                    float threshold;
                                    float currentComponent;
                                    float nextComponent;
                                    int crosses;

                                    if (cornerIndex == 0) {
                                        threshold = minZ;
                                        currentComponent =
                                            intersections[boundaryIndex].z;
                                        nextComponent =
                                            intersections[(boundaryIndex + 1) %
                                                          boundaryCount].z;
                                    } else if (cornerIndex == 1) {
                                        threshold = minX;
                                        currentComponent =
                                            intersections[boundaryIndex].x;
                                        nextComponent =
                                            intersections[(boundaryIndex + 1) %
                                                          boundaryCount].x;
                                    } else if (cornerIndex == 2) {
                                        threshold = maxZ;
                                        currentComponent =
                                            intersections[boundaryIndex].z;
                                        nextComponent =
                                            intersections[(boundaryIndex + 1) %
                                                          boundaryCount].z;
                                    } else {
                                        threshold = maxX;
                                        currentComponent =
                                            intersections[boundaryIndex].x;
                                        nextComponent =
                                            intersections[(boundaryIndex + 1) %
                                                          boundaryCount].x;
                                    }
                                    crosses = 0;
                                    if (currentComponent > threshold) {
                                        crosses = 1;
                                    }
                                    if (nextComponent > threshold) {
                                        crosses ^= 1;
                                    }
                                    if (crosses) {
                                        if (tile->navigationCount < 70) {
                                            tile->navigationAreas
                                                [tile->navigationCount] =
                                                areaIndex;
                                            tile->navigationCount++;
                                            if (tile->navigationCount >
                                                most_navigation_per_tile) {
                                                most_navigation_per_tile =
                                                    tile->navigationCount;
                                            }
                                        }
                                        goto next_tile;
                                    }
                                }
                                previousCorner = cornerIndex;
                            }
                        }
                    }

                    if (allTileCornersInsideArea) {
                        if (tile->navigationCount < 70) {
                            tile->navigationAreas[tile->navigationCount] =
                                areaIndex;
                            tile->navigationCount++;
                            if (tile->navigationCount >
                                most_navigation_per_tile) {
                                most_navigation_per_tile =
                                    tile->navigationCount;
                            }
                        }
                    }
                }
            }

        next_tile:
            tileIndex++;
        }
    }
}
