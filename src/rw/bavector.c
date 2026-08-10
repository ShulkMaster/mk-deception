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

static void SqrtTableDestroy(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);

    if (globals->sqrtTable != 0) {
        RwEngineInstance->fpFree(globals->sqrtTable);
        globals->sqrtTable = 0;
    }
}

static RwBool SqrtTableCreate(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwUInt32* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    RwUInt32* upperTable;
    RwRealBits input;
    RwRealBits output;
    RwUInt32 index;

    if (table == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return 0;
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

    globals->sqrtTable = table;
    return 1;
}

static void InvSqrtTableDestroy(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);

    if (globals->invSqrtTable != 0) {
        RwEngineInstance->fpFree(globals->invSqrtTable);
        globals->invSqrtTable = 0;
    }
}

static RwBool InvSqrtTableCreate(void) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwUInt32* table = RwEngineInstance->fpMalloc(0x4000, 0x40401);
    RwUInt32* upperTable;
    RwRealBits input;
    RwRealBits output;
    RwUInt32 index;

    if (table == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, 0x4000);
        RwErrorSet(&error);
        return 0;
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

    globals->invSqrtTable = table;
    return 1;
}

RwReal _rwSqrt(RwReal value) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        RwUInt32* table = globals->sqrtTable;
        result.bits += 0x800;
        result.bits = ((result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}

RwReal _rwInvSqrt(RwReal value) {
    RwVectorGlobals* globals = (RwVectorGlobals*)((RwUInt8*)RwEngineInstance +
                                                 vectorModule.globalsOffset);
    RwRealBits result;
    result.value = value;
    if (result.bits != 0) {
        RwUInt32* table = globals->invSqrtTable;
        result.bits += 0x800;
        result.bits = ((~result.bits & 0x7F800000) >> 1) +
            table[(result.bits & 0x00FFFFFF) >> 12];
    }
    return result.value;
}








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
        return 0;
    }
    if (!InvSqrtTableCreate()) {
        return 0;
    }
    ++vectorModule.numInstances;
    return instance;
}
