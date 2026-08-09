#include "libmkparticle/rw_engine.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

typedef struct RpGameCubeMTEntry24 {
    RwInt32 value[5];
    RwUInt16 field_0x14;
    RwUInt16 field_0x16;
} RpGameCubeMTEntry24;

typedef struct RpGameCubeMTEntry64 {
    RwInt32 field_0x00;
    RwInt32 field_0x04;
    RwInt32 field_0x08;
    RwInt32 field_0x0C;
    RwInt32 field_0x10;
    RwInt32 field_0x14;
    RwInt32 field_0x18;
    RwInt32 field_0x1C;
    RwInt32 field_0x20;
    RwInt32 field_0x24;
    RwInt32 field_0x28;
    RwUInt8 field_0x2C;
    RwUInt8 reserved_0x2D[3];
    RwInt32 field_0x30;
    RwInt32 field_0x34;
    RwInt32 field_0x38;
    RwInt32 field_0x3C;
    RwUInt8 field_0x40;
    RwUInt8 reserved_0x41[3];
    RwInt32 field_0x44;
    RwInt32 field_0x48;
    RwInt32 field_0x4C;
    RwInt32 field_0x50;
    RwInt32 field_0x54;
    RwInt32 field_0x58;
    RwUInt32 field_0x5C;
    RwInt32 field_0x60;
} RpGameCubeMTEntry64;

typedef struct RpGameCubeMTEntry60 {
    RwInt32 value[3];
    RwReal real[12];
} RpGameCubeMTEntry60;

typedef struct RpGameCubeMTEntry20 {
    RwInt32 value[5];
} RpGameCubeMTEntry20;

typedef struct RpGameCubeMTEntry40 {
    RwInt32 value[3];
    RwReal real[6];
    RwInt32 field_0x24;
} RpGameCubeMTEntry40;

struct RpGameCubeMTEffectConfig {
    RwInt32 field_0x00;
    RwUInt8 allocationCount24;
    RwUInt8 allocationCount64;
    RwUInt8 allocationCount60;
    RwUInt8 allocationCount20;
    RwUInt8 allocationCount40;
    RwUInt8 count24;
    RwUInt8 count64;
    RwUInt8 count60;
    RwUInt8 count20;
    RwUInt8 count40;
    RwUInt16 values[16];
    RwUInt8 bytes[16];
    RwUInt8 reserved_0x3E[2];
    RpGameCubeMTEntry60* entries60;
    RpGameCubeMTEntry24* entries24;
    RpGameCubeMTEntry40* entries40;
    RpGameCubeMTEntry20* entries20;
    RpGameCubeMTEntry64* entries64;
    RwInt32 field_0x54;
    RwInt32 field_0x58;
};

typedef struct RpGameCubeMTStreamHeader {
    RwUInt8 count24;
    RwUInt8 count64;
    RwUInt8 field_0x02;
    RwUInt8 packedFields;
    RwUInt16 values[16];
    RwUInt8 bytes[16];
    RwUInt8 count60;
    RwUInt8 count40;
    RwUInt8 count20;
    RwUInt8 reserved;
} RpGameCubeMTStreamHeader;

typedef struct RpGameCubeMTStreamEntry24 {
    RwUInt8 value[5];
    RwUInt8 reserved[3];
    RwUInt16 field_0x08;
    RwUInt16 field_0x0A;
} RpGameCubeMTStreamEntry24;

typedef struct RpGameCubeMTStreamEntry64 {
    RwUInt8 value[24];
    RwUInt32 field_0x18;
} RpGameCubeMTStreamEntry64;

typedef struct RpGameCubeMTStreamEntry60 {
    RwUInt8 value[3];
    RwUInt8 reserved;
} RpGameCubeMTStreamEntry60;

typedef struct RpGameCubeMTStreamEntry20 {
    RwUInt8 value[5];
    RwUInt8 reserved[3];
} RpGameCubeMTStreamEntry20;

typedef struct RpGameCubeMTStreamEntry40 {
    RwUInt8 value[3];
    signed char field_0x03;
} RpGameCubeMTStreamEntry40;

