#include <dolphin/gx.h>
#include "__gx.h"

extern f32 cosf(f32 value);

typedef struct GXLightObjPriv {
    u32 reserved[3], color;
    f32 a[3], k[3], position[3], direction[3];
} GXLightObjPriv;
typedef char GXLightObjPriv_size[(sizeof(GXLightObjPriv) == 0x40) ? 1 : -1];
#define GXCOLOR_AS_U32(color) (*(u32*)&(color))

void GXInitLightAttn(GXLightObj* light, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2) {
    GXLightObjPriv* o = (GXLightObjPriv*)light;
    o->a[0]=a0; o->a[1]=a1; o->a[2]=a2; o->k[0]=k0; o->k[1]=k1; o->k[2]=k2;
}
void GXInitLightAttnA(GXLightObj* light, f32 a0, f32 a1, f32 a2) {
    GXLightObjPriv* o = (GXLightObjPriv*)light;
    o->a[0]=a0; o->a[1]=a1; o->a[2]=a2;
}
void GXInitLightSpot(GXLightObj* light, f32 cutoff, GXSpotFn function) {
    f32 a0, a1, a2;
    f32 r;
    f32 d;
    f32 cr;
    GXLightObjPriv* o;
    o = (GXLightObjPriv*)light;
    if (cutoff <= 0.0f || cutoff > 90.0f)
        function = GX_SP_OFF;
    r = (3.1415927f * cutoff) / 180.0f;
    cr = cosf(r);
    switch (function) {
    case GX_SP_FLAT: a0=-1000.0f*cr; a1=1000.0f; a2=0.0f; break;
    case GX_SP_COS: a1=1.0f/(1.0f-cr); a0=-cr*a1; a2=0.0f; break;
    case GX_SP_COS2: a2=1.0f/(1.0f-cr); a0=0.0f; a1=-cr*a2; break;
    case GX_SP_SHARP:
        d=1.0f/((1.0f-cr)*(1.0f-cr));
        a0=(cr*(cr-2.0f))*d; a1=2.0f*d; a2=-d; break;
    case GX_SP_RING1:
        d=1.0f/((1.0f-cr)*(1.0f-cr));
        a2=-4.0f*d; a0=a2*cr; a1=(4.0f*(1.0f+cr))*d; break;
    case GX_SP_RING2:
        d=1.0f/((1.0f-cr)*(1.0f-cr));
        a0=1.0f-((2.0f*cr*cr)*d); a1=(4.0f*cr)*d; a2=-2.0f*d; break;
    case GX_SP_OFF:
    default: a0=1.0f; a1=0.0f; a2=0.0f; break;
    }
    o->a[0]=a0; o->a[1]=a1; o->a[2]=a2;
}
void GXInitLightDistAttn(GXLightObj* light, f32 distance, f32 brightness, GXDistAttnFn function) {
    f32 k0, k1, k2;
    GXLightObjPriv* o;
    o = (GXLightObjPriv*)light;
    if (distance < 0.0f) function=GX_DA_OFF;
    if (brightness <= 0.0f || brightness >= 1.0f) function=GX_DA_OFF;
    switch (function) {
    case GX_DA_GENTLE: k0=1.0f; k1=(1.0f-brightness)/(brightness*distance); k2=0.0f; break;
    case GX_DA_MEDIUM:
        k0=1.0f; k1=(0.5f*(1.0f-brightness))/(brightness*distance);
        k2=(0.5f*(1.0f-brightness))/(brightness*distance*distance); break;
    case GX_DA_STEEP: k0=1.0f; k1=0.0f; k2=(1.0f-brightness)/(brightness*distance*distance); break;
    case GX_DA_OFF:
    default: k0=1.0f; k1=0.0f; k2=0.0f; break;
    }
    o->k[0]=k0; o->k[1]=k1; o->k[2]=k2;
}
void GXInitLightPos(GXLightObj* light, f32 x, f32 y, f32 z) {
    GXLightObjPriv* o=(GXLightObjPriv*)light; o->position[0]=x; o->position[1]=y; o->position[2]=z;
}
void GXInitLightDir(GXLightObj* light, f32 x, f32 y, f32 z) {
    GXLightObjPriv* o=(GXLightObjPriv*)light; o->direction[0]=-x; o->direction[1]=-y; o->direction[2]=-z;
}
void GXInitLightColor(GXLightObj* light, GXColor color) {
    ((GXLightObjPriv*)light)->color=GXCOLOR_AS_U32(color);
}

