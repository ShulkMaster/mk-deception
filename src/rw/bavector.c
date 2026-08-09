#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwvector.h"

extern RwReal sqrtf(RwReal value);

typedef union RwRealBits {
    RwReal value;
    RwUInt32 bits;
} RwRealBits;

typedef struct RwVectorGlobals {
    RwUInt32* sqrtTable;
    RwUInt32* invSqrtTable;
} RwVectorGlobals;

static RwModuleInfo vectorModule;

#define VECTORGLOBALS \
    RWPLUGINOFFSET(RwVectorGlobals, RwEngineInstance, \
                   vectorModule.globalsOffset)

static void SqrtTableDestroy(void) {
    if (VECTORGLOBALS.sqrtTable != NULL) {
        RwEngineInstance->fpFree(VECTORGLOBALS.sqrtTable);
        VECTORGLOBALS.sqrtTable = NULL;
    }
}

static RwBool SqrtTableCreate(void) {
    RwUInt32* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    RwUInt32* upperTable;
    RwRealBits input;
    RwRealBits output;
    RwUInt32 index;

    if (table == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return FALSE;
    }

    upperTable = table + 0x800;
    input.value = 1.0f;
    for (index = 0; index < 0x800; ++index) {
        output.value = sqrtf(input.value);
        output.bits -= 0x1FC00000;
        upperTable[index] = output.bits;
        input.bits += 0x1000;
    }
    for (index = 0; index < 0x800; ++index) {
        output.value = sqrtf(input.value);
        output.bits -= 0x20000000;
        table[index] = output.bits;
        input.bits += 0x1000;
    }

    VECTORGLOBALS.sqrtTable = table;
    return TRUE;
}

static void InvSqrtTableDestroy(void) {
    if (VECTORGLOBALS.invSqrtTable != NULL) {
        RwEngineInstance->fpFree(VECTORGLOBALS.invSqrtTable);
        VECTORGLOBALS.invSqrtTable = NULL;
    }
}

static RwBool InvSqrtTableCreate(void) {
    RwUInt32* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    RwUInt32* upperTable;
    RwRealBits input;
    RwRealBits output;
    RwUInt32 index;

    if (table == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return FALSE;
    }

    upperTable = table + 0x800;
    input.value = 1.0f;
    for (index = 0; index < 0x800; ++index) {
        output.value = 1.0f / sqrtf(input.value);
        output.bits -= 0x20000000;
        upperTable[index] = output.bits;
        input.bits += 0x1000;
    }
    for (index = 0; index < 0x800; ++index) {
        output.value = 1.0f / sqrtf(input.value);
        output.bits -= 0x1FC00000;
        table[index] = output.bits;
        input.bits += 0x1000;
    }

    VECTORGLOBALS.invSqrtTable = table;
    return TRUE;
}

RwReal _rwSqrt(RwReal value) {
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        RwUInt32* table = VECTORGLOBALS.sqrtTable;
        result.bits += 0x800;
        result.bits = ((result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}

RwReal _rwInvSqrt(RwReal value) {
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        RwUInt32* table = VECTORGLOBALS.invSqrtTable;
        result.bits += 0x800;
        result.bits = ((~result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}

/*
 * Retail expands the GameCube driver override of these three public APIs into
 * paired-single loads, multiply-adds, and stores (psq_l/ps_maddsX/psq_st).
 * The typed scalar C below expresses the same matrix operations and ABI, but
 * this compiler configuration cannot recreate that platform macro without
 * nonportable intrinsics or embedded assembly, which the quality gate forbids.
 */
RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn,
                           const RwMatrix* matrix) {
    RwReal x = pointIn->x;
    RwReal y = pointIn->y;
    RwReal z = pointIn->z;
    pointOut->x = matrix->right.x * x + matrix->up.x * y +
                  matrix->at.x * z + matrix->pos.x;
    pointOut->y = matrix->right.y * x + matrix->up.y * y +
                  matrix->at.y * z + matrix->pos.y;
    pointOut->z = matrix->right.z * x + matrix->up.z * y +
                  matrix->at.z * z + matrix->pos.z;
    return pointOut;
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn,
                            RwInt32 numPoints, const RwMatrix* matrix) {
    RwV3d* result = pointsOut;
    do {
        RwReal x = pointsIn->x;
        RwReal y = pointsIn->y;
        RwReal z = pointsIn->z;
        pointsOut->x = matrix->right.x * x + matrix->up.x * y +
                       matrix->at.x * z + matrix->pos.x;
        pointsOut->y = matrix->right.y * x + matrix->up.y * y +
                       matrix->at.y * z + matrix->pos.y;
        pointsOut->z = matrix->right.z * x + matrix->up.z * y +
                       matrix->at.z * z + matrix->pos.z;
        ++pointsIn;
        ++pointsOut;
    } while (--numPoints != 0);
    return result;
}

RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn,
                            const RwMatrix* matrix) {
    RwReal x = vectorIn->x;
    RwReal y = vectorIn->y;
    RwReal z = vectorIn->z;
    vectorOut->x = matrix->right.x * x + matrix->up.x * y + matrix->at.x * z;
    vectorOut->y = matrix->right.y * x + matrix->up.y * y + matrix->at.y * z;
    vectorOut->z = matrix->right.z * x + matrix->up.z * y + matrix->at.z * z;
    return vectorOut;
}

void* _rwVectorClose(void* instance, RwInt32 offset, RwInt32 size) {
    InvSqrtTableDestroy();
    SqrtTableDestroy();
    --vectorModule.numInstances;
    return instance;
}

void* _rwVectorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    vectorModule.globalsOffset = offset;
    if (!SqrtTableCreate()) {
        return NULL;
    }
    if (!InvSqrtTableCreate()) {
        return NULL;
    }
    ++vectorModule.numInstances;
    return instance;
}
