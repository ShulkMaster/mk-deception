#include "libmkparticle/rw_engine.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

typedef struct RpGameCubeMTEntry24 {
    int value[5];
    unsigned short field_0x14;
    unsigned short field_0x16;
} RpGameCubeMTEntry24;

typedef struct RpGameCubeMTEntry64 {
    int field_0x00;
    int field_0x04;
    int field_0x08;
    int field_0x0C;
    int field_0x10;
    int field_0x14;
    int field_0x18;
    int field_0x1C;
    int field_0x20;
    int field_0x24;
    int field_0x28;
    unsigned char field_0x2C;
    unsigned char reserved_0x2D[3];
    int field_0x30;
    int field_0x34;
    int field_0x38;
    int field_0x3C;
    unsigned char field_0x40;
    unsigned char reserved_0x41[3];
    int field_0x44;
    int field_0x48;
    int field_0x4C;
    int field_0x50;
    int field_0x54;
    int field_0x58;
    unsigned int field_0x5C;
    int field_0x60;
} RpGameCubeMTEntry64;

typedef struct RpGameCubeMTEntry60 {
    int value[3];
    float real[12];
} RpGameCubeMTEntry60;

typedef struct RpGameCubeMTEntry20 {
    int value[5];
} RpGameCubeMTEntry20;

typedef struct RpGameCubeMTEntry40 {
    int value[3];
    float real[6];
    int field_0x24;
} RpGameCubeMTEntry40;

struct RpGameCubeMTEffectConfig {
    int field_0x00;
    unsigned char allocationCount24;
    unsigned char allocationCount64;
    unsigned char allocationCount60;
    unsigned char allocationCount20;
    unsigned char allocationCount40;
    unsigned char count24;
    unsigned char count64;
    unsigned char count60;
    unsigned char count20;
    unsigned char count40;
    unsigned short values[16];
    unsigned char bytes[16];
    unsigned char reserved_0x3E[2];
    RpGameCubeMTEntry60* entries60;
    RpGameCubeMTEntry24* entries24;
    RpGameCubeMTEntry40* entries40;
    RpGameCubeMTEntry20* entries20;
    RpGameCubeMTEntry64* entries64;
    int field_0x54;
    int field_0x58;
};

typedef struct RpGameCubeMTStreamHeader {
    unsigned char count24;
    unsigned char count64;
    unsigned char field_0x02;
    unsigned char packedFields;
    unsigned short values[16];
    unsigned char bytes[16];
    unsigned char count60;
    unsigned char count40;
    unsigned char count20;
    unsigned char reserved;
} RpGameCubeMTStreamHeader;

typedef struct RpGameCubeMTStreamEntry24 {
    unsigned char value[5];
    unsigned char reserved[3];
    unsigned short field_0x08;
    unsigned short field_0x0A;
} RpGameCubeMTStreamEntry24;

typedef struct RpGameCubeMTStreamEntry64 {
    unsigned char value[24];
    unsigned int field_0x18;
} RpGameCubeMTStreamEntry64;

typedef struct RpGameCubeMTStreamEntry60 {
    unsigned char value[3];
    unsigned char reserved;
} RpGameCubeMTStreamEntry60;

typedef struct RpGameCubeMTStreamEntry20 {
    unsigned char value[5];
    unsigned char reserved[3];
} RpGameCubeMTStreamEntry20;

typedef struct RpGameCubeMTStreamEntry40 {
    unsigned char value[3];
    signed char field_0x03;
} RpGameCubeMTStreamEntry40;

extern void* memcpy(void* destination, const void* source, unsigned int size);
extern void* memset(void* destination, int value, unsigned int size);

static int GameCubeMTEffectStreamGetSize(const RpMTEffect* effect)
{
    int size = 0;
    const RpGameCubeMTEffectConfig* config =
        (const RpGameCubeMTEffectConfig*)((const unsigned char*)effect + 0x30);

    size += sizeof(RpGameCubeMTStreamHeader);
    size += config->count60 * (sizeof(RpGameCubeMTStreamEntry60) + 0x30);
    size += config->count24 * sizeof(RpGameCubeMTStreamEntry24);
    size += config->count40 *
            (sizeof(RpGameCubeMTStreamEntry40) + 0x18);
    size += config->count20 * sizeof(RpGameCubeMTStreamEntry20);
    size += config->count64 * sizeof(RpGameCubeMTStreamEntry64);
    return size;
}