static inline void PushLightScalar(const GXLightObjPriv* o) {
    GX_WRITE_U32(0); GX_WRITE_U32(0); GX_WRITE_U32(0); GX_WRITE_U32(o->color);
    GX_WRITE_F32(o->a[0]); GX_WRITE_F32(o->a[1]); GX_WRITE_F32(o->a[2]);
    GX_WRITE_F32(o->k[0]); GX_WRITE_F32(o->k[1]); GX_WRITE_F32(o->k[2]);
    GX_WRITE_F32(o->position[0]); GX_WRITE_F32(o->position[1]); GX_WRITE_F32(o->position[2]);
    GX_WRITE_F32(o->direction[0]); GX_WRITE_F32(o->direction[1]); GX_WRITE_F32(o->direction[2]);
}
void GXLoadLightObjImm(const GXLightObj* light, GXLightID id) {
    u32 index=31-__cntlzw(id), address;
    index &= 7; address=index*0x10+0x600;
    GX_WRITE_U8(0x10); GX_WRITE_U32(address|0xF0000);
    PushLightScalar((const GXLightObjPriv*)light);
    __GXData->bpSentNot=1;
}

void GXSetChanAmbColor(GXChannelID channel, GXColor color) {
    u32 reg;
    u32 rgb;
    u32 index;

    switch (channel) {
    case GX_COLOR0:
        reg = __GXData->ambColor[GX_COLOR0];
        rgb = GXCOLOR_AS_U32(color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        index = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->ambColor[GX_COLOR1];
        rgb = GXCOLOR_AS_U32(color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        index = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->ambColor[GX_COLOR0];
        SET_REG_FIELD(0, reg, 8, 0, color.a);
        index = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->ambColor[GX_COLOR1];
        SET_REG_FIELD(0, reg, 8, 0, color.a);
        index = 1;
        break;
    case GX_COLOR0A0:
        reg = GXCOLOR_AS_U32(color);
        index = 0;
        break;
    case GX_COLOR1A1:
        reg = GXCOLOR_AS_U32(color);
        index = 1;
        break;
    default:
        return;
    }

    GX_WRITE_XF_REG(index + 10, reg);
    __GXData->bpSentNot = 1;
    __GXData->ambColor[index] = reg;
}
void GXSetChanMatColor(GXChannelID channel, GXColor color) {
    u32 reg;
    u32 rgb;
    u32 index;

    switch (channel) {
    case GX_COLOR0:
        reg = __GXData->matColor[GX_COLOR0];
        rgb = GXCOLOR_AS_U32(color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        index = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->matColor[GX_COLOR1];
        rgb = GXCOLOR_AS_U32(color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        index = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->matColor[GX_COLOR0];
        SET_REG_FIELD(0, reg, 8, 0, color.a);
        index = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->matColor[GX_COLOR1];
        SET_REG_FIELD(0, reg, 8, 0, color.a);
        index = 1;
        break;
    case GX_COLOR0A0:
        reg = GXCOLOR_AS_U32(color);
        index = 0;
        break;
    case GX_COLOR1A1:
        reg = GXCOLOR_AS_U32(color);
        index = 1;
        break;
    default:
        return;
    }

    GX_WRITE_XF_REG(index + 12, reg);
    __GXData->bpSentNot = 1;
    __GXData->matColor[index] = reg;
}
void GXSetNumChans(u8 count) {
    SET_REG_FIELD(0,__GXData->genMode,3,4,count); GX_WRITE_XF_REG(9,count); __GXData->dirtyState|=4;
}
void GXSetChanCtrl(GXChannelID channel, GXBool enable, GXColorSrc ambient,
                   GXColorSrc material, unsigned int lights, GXDiffuseFn diffuse, GXAttnFn attenuation) {
    u32 reg=0, index=channel&3;
    SET_REG_FIELD(0,reg,1,1,enable); SET_REG_FIELD(0,reg,1,0,material);
    SET_REG_FIELD(0,reg,1,6,ambient); SET_REG_FIELD(0,reg,2,7,attenuation==0?0:diffuse);
    SET_REG_FIELD(0,reg,1,9,attenuation!=2); SET_REG_FIELD(0,reg,1,10,attenuation!=0);
    SET_REG_FIELD(0,reg,4,2,lights&0xF); SET_REG_FIELD(0,reg,4,11,(lights>>4)&0xF);
    GX_WRITE_XF_REG(index+14,reg);
    if (channel==GX_COLOR0A0) GX_WRITE_XF_REG(16,reg);
    else if (channel==GX_COLOR1A1) GX_WRITE_XF_REG(17,reg);
    __GXData->bpSentNot=1;
}