extern void* memcpy(void* destination, const void* source, RwUInt32 size);
extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern void* RwMemLittleEndian16(void* memory, RwUInt32 size);
extern RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* values,
                                   RwUInt32 size);
extern RwStream* RwStreamReadReal(RwStream* stream, RwReal* values,
                                  RwUInt32 size);

static RwInt32 GameCubeMTEffectStreamGetSize(const RpMTEffect* effect)
{
    RwInt32 size = 0;
    const RpGameCubeMTEffectConfig* config =
        (const RpGameCubeMTEffectConfig*)((const RwUInt8*)effect + 0x30);

    size += sizeof(RpGameCubeMTStreamHeader);
    size += config->count60 * (sizeof(RpGameCubeMTStreamEntry60) + 0x30);
    size += config->count24 * sizeof(RpGameCubeMTStreamEntry24);
    size += config->count40 *
            (sizeof(RpGameCubeMTStreamEntry40) + 0x18);
    size += config->count20 * sizeof(RpGameCubeMTStreamEntry20);
    size += config->count64 * sizeof(RpGameCubeMTStreamEntry64);
    return size;
}

/* Near match: only equivalent packed-nibble masking/evaluation differs. */
static RwStream* GameCubeMTEffectStreamWrite(
    const RpMTEffect* effect, RwStream* stream)
{
    const RpGameCubeMTEffectConfig* config =
        (const RpGameCubeMTEffectConfig*)((const RwUInt8*)effect + 0x30);
    RpGameCubeMTStreamHeader header;
    RwUInt32 index;

    if (_rwStreamWriteVersionedChunkHeader(
            stream, 3, GameCubeMTEffectStreamGetSize(effect),
            0x36003, 0xFFFF) == NULL) {
        return NULL;
    }

    header.field_0x02 = (RwUInt8)config->field_0x00;
    header.count64 = config->count64;
    header.count24 = config->count24;
    header.count60 = config->count60;
    header.count40 = config->count40;
    header.count20 = config->count20;
    header.packedFields =
        (((RwUInt8)config->field_0x54 & 0xF) * 0x10) |
        ((RwUInt8)config->field_0x58 & 0xF);
    memcpy(header.bytes, config->bytes, sizeof(header.bytes));
    memcpy(header.values, config->values, sizeof(header.values));
    RwMemLittleEndian16(header.values, sizeof(header.values));
    if (RwStreamWrite(stream, &header, sizeof(header)) == NULL) {
        return NULL;
    }

    for (index = 0; index < config->count24; index++) {
        const RpGameCubeMTEntry24* entry = &config->entries24[index];
        RpGameCubeMTStreamEntry24 encoded;

        encoded.value[0] = (RwUInt8)entry->value[0];
        encoded.value[1] = (RwUInt8)entry->value[1];
        encoded.value[2] = (RwUInt8)entry->value[2];
        encoded.value[3] = (RwUInt8)entry->value[3];
        encoded.value[4] = (RwUInt8)entry->value[4];
        encoded.reserved[0] = 0;
        encoded.reserved[1] = 0;
        encoded.reserved[2] = 0;
        encoded.field_0x08 = entry->field_0x14;
        encoded.field_0x0A = entry->field_0x16;
        RwMemLittleEndian16(&encoded.field_0x08, 4);
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == NULL) {
            return NULL;
        }
    }

    for (index = 0; index < config->count64; index++) {
        const RpGameCubeMTEntry64* entry = &config->entries64[index];
        RpGameCubeMTStreamEntry64 encoded;

        encoded.value[0] = (RwUInt8)entry->field_0x00;
        encoded.value[1] = (RwUInt8)entry->field_0x04;
        encoded.value[2] = (RwUInt8)entry->field_0x08;
        encoded.value[3] = (RwUInt8)entry->field_0x0C;
        encoded.value[4] = (RwUInt8)entry->field_0x10;
        encoded.value[5] = (RwUInt8)entry->field_0x14;
        encoded.value[6] = (RwUInt8)entry->field_0x18;
        encoded.value[7] = (RwUInt8)entry->field_0x1C;
        encoded.value[8] = (RwUInt8)entry->field_0x20;
        encoded.value[9] = (RwUInt8)entry->field_0x24;
        encoded.value[10] = (RwUInt8)entry->field_0x28;
        encoded.value[11] = (RwUInt8)entry->field_0x2C;
        encoded.value[12] = (RwUInt8)entry->field_0x30;
        encoded.value[13] = (RwUInt8)entry->field_0x34;
        encoded.value[14] = (RwUInt8)entry->field_0x38;
        encoded.value[15] = (RwUInt8)entry->field_0x3C;
        encoded.value[16] = (RwUInt8)entry->field_0x40;
        encoded.value[17] = (RwUInt8)entry->field_0x44;
        encoded.value[18] = (RwUInt8)entry->field_0x48;
        encoded.value[19] = (RwUInt8)entry->field_0x4C;
        encoded.value[20] = (RwUInt8)entry->field_0x50;
        encoded.value[21] = (RwUInt8)entry->field_0x54;
        encoded.value[22] = (RwUInt8)entry->field_0x58;
        encoded.value[23] = (RwUInt8)entry->field_0x5C;
        encoded.field_0x18 = (RwUInt32)entry->field_0x60;
        RwMemLittleEndian32(&encoded.field_0x18, sizeof(encoded.field_0x18));
        if ((entry->field_0x54 & 0x100) != 0) {
            encoded.value[23] |= 0x80;
        }
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == NULL) {
            return NULL;
        }
    }

    for (index = 0; index < config->count60; index++) {
        const RpGameCubeMTEntry60* entry = &config->entries60[index];
        RpGameCubeMTStreamEntry60 encoded;

        encoded.value[0] = (RwUInt8)entry->value[0];
        encoded.value[1] = (RwUInt8)entry->value[1];
        encoded.value[2] = (RwUInt8)entry->value[2];
        encoded.reserved = 0;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == NULL ||
            RwStreamWriteReal(stream, entry->real, sizeof(entry->real)) ==
                NULL) {
            return NULL;
        }
    }

    for (index = 0; index < config->count40; index++) {
        const RpGameCubeMTEntry40* entry = &config->entries40[index];
        RpGameCubeMTStreamEntry40 encoded;

        encoded.value[0] = (RwUInt8)entry->value[0];
        encoded.value[1] = (RwUInt8)entry->value[1];
        encoded.value[2] = (RwUInt8)entry->value[2];
        encoded.field_0x03 = (signed char)entry->field_0x24;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == NULL ||
            RwStreamWriteReal(stream, entry->real, sizeof(entry->real)) ==
                NULL) {
            return NULL;
        }
    }

    for (index = 0; index < config->count20; index++) {
        const RpGameCubeMTEntry20* entry = &config->entries20[index];
        RpGameCubeMTStreamEntry20 encoded;

        encoded.value[0] = (RwUInt8)entry->value[0];
        encoded.value[1] = (RwUInt8)entry->value[1];
        encoded.value[2] = (RwUInt8)entry->value[2];
        encoded.value[3] = (RwUInt8)entry->value[3];
        encoded.value[4] = (RwUInt8)entry->value[4];
        encoded.reserved[0] = 0;
        encoded.reserved[1] = 0;
        encoded.reserved[2] = 0;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == NULL) {
            return NULL;
        }
    }
    return (RwStream*)effect;
}

