#include "rw/gamecube.h"

enum {
    rwGCNVA_POS = 9,
    rwGCNVA_NRM = 10,
    rwGCNVA_CLR0 = 11,
    rwGCNVA_CLR1 = 12,
    rwGCNVA_TEX0 = 13,
    rwGCNVA_MAX = 21,
    rwGCNINDEX8 = 2,
    rwGCNINDEX16 = 3
};

/* Retail retains one unused boolean normalization when position data is absent.
 * The clean source omits that three-instruction debug/macro residue. */
void _rwGCNVertexBufferFill(const RwGameCubeVertexDescriptor* format,
                            const RwGameCubeVertexStreams* streams,
                            const RwGameCubeVertexData* data,
                            RwBool compressedNormals, void* remap)
{
    RwUInt32 streamIndex = 0;
    RwUInt32 attribute = rwGCNVA_POS;
    RwUInt32 descriptor;
    RwUInt32 type;
    RwUInt32 positionPresent;
    RwUInt32 normalMode;
    RwReal scale;

    while (attribute < rwGCNVA_MAX) {
        switch (attribute) {
        case rwGCNVA_POS:
            descriptor = (format->vcdLo & (3U << 9)) >> 9;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                positionPresent = format->vatA & 1;
                if (positionPresent == 1) {
                    type = (format->vatA >> 1) & 7;
                    scale = (RwReal)(1 << ((format->vatA >> 4) & 0x1F));
                    _rwGCNVtxFmtInstPos3D(
                        streams->streams[streamIndex].data,
                        (const RwV3d *)data->source[attribute], type,
                        data->counts[attribute],
                        streams->streams[streamIndex].stride, remap, scale);
                    streamIndex++;
                }
            }
            break;

        case rwGCNVA_NRM:
            descriptor = (format->vcdLo & (3U << 11)) >> 11;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                normalMode = (format->vatA >> 9) & 1;
                type = (format->vatA >> 10) & 7;
                if (normalMode == 1) {
                    if (compressedNormals == FALSE)
                        _rwGCNVtxFmtInstNBT(
                            streams->streams[streamIndex].data,
                            (const RwV3d *)data->source[attribute], type,
                            data->counts[attribute],
                            streams->streams[streamIndex].stride);
                    else
                        _rwGCNVtxFmtInstNBTCmp(
                            streams->streams[streamIndex].data,
                            data->source[attribute], type,
                            data->counts[attribute],
                            streams->streams[streamIndex].stride);
                } else {
                    if (compressedNormals == FALSE)
                        _rwGCNVtxFmtInstNrm(
                            streams->streams[streamIndex].data,
                            (const RwV3d *)data->source[attribute], type,
                            data->counts[attribute],
                            streams->streams[streamIndex].stride);
                    else
                        _rwGCNVtxFmtInstNrmCmp(
                            streams->streams[streamIndex].data,
                            data->source[attribute], type,
                            data->counts[attribute],
                            streams->streams[streamIndex].stride);
                }
                streamIndex++;
            }
            break;

        case rwGCNVA_CLR0:
        case rwGCNVA_CLR1: {
            descriptor =
                (format->vcdLo >>
                 (13 + (attribute - rwGCNVA_CLR0) * 2)) & 3;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type =
                    (format->vatA >>
                     (14 + (attribute - rwGCNVA_CLR0) * 4)) & 7;
                _rwGCNVtxFmtInstClr(
                    streams->streams[streamIndex].data,
                    (const RwRGBA *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride);
                streamIndex++;
            }
            break;
        }

        default: {
            break;
        }
        case 13:
            descriptor = format->vcdHi & 3;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatA >> 22) & 7;
                scale = (RwReal)(1 << ((format->vatA >> 25) & 0x1F));
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 14:
            descriptor = (format->vcdHi & (3U << 2)) >> 2;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatB >> 1) & 7;
                scale = (RwReal)((1 << (format->vatB >> 4)) & 0x1F);
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 15:
            descriptor = (format->vcdHi & (3U << 4)) >> 4;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatB >> 10) & 7;
                scale = (RwReal)((1 << (format->vatB >> 13)) & 0x1F);
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 16:
            descriptor = (format->vcdHi & (3U << 6)) >> 6;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatB >> 19) & 7;
                scale = (RwReal)((1 << (format->vatB >> 22)) & 0x1F);
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 17:
            descriptor = (format->vcdHi & (3U << 8)) >> 8;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatB >> 28) & 7;
                scale = (RwReal)(1 << (format->vatC & 0x1F));
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 18:
            descriptor = (format->vcdHi & (3U << 10)) >> 10;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatC >> 6) & 7;
                scale = (RwReal)(1 << ((format->vatC >> 9) & 0x1F));
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 19:
            descriptor = (format->vcdHi & (3U << 12)) >> 12;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatC >> 15) & 7;
                scale = (RwReal)(1 << ((format->vatC >> 18) & 0x1F));
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        case 20:
            descriptor = (format->vcdHi & (3U << 14)) >> 14;
            if (descriptor == rwGCNINDEX8 || descriptor == rwGCNINDEX16) {
                type = (format->vatC >> 24) & 7;
                scale = (RwReal)(1 << ((format->vatC >> 27) & 0x1F));
                _rwGCNVtxFmtInstTex(
                    streams->streams[streamIndex].data,
                    (const RwTexCoords *)data->source[attribute], type,
                    data->counts[attribute],
                    streams->streams[streamIndex].stride, scale);
                streamIndex++;
            }
            break;
        }
        attribute++;
    }
}
