#include "math/mk_math.h"
#include "platform/io.h"
#include "runtime/asset.h"
#include "runtime/mk_mem.h"

typedef union NavFloatBits {
    float value;
    unsigned int bits;
} NavFloatBits;

typedef struct NavPortalEntry {
    int adjacentArea;
    float x;
    float z;
    float length;
    float normalX;
    float normalZ;
} NavPortalEntry;
typedef char NavPortalEntrySizeCheck[
    sizeof(NavPortalEntry) == 0x18 ? 1 : -1];

typedef struct NavBoundary {
    float x;
    float z;
    float offset;
} NavBoundary;
typedef char NavBoundarySizeCheck[sizeof(NavBoundary) == 0x0C ? 1 : -1];

typedef struct NavArea {
    int boundaryCount;
    NavBoundary boundaries[1];
} NavArea;

typedef struct NavPortalList {
    int count;
    NavPortalEntry entries[1];
} NavPortalList;

typedef struct KonquestNavData {
    int areaCount;
    NavArea* areas[1];
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
typedef char NavTileSizeCheck[sizeof(NavTile) == 0x150 ? 1 : -1];

typedef struct KonquestPdata {
    char pad00[0x15C];
    NavTile* navTiles;
    char pad160[0x1C];
    int navTileWidth;
    int navTileHeight;
    char pad184[0x270];
    KonquestNavData* navData;
    char pad3F8[4];
    int* areaPredecessors;
    int* areaQueue;
} KonquestPdata;
typedef char KonquestPdataSizeCheck[
    sizeof(KonquestPdata) == 0x404 ? 1 : -1];

extern KonquestPdata* konquest_pdata;
extern float __float_max[];

static const char konquestNavStrings[] =
    "NAV\0No navigation data for this konquest level";

int get_tile_from_position(const Vec* position);

static void setup_per_tile_navigations(void);

static inline NavArea* nav_get_area(KonquestNavData* nav, int areaIndex) {
    int areaCount = nav->areaCount;

    if (areaIndex < 0 || areaIndex >= areaCount) {
        return 0;
    }
    return nav->areas[areaIndex];
}

static inline NavPortalList* nav_get_portals(NavArea* area) {
    return (NavPortalList*)&area->boundaries[area->boundaryCount];
}

static inline float nav_inverse_sqrt(float lengthSquared) {
    float inverseLength;

    if (lengthSquared <= 0.0f) {
        inverseLength = 0.0f;
    } else {
        NavFloatBits estimateBits;
        NavFloatBits value;
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
    KonquestNavData* nav = konquest_pdata->navData;
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
    i = 0;
    if (areaCount > 0) {
        int remaining = areaCount;

        do {
            konquest_pdata->areaPredecessors[i] = -1;
            i++;
            remaining--;
        } while (remaining != 0);
    }
    konquest_pdata->areaPredecessors[startArea] = startArea;
    return areaCount;
}

static inline int nav_area_contains_point(NavArea* area, const Vec* position) {
    NavBoundary* boundary = area->boundaries;
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
    NavTile* tile;
    int i = 0;
    int areaOffset = 0;

    if (tileIndex < 0) {
        return -1;
    }
    tile = &konquest_pdata->navTiles[tileIndex];
    while (i < tile->navigationCount) {
        int areaIndex = tile->navigationAreas[areaOffset];
        NavArea* area = nav_get_area(konquest_pdata->navData, areaIndex);

        if (nav_area_contains_point(area, position)) {
            return areaIndex;
        }
        i++;
        areaOffset++;
    }
    return -1;
}

/*
 * Soft ceiling: nav_get_unit_vector_to_nav_portal ~93.69% -- validation,
 * portal traversal, side test, normalization, and stores match retail. Residue
 * is GPR coloring and pointer-versus-offset induction for the portal list.
 */
void nav_get_unit_vector_to_nav_portal(Vec* out, Vec* pos, int areaIndex, int portalId) {
    KonquestPdata* pdata = konquest_pdata;
    KonquestNavData* nav = pdata->navData;
    NavArea* area;
    NavPortalList* portals;
    NavPortalEntry* portal;
    int areaCount = nav->areaCount;
    int i;

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
    portal = portals->entries;
    for (i = 0; i < portals->count; i++, portal++) {
        if (portal->adjacentArea == portalId) {
            float length = portal->length;
            float halfLength = 0.5f * length;
            float normalZ = portal->normalZ;
            float normalX = portal->normalX;
            float dz = normalZ * halfLength + portal->z - pos->z;
            float dx = normalX * halfLength + portal->x - pos->x;
            float side = normalX * dx + normalZ * dz;

            if (side >= 0.25f * length || side <= -0.25f * length) {
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
    }
}

/*
 * Soft ceiling: nav_which_area_is_next ~86.94% -- breadth-first traversal,
 * predecessor writes, target detection, and backtracking match retail. Residue
 * is counted-clear lowering, save grouping, and queue/portal register coloring.
 */
int nav_which_area_is_next(int fromArea, int toArea) {
    int areaCount;
    int i;
    int queuedCount;
    int found;
    int result;
    int nextArea;
    int currentArea;

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
    while (i != queuedCount && found == 0) {
        NavArea* area;
        NavPortalList* portals;
        NavPortalEntry* portal;
        int portalCount;
        int portalIndex;

        currentArea = konquest_pdata->areaQueue[i++];
        area = nav_get_area(konquest_pdata->navData, currentArea);
        portals = nav_get_portals(area);
        portal = portals->entries;
        portalCount = portals->count;
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

static NavArea* unit_vector_to_area(NavArea* area, Vec* nearestNormal,
                                    float* nearestDistance,
                                    Vec* farthestNormal,
                                    float* farthestDistance,
                                    Vec* position);

/*
 * Soft ceiling: nav_get_unit_vector_to_closest_area ~92.91% -- packed-area
 * traversal, distance selection, blend, normalization, and stores match retail.
 * Remaining differences are save grouping and local GPR/FPR scheduling.
 */
void nav_get_unit_vector_to_closest_area(Vec* out, Vec* pos) {
    KonquestNavData* nav;
    NavArea* area;
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
    int areaIndex;
    int count;

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
    /* Variable-size area records follow the area-pointer table in the NAV blob. */
    area = (NavArea*)&nav->areas[count];
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

/*
 * Soft ceiling: nav_get_unit_vector_to_area ~99.65% -- operations and control
 * flow match; only five operands use different GPR/FPR allocation.
 */
void nav_get_unit_vector_to_area(int areaIndex, Vec* out, Vec* pos) {
    KonquestNavData* nav;
    NavArea* area;
    float nearestDistance;
    float farthestDistance;
    Vec farthestNormal;
    Vec nearestNormal;
    float ratio;
    float blend;
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
    blend = 1.0f - ratio;
    nearestNormal.x += farthestNormal.x * blend;
    nearestNormal.z += farthestNormal.z * blend;
    lengthSquared = nearestNormal.x * nearestNormal.x +
                    nearestNormal.z * nearestNormal.z;
    inverseLength = nav_inverse_sqrt(lengthSquared);
    out->x = nearestNormal.x * inverseLength;
    out->z = nearestNormal.z * inverseLength;
    out->x = -out->x;
    out->z = -out->z;
    out->y = 0.0f;
}

/*
 * Soft ceiling: unit_vector_to_area ~91.67% -- boundary traversal, extrema,
 * normals, and packed next-area address match retail. Residue is register
 * coloring plus MWCC's joined return versus retail's split returns.
 */
static NavArea* unit_vector_to_area(NavArea* area, Vec* nearestNormal,
                                    float* nearestDistance,
                                    Vec* farthestNormal,
                                    float* farthestDistance,
                                    Vec* position) {
    int inside;
    NavBoundary* boundary;
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
    /* Step across the packed portal tail to the next variable-size area. */
    count = ((NavPortalList*)boundary)->count;
    boundary = (NavBoundary*)((NavPortalList*)boundary)->entries;
    boundary = (NavBoundary*)((NavPortalEntry*)boundary + count);
    if (inside != 0) {
        return 0;
    }
    return (NavArea*)boundary;
}

/*
 * Soft ceiling: nav_what_area_is_point_in ~88.21% -- hint validation,
 * containment, adjacent search, and tile fallback match retail. Residue comes
 * from counted-clear and byte-indexed tile-scan lowering and register scheduling.
 */
int nav_what_area_is_point_in(Vec* pos, int hintArea) {
    KonquestNavData* nav;
    NavArea* area;
    NavPortalList* portals;
    NavPortalEntry* portal;
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
        NavArea* adjacentArea = nav_get_area(konquest_pdata->navData,
                                             adjacentAreaIndex);

        if (nav_area_contains_point(adjacentArea, pos)) {
            return adjacentAreaIndex;
        }
    }
    return nav_find_area_in_tile(pos);
}

void konquest_nav_init(void) {
    unsigned int artId;
    KonquestNavData* nav;
    int allocationSize;

    artId = get_artid_of_named_item_in_slot(0x60029, konquestNavStrings, 0);
    if (artId != 0) {
        nav = (KonquestNavData*)get_nav_data(0x60029, artId);
        if (nav != 0) {
            konquest_pdata->navData = nav;
            allocationSize = nav->areaCount * (int)sizeof(int);
            konquest_pdata->areaQueue = (int*)get_mem(allocationSize);
            if (konquest_pdata->areaQueue != 0) {
                konquest_pdata->areaPredecessors = (int*)get_mem(allocationSize);
                if (konquest_pdata->areaPredecessors != 0) {
                    setup_per_tile_navigations();
                }
            }
        } else {
            debug_print_message(&konquestNavStrings[4]);
        }
    }
}

/*
 * Soft ceiling: setup_per_tile_navigations ~86.18% -- polygon intersections,
 * tile rejection, containment, edge crossing, and all bounded insertion paths
 * match retail. Residue is stack-offset versus pointer induction, constant-loop
 * ctr selection, and cascading register allocation in the geometry loops.
 */
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
        NavArea* area = nav_get_area(konquest_pdata->navData, areaIndex);

        boundaryCount = area->boundaryCount;

        if (boundaryCount <= 15) {
            NavBoundary* boundary = area->boundaries;
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

                current.x = boundary->x;
                current.y = 0.0f;
                current.z = boundary->z;
                currentOffset = boundary->offset;
                boundary++;
                intersect_xz_lines(&previous, &current,
                                   &intersections[boundaryIndex],
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
            NavTile* tile = &konquest_pdata->navTiles[tileIndex];
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
            NavBoundary* boundary;

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
                         boundaryIndex++, boundary++) {
                        float boundaryX = boundary->x;
                        float boundaryZ = boundary->z;
                        float boundaryOffset = boundary->offset;
                        int allCornersOutside = 1;
                        int allCornersInside = 1;
                        int remainingCorners = 4;
                        Vec* corner = tileCorners;
                        int* outside = cornerOutside;

                        do {
                            float planeDistance = boundaryX * corner->x +
                                                  boundaryZ * corner->z;

                            if (planeDistance > boundaryOffset) {
                                *outside = 1;
                                allCornersInside = 0;
                                allTileCornersInsideArea = 0;
                            } else {
                                *outside = 0;
                                allCornersOutside = 0;
                            }
                            corner++;
                            outside++;
                            remainingCorners--;
                        } while (remainingCorners != 0);
                        if (allCornersOutside) {
                            break;
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
                                        boundaryIndex = boundaryCount;
                                        break;
                                    }
                                }
                                previousCorner = cornerIndex;
                            }
                            if (boundaryIndex >= boundaryCount) {
                                break;
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

            tileIndex++;
        }
    }
}