static const RpMTEffect* GameCubeMTEffectStreamWrite(
    const RpMTEffect* effect, RwStream* stream)
{
    const RpGameCubeMTEffectConfig* config =
        (const RpGameCubeMTEffectConfig*)((const unsigned char*)effect + 0x30);
    RpGameCubeMTStreamHeader header;
    unsigned int index;

    if (_rwStreamWriteVersionedChunkHeader(
            stream, 3, GameCubeMTEffectStreamGetSize(effect),
            0x36003, 0xFFFF) == 0) {
        return 0;
    }

    header.field_0x02 = (unsigned char)config->field_0x00;
    header.count64 = config->count64;
    header.count24 = config->count24;
    header.count60 = config->count60;
    header.count40 = config->count40;
    header.count20 = config->count20;
    header.packedFields =
        (((unsigned char)config->field_0x54 & 0xF) * 0x10) |
        ((unsigned char)config->field_0x58 & 0xF);
    memcpy(header.bytes, config->bytes, sizeof(header.bytes));
    memcpy(header.values, config->values, sizeof(header.values));
    RwMemLittleEndian16(header.values, sizeof(header.values));
    if (RwStreamWrite(stream, &header, sizeof(header)) == 0) {
        return 0;
    }

    for (index = 0; index < config->count24; index++) {
        const RpGameCubeMTEntry24* entry = &config->entries24[index];
        RpGameCubeMTStreamEntry24 encoded;

        encoded.value[0] = (unsigned char)entry->value[0];
        encoded.value[1] = (unsigned char)entry->value[1];
        encoded.value[2] = (unsigned char)entry->value[2];
        encoded.value[3] = (unsigned char)entry->value[3];
        encoded.value[4] = (unsigned char)entry->value[4];
        encoded.reserved[0] = 0;
        encoded.reserved[1] = 0;
        encoded.reserved[2] = 0;
        encoded.field_0x08 = entry->field_0x14;
        encoded.field_0x0A = entry->field_0x16;
        RwMemLittleEndian16(&encoded.field_0x08, 4);
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == 0) {
            return 0;
        }
    }

    for (index = 0; index < config->count64; index++) {
        const RpGameCubeMTEntry64* entry = &config->entries64[index];
        RpGameCubeMTStreamEntry64 encoded;

        encoded.value[0] = (unsigned char)entry->field_0x00;
        encoded.value[1] = (unsigned char)entry->field_0x04;
        encoded.value[2] = (unsigned char)entry->field_0x08;
        encoded.value[3] = (unsigned char)entry->field_0x0C;
        encoded.value[4] = (unsigned char)entry->field_0x10;
        encoded.value[5] = (unsigned char)entry->field_0x14;
        encoded.value[6] = (unsigned char)entry->field_0x18;
        encoded.value[7] = (unsigned char)entry->field_0x1C;
        encoded.value[8] = (unsigned char)entry->field_0x20;
        encoded.value[9] = (unsigned char)entry->field_0x24;
        encoded.value[10] = (unsigned char)entry->field_0x28;
        encoded.value[11] = (unsigned char)entry->field_0x2C;
        encoded.value[12] = (unsigned char)entry->field_0x30;
        encoded.value[13] = (unsigned char)entry->field_0x34;
        encoded.value[14] = (unsigned char)entry->field_0x38;
        encoded.value[15] = (unsigned char)entry->field_0x3C;
        encoded.value[16] = (unsigned char)entry->field_0x40;
        encoded.value[17] = (unsigned char)entry->field_0x44;
        encoded.value[18] = (unsigned char)entry->field_0x48;
        encoded.value[19] = (unsigned char)entry->field_0x4C;
        encoded.value[20] = (unsigned char)entry->field_0x50;
        encoded.value[21] = (unsigned char)entry->field_0x54;
        encoded.value[22] = (unsigned char)entry->field_0x58;
        encoded.value[23] = (unsigned char)entry->field_0x5C;
        encoded.field_0x18 = (unsigned int)entry->field_0x60;
        RwMemLittleEndian32(&encoded.field_0x18, sizeof(encoded.field_0x18));
        if ((entry->field_0x54 & 0x100) != 0) {
            encoded.value[23] |= 0x80;
        }
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == 0) {
            return 0;
        }
    }

    for (index = 0; index < config->count60; index++) {
        const RpGameCubeMTEntry60* entry = &config->entries60[index];
        RpGameCubeMTStreamEntry60 encoded;

        encoded.value[0] = (unsigned char)entry->value[0];
        encoded.value[1] = (unsigned char)entry->value[1];
        encoded.value[2] = (unsigned char)entry->value[2];
        encoded.reserved = 0;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == 0 ||
            RwStreamWriteReal(stream, entry->real, sizeof(entry->real)) ==
                0) {
            return 0;
        }
    }

    for (index = 0; index < config->count40; index++) {
        const RpGameCubeMTEntry40* entry = &config->entries40[index];
        RpGameCubeMTStreamEntry40 encoded;

        encoded.value[0] = (unsigned char)entry->value[0];
        encoded.value[1] = (unsigned char)entry->value[1];
        encoded.value[2] = (unsigned char)entry->value[2];
        encoded.field_0x03 = (signed char)entry->field_0x24;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == 0 ||
            RwStreamWriteReal(stream, entry->real, sizeof(entry->real)) ==
                0) {
            return 0;
        }
    }

    for (index = 0; index < config->count20; index++) {
        const RpGameCubeMTEntry20* entry = &config->entries20[index];
        RpGameCubeMTStreamEntry20 encoded;

        encoded.value[0] = (unsigned char)entry->value[0];
        encoded.value[1] = (unsigned char)entry->value[1];
        encoded.value[2] = (unsigned char)entry->value[2];
        encoded.value[3] = (unsigned char)entry->value[3];
        encoded.value[4] = (unsigned char)entry->value[4];
        encoded.reserved[0] = 0;
        encoded.reserved[1] = 0;
        encoded.reserved[2] = 0;
        if (RwStreamWrite(stream, &encoded, sizeof(encoded)) == 0) {
            return 0;
        }
    }
    return effect;
}