/* Near match: retail retains three redundant byte zero-extensions. */
static RpMTEffect* GameCubeMTEffectStreamRead(
    RwStream* stream, RwInt32 type, RwUInt32 version, RwInt32 length)
{
    RpGameCubeMTStreamHeader header;
    RpMTEffect* effect;
    RpGameCubeMTEffectConfig* config;
    RwUInt32 streamSize;
    RwUInt32 index;

    streamSize = sizeof(header);
    if (RwStreamRead(stream, &header, streamSize) != streamSize) {
        return NULL;
    }
    effect = RpGameCubeMTEffectCreate(
        header.count64, header.count24, header.count60,
        header.count20, header.count40);
    if (effect == NULL) {
        return NULL;
    }
    config = RpGameCubeMTEffectGetConfig(effect);
    config->field_0x00 = header.field_0x02;
    config->field_0x54 = (header.packedFields >> 4) & 0xF;
    config->field_0x58 = header.packedFields & 0xF;
    memcpy(config->bytes, header.bytes, sizeof(config->bytes));
    RwMemNative32(header.values, sizeof(header.values));
    memcpy(config->values, header.values, sizeof(config->values));

    for (index = 0; index < config->count24; index++) {
        RpGameCubeMTEntry24* entry = &config->entries24[index];
        RpGameCubeMTStreamEntry24 encoded;

        streamSize = sizeof(encoded);
        if (RwStreamRead(stream, &encoded, streamSize) != streamSize) {
            RwEngineInstance->fpFree(effect);
            return NULL;
        }
        entry->value[0] = encoded.value[0];
        entry->value[1] = encoded.value[1];
        entry->value[2] = encoded.value[2];
        entry->value[3] = encoded.value[3];
        entry->value[4] = encoded.value[4];
        RwMemNative32(&encoded.field_0x08, 4);
        entry->field_0x14 = encoded.field_0x08;
        entry->field_0x16 = encoded.field_0x0A;
    }

    for (index = 0; index < config->count64; index++) {
        RpGameCubeMTEntry64* entry = &config->entries64[index];
        RpGameCubeMTStreamEntry64 encoded;

        streamSize = sizeof(encoded);
        if (RwStreamRead(stream, &encoded, streamSize) != streamSize) {
            RwEngineInstance->fpFree(effect);
            return NULL;
        }
        entry->field_0x00 = encoded.value[0];
        entry->field_0x04 = encoded.value[1];
        entry->field_0x08 = encoded.value[2];
        entry->field_0x0C = encoded.value[3];
        entry->field_0x10 = encoded.value[4];
        entry->field_0x14 = encoded.value[5];
        entry->field_0x18 = encoded.value[6];
        entry->field_0x1C = encoded.value[7];
        entry->field_0x20 = encoded.value[8];
        entry->field_0x24 = encoded.value[9];
        entry->field_0x28 = encoded.value[10];
        entry->field_0x2C = encoded.value[11];
        entry->field_0x30 = encoded.value[12];
        entry->field_0x34 = encoded.value[13];
        entry->field_0x38 = encoded.value[14];
        entry->field_0x3C = encoded.value[15];
        entry->field_0x40 = encoded.value[16];
        entry->field_0x44 = encoded.value[17];
        entry->field_0x48 = encoded.value[18];
        entry->field_0x4C = encoded.value[19];
        entry->field_0x50 = encoded.value[20];
        entry->field_0x54 = encoded.value[21];
        entry->field_0x58 = encoded.value[22];
        entry->field_0x5C = encoded.value[23];
        RwMemNative32(&encoded.field_0x18, sizeof(encoded.field_0x18));
        entry->field_0x60 = (RwInt32)encoded.field_0x18;
        if ((entry->field_0x5C & 0x80) != 0) {
            entry->field_0x54 |= 0x100;
            entry->field_0x5C &= ~0x80;
        }
    }

    for (index = 0; index < config->count60; index++) {
        RpGameCubeMTEntry60* entry = &config->entries60[index];
        RpGameCubeMTStreamEntry60 encoded;

        if (RwStreamRead(stream, &encoded, sizeof(encoded)) !=
                sizeof(encoded) ||
            RwStreamReadReal(stream, entry->real, sizeof(entry->real)) ==
                NULL) {
            RwEngineInstance->fpFree(effect);
            return NULL;
        }
        entry->value[0] = encoded.value[0];
        entry->value[1] = encoded.value[1];
        entry->value[2] = encoded.value[2];
    }

    for (index = 0; index < config->count40; index++) {
        RpGameCubeMTEntry40* entry = &config->entries40[index];
        RpGameCubeMTStreamEntry40 encoded;

        if (RwStreamRead(stream, &encoded, sizeof(encoded)) !=
                sizeof(encoded) ||
            RwStreamReadReal(stream, entry->real, sizeof(entry->real)) ==
                NULL) {
            RwEngineInstance->fpFree(effect);
            return NULL;
        }
        entry->value[0] = encoded.value[0];
        entry->value[1] = encoded.value[1];
        entry->value[2] = encoded.value[2];
        entry->field_0x24 = encoded.field_0x03;
    }

    for (index = 0; index < config->count20; index++) {
        RpGameCubeMTEntry20* entry = &config->entries20[index];
        RpGameCubeMTStreamEntry20 encoded;

        if (RwStreamRead(stream, &encoded, sizeof(encoded)) !=
            sizeof(encoded)) {
            RwEngineInstance->fpFree(effect);
            return NULL;
        }
        entry->value[0] = encoded.value[0];
        entry->value[1] = encoded.value[1];
        entry->value[2] = encoded.value[2];
        entry->value[3] = encoded.value[3];
        entry->value[4] = encoded.value[4];
    }
    return effect;
}

