#include "rw/gamecube.h"

void _rwGCNVertexBufferFill(const RwGameCubeVertexDescriptor* format,
                            const RwGameCubeVertexStreams* streams,
                            const RwGameCubeVertexData* data,
                            int compressedNormals, void* remap)
{
    unsigned int streamIndex = 0;
    unsigned int attribute = 9;
    unsigned int descriptor;
    unsigned int type;
    unsigned int positionPresent;
    unsigned int normalMode;
    float scale;

    while (attribute < 21) {
        switch (attribute) {
        case 9:
            descriptor = (format->vcdLo & (3U << 9)) >> 9;
            if (descriptor == 2 || descriptor == 3) {
                positionPresent = format->vatA & 1;
                if (positionPresent == 1) {
                    type = (format->vatA >> 1) & 7;
                    scale = (float)(1 << ((format->vatA >> 4) & 0x1F));
                    _rwGCNVtxFmtInstPos3D(
                        streams->streams[streamIndex].data,
                        (const RwV3d *)data->source[attribute], type,
                        data->counts[attribute],
                        streams->streams[streamIndex].stride, remap, scale);
                    streamIndex++;
                }
            }
            break;

        case 10:
            descriptor = (format->vcdLo & (3U << 11)) >> 11;
            if (descriptor == 2 || descriptor == 3) {
                normalMode = (format->vatA >> 9) & 1;
                type = (format->vatA >> 10) & 7;
                if (normalMode == 1) {
                    if (compressedNormals == 0)
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
                    if (compressedNormals == 0)
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

        case 11:
        case 12: {
            descriptor =
                (format->vcdLo >>
                 (13 + (attribute - 11) * 2)) & 3;
            if (descriptor == 2 || descriptor == 3) {
                type =
                    (format->vatA >>
                     (14 + (attribute - 11) * 4)) & 7;
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatA >> 22) & 7;
                scale = (float)(1 << ((format->vatA >> 25) & 0x1F));
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatB >> 1) & 7;
                scale = (float)((1 << (format->vatB >> 4)) & 0x1F);
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatB >> 10) & 7;
                scale = (float)((1 << (format->vatB >> 13)) & 0x1F);
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatB >> 19) & 7;
                scale = (float)((1 << (format->vatB >> 22)) & 0x1F);
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatB >> 28) & 7;
                scale = (float)(1 << (format->vatC & 0x1F));
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatC >> 6) & 7;
                scale = (float)(1 << ((format->vatC >> 9) & 0x1F));
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatC >> 15) & 7;
                scale = (float)(1 << ((format->vatC >> 18) & 0x1F));
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
            if (descriptor == 2 || descriptor == 3) {
                type = (format->vatC >> 24) & 7;
                scale = (float)(1 << ((format->vatC >> 27) & 0x1F));
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