static RpMTEffect* GameCubeMTEffectStreamRead(
    RwStream* stream, int type, unsigned int version, int length)
{
    RpGameCubeMTStreamHeader header;
    RpMTEffect* effect;
    RpGameCubeMTEffectConfig* config;
    unsigned int streamSize;
    unsigned int index;

    streamSize = sizeof(header);
    if (RwStreamRead(stream, &header, streamSize) != streamSize) {
        return 0;
    }
    effect = RpGameCubeMTEffectCreate(
        header.count64, header.count24, header.count60,
        header.count20, header.count40);
    if (effect == 0) {
        return 0;
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
            return 0;
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
            return 0;
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
        entry->field_0x60 = (int)encoded.field_0x18;
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
                0) {
            RwEngineInstance->fpFree(effect);
            return 0;
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
                0) {
            RwEngineInstance->fpFree(effect);
            return 0;
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
            return 0;
        }
        entry->value[0] = encoded.value[0];
        entry->value[1] = encoded.value[1];
        entry->value[2] = encoded.value[2];
        entry->value[3] = encoded.value[3];
        entry->value[4] = encoded.value[4];
    }
    return effect;
}

int _rpGameCubeMTDataPluginAttach(void)
{
    int result = _rpMTEffectRegisterPlatform(
        6, GameCubeMTEffectStreamRead, GameCubeMTEffectStreamWrite,
        GameCubeMTEffectStreamGetSize, 0);
    return result;
}

RpMTEffect* RpGameCubeMTEffectCreate(
    unsigned int count64, unsigned int count24, unsigned int count60,
    unsigned int count20, unsigned int count40)
{
    unsigned int size = 0x8C;
    RpMTEffect* effect;
    RpGameCubeMTEffectConfig* config;
    unsigned char* next;

    size = ((size + 3) & ~3U) +
        count24 * sizeof(RpGameCubeMTEntry24);
    size = ((size + 3) & ~3U) +
        count64 * sizeof(RpGameCubeMTEntry64);
    size = ((size + 3) & ~3U) +
        count20 * sizeof(RpGameCubeMTEntry20);
    size = ((size + 3) & ~3U) +
        count60 * sizeof(RpGameCubeMTEntry60);
    size = ((size + 3) & ~3U) +
        count40 * sizeof(RpGameCubeMTEntry40);
    size = (size + 3) & ~3U;

    effect = RwEngineInstance->fpMalloc(size, 0x30129);
    if (effect == 0) {
        RwError error;

        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    memset(effect, 0, size);
    _rpMTEffectInit(effect, 6);
    config = (RpGameCubeMTEffectConfig*)((unsigned char*)effect + 0x30);
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

    next = (unsigned char*)config + sizeof(*config);
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
        (RpGameCubeMTEffectConfig*)((unsigned char*)effect + 0x30);
    return config;
}