RwBool _rpGameCubeMTDataPluginAttach(void)
{
    RwBool result = _rpMTEffectRegisterPlatform(
        6, GameCubeMTEffectStreamRead, GameCubeMTEffectStreamWrite,
        GameCubeMTEffectStreamGetSize, NULL);
    return result;
}

RpMTEffect* RpGameCubeMTEffectCreate(
    RwUInt32 count64, RwUInt32 count24, RwUInt32 count60,
    RwUInt32 count20, RwUInt32 count40)
{
    /* The remaining objdiff residue is allocation-size register coloring. */
    RwUInt32 size = 0x8C;
    RpMTEffect* effect;
    RpGameCubeMTEffectConfig* config;
    RwUInt8* next;

    size = (size + 3) & ~3U;
    size += count24 * sizeof(RpGameCubeMTEntry24);
    size = (size + 3) & ~3U;
    size += count64 * sizeof(RpGameCubeMTEntry64);
    size = (size + 3) & ~3U;
    size += count20 * sizeof(RpGameCubeMTEntry20);
    size = (size + 3) & ~3U;
    size += count60 * sizeof(RpGameCubeMTEntry60);
    size = (size + 3) & ~3U;
    size += count40 * sizeof(RpGameCubeMTEntry40);
    size = (size + 3) & ~3U;

    effect = RwEngineInstance->fpMalloc(size, 0x30129);
    if (effect == NULL) {
        RwError error;

        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return NULL;
    }
    memset(effect, 0, size);
    _rpMTEffectInit(effect, 6);
    config = (RpGameCubeMTEffectConfig*)((RwUInt8*)effect + 0x30);
    config->allocationCount64 = count64;
    config->allocationCount24 = count24;
    config->allocationCount20 = count20;
    config->allocationCount60 = count60;
    config->allocationCount40 = count40;
    config->count64 = config->allocationCount64;
    config->count24 = config->allocationCount24;
    config->count20 = config->allocationCount20;
    config->count60 = config->allocationCount60;
    config->count40 = config->allocationCount40;

    next = (RwUInt8*)config + sizeof(*config);
    if (count60 != 0) {
        config->entries60 = (RpGameCubeMTEntry60*)next;
        next += count60 * sizeof(*config->entries60);
    }
    if (count24 != 0) {
        config->entries24 = (RpGameCubeMTEntry24*)next;
        next += count24 * sizeof(*config->entries24);
    }
    if (count40 != 0) {
        config->entries40 = (RpGameCubeMTEntry40*)next;
        next += count40 * sizeof(*config->entries40);
    }
    if (count20 != 0) {
        config->entries20 = (RpGameCubeMTEntry20*)next;
        next += count20 * sizeof(*config->entries20);
    }
    if (count64 != 0) {
        config->entries64 = (RpGameCubeMTEntry64*)next;
    }
    return effect;
}

RpGameCubeMTEffectConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect)
{
    RpGameCubeMTEffectConfig* config =
        (RpGameCubeMTEffectConfig*)((RwUInt8*)effect + 0x30);
    return config;
}
