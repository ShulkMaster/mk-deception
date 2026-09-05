#include "cri/adx_basic.h"
#include "cri/adx_dcd.h"
#include "runtime/cstdio.h"
#include "runtime/cstring.h"

typedef struct AhxDecoder AhxDecoder;
typedef struct AdxBasicAhx AdxBasicAhx;
typedef struct AdxBasicDecoderExt AdxBasicDecoderExt;
typedef void (*AdxDecodeNotify)(void*, int, int);
typedef void (*AhxSetExtFunc)(AhxDecoder*, short*);
typedef void (*Pl2EncodeFunc)(AdxBasicDecoderExt*, short, short*, short*);
typedef void (*Pl2ResetFunc)(AdxBasicDecoderExt*);

struct AdxBasicDecoderExt {
    AdxBasicDecoder base;
    short default_key[3];
    short snapshot_key[3];
    short delay_left[2];
    short delay_right[2];
    AhxDecoder* ahx_decoder;
    int ahx_max_decoded_samples;
    int ahx_max_decoded_blocks;
    int ainf_length;
    unsigned char ainf[16];
    short default_out_volume;
    short default_pan[2];
    unsigned char reserved_DA[2];
    void* pl2_context;
    unsigned char reserved_E0[8];
    int last_notified_data_length;
    int field_EC;
    AdxDecodeNotify notify;
    void* notify_object;
};

typedef struct AdxEncryptionKey {
    short type;
    short state;
    short multiplier;
    short increment;
} AdxEncryptionKey;

typedef struct AdxBasicDecoderProgressView {
    unsigned char reserved_00[0x74];
    int state;
} AdxBasicDecoderProgressView;

typedef char AdxBasicDecoderExtSizeCheck[
    sizeof(AdxBasicDecoderExt) == 0xF8 ? 1 : -1];
typedef char AdxBasicDecoderProgressViewSizeCheck[
    sizeof(AdxBasicDecoderProgressView) == 0x78 ? 1 : -1];

extern void ADXB_ExecOneAhx(AdxBasicAhx*);
extern void ADXB_ExecOneAiff(AdxBasicDecoder*);
extern void ADXB_ExecOneAu(AdxBasicDecoder*);
extern void ADXB_ExecOneWav(AdxBasicDecoder*);
extern int ADXB_CheckAiff(const signed char*);
extern int ADXB_CheckAu(const signed char*);
extern int ADXB_CheckWav(const signed char*);
extern int ADXB_DecodeHeaderAiff(AdxBasicDecoder*, signed char*, int);
extern int ADXB_DecodeHeaderAu(AdxBasicDecoder*, signed char*, int);
extern int ADXB_DecodeHeaderWav(AdxBasicDecoder*, signed char*, int);
extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void ADXERR_CallErrFunc2(const char*, const char*);

static int skg_init_count = 0;
static void (*skg_err_func)(void*) = 0;
static void* skg_err_obj = 0;
AhxSetExtFunc ahxsetextfunc = 0;
Pl2EncodeFunc pl2encodefunc = 0;
Pl2ResetFunc pl2resetfunc = 0;
static short adxb_def_k0 = 0;
static short adxb_def_km = 0;
static short adxb_def_ka = 0;
AdxBasicDecoderExt adxb_obj[16];

static const char skg_version[] =
    "\nSKG/GC Ver.0.64 Build:Sep  3 2004 17:49:16\n";
const unsigned short skg_prim_tbl[1024] = {
    0x401B, 0x4021, 0x4025, 0x402B, 0x4031, 0x403F, 0x4043, 0x4045,
    0x405D, 0x4061, 0x4067, 0x406D, 0x4087, 0x4091, 0x40A3, 0x40A9,
    0x40B1, 0x40B7, 0x40BD, 0x40DB, 0x40DF, 0x40EB, 0x40F7, 0x40F9,
    0x4109, 0x410B, 0x4111, 0x4115, 0x4121, 0x4133, 0x4135, 0x413B,
    0x413F, 0x4159, 0x4165, 0x416B, 0x4177, 0x417B, 0x4193, 0x41AB,
    0x41B7, 0x41BD, 0x41BF, 0x41CB, 0x41E7, 0x41EF, 0x41F3, 0x41F9,
    0x4205, 0x4207, 0x4219, 0x421F, 0x4223, 0x4229, 0x422F, 0x4243,
    0x4253, 0x4255, 0x425B, 0x4261, 0x4273, 0x427D, 0x4283, 0x4285,
    0x4289, 0x4291, 0x4297, 0x429D, 0x42B5, 0x42C5, 0x42CB, 0x42D3,
    0x42DD, 0x42E3, 0x42F1, 0x4307, 0x430F, 0x431F, 0x4325, 0x4327,
    0x4333, 0x4337, 0x4339, 0x434F, 0x4357, 0x4369, 0x438B, 0x438D,
    0x4393, 0x43A5, 0x43A9, 0x43AF, 0x43B5, 0x43BD, 0x43C7, 0x43CF,
    0x43E1, 0x43E7, 0x43EB, 0x43ED, 0x43F1, 0x43F9, 0x4409, 0x440B,
    0x4417, 0x4423, 0x4429, 0x443B, 0x443F, 0x4445, 0x444B, 0x4451,
    0x4453, 0x4459, 0x4465, 0x446F, 0x4483, 0x448F, 0x44A1, 0x44A5,
    0x44AB, 0x44AD, 0x44BD, 0x44BF, 0x44C9, 0x44D7, 0x44DB, 0x44F9,
    0x44FB, 0x4505, 0x4511, 0x4513, 0x452B, 0x4531, 0x4541, 0x4549,
    0x4553, 0x4555, 0x4561, 0x4577, 0x457D, 0x457F, 0x458F, 0x45A3,
    0x45AD, 0x45AF, 0x45BB, 0x45C7, 0x45D9, 0x45E3, 0x45EF, 0x45F5,
    0x45F7, 0x4601, 0x4603, 0x4609, 0x4613, 0x4625, 0x4627, 0x4633,
    0x4639, 0x463D, 0x4643, 0x4645, 0x465D, 0x4679, 0x467B, 0x467F,
    0x4681, 0x468B, 0x468D, 0x469D, 0x46A9, 0x46B1, 0x46C7, 0x46C9,
    0x46CF, 0x46D3, 0x46D5, 0x46DF, 0x46E5, 0x46F9, 0x4705, 0x470F,
    0x4717, 0x4723, 0x4729, 0x472F, 0x4735, 0x4739, 0x474B, 0x474D,
    0x4751, 0x475D, 0x476F, 0x4771, 0x477D, 0x4783, 0x4787, 0x4789,
    0x4799, 0x47A5, 0x47B1, 0x47BF, 0x47C3, 0x47CB, 0x47DD, 0x47E1,
    0x47ED, 0x47FB, 0x4801, 0x4807, 0x480B, 0x4813, 0x4819, 0x481D,
    0x4831, 0x483D, 0x4847, 0x4855, 0x4859, 0x485B, 0x486B, 0x486D,
    0x4879, 0x4897, 0x489B, 0x48A1, 0x48B9, 0x48CD, 0x48E5, 0x48EF,
    0x48F7, 0x4903, 0x490D, 0x4919, 0x491F, 0x492B, 0x4937, 0x493D,
    0x4945, 0x4955, 0x4963, 0x4969, 0x496D, 0x4973, 0x4997, 0x49AB,
    0x49B5, 0x49D3, 0x49DF, 0x49E1, 0x49E5, 0x49E7, 0x4A03, 0x4A0F,
    0x4A1D, 0x4A23, 0x4A39, 0x4A41, 0x4A45, 0x4A57, 0x4A5D, 0x4A6B,
    0x4A7D, 0x4A81, 0x4A87, 0x4A89, 0x4A8F, 0x4AB1, 0x4AC3, 0x4AC5,
    0x4AD5, 0x4ADB, 0x4AED, 0x4AEF, 0x4B07, 0x4B0B, 0x4B0D, 0x4B13,
    0x4B1F, 0x4B25, 0x4B31, 0x4B3B, 0x4B43, 0x4B49, 0x4B59, 0x4B65,
    0x4B6D, 0x4B77, 0x4B85, 0x4BAD, 0x4BB3, 0x4BB5, 0x4BBB, 0x4BBF,
    0x4BCB, 0x4BD9, 0x4BDD, 0x4BDF, 0x4BE3, 0x4BE5, 0x4BE9, 0x4BF1,
    0x4BF7, 0x4C01, 0x4C07, 0x4C0D, 0x4C0F, 0x4C15, 0x4C1B, 0x4C21,
    0x4C2D, 0x4C33, 0x4C4B, 0x4C55, 0x4C57, 0x4C61, 0x4C67, 0x4C73,
    0x4C79, 0x4C7F, 0x4C8D, 0x4C93, 0x4C99, 0x4CCD, 0x4CE1, 0x4CE7,
    0x4CF1, 0x4CF3, 0x4CFD, 0x4D05, 0x4D0F, 0x4D1B, 0x4D27, 0x4D29,
    0x4D2F, 0x4D33, 0x4D41, 0x4D51, 0x4D59, 0x4D65, 0x4D6B, 0x4D81,
    0x4D83, 0x4D8D, 0x4D95, 0x4D9B, 0x4DB1, 0x4DB3, 0x4DC9, 0x4DCF,
    0x4DD7, 0x4DE1, 0x4DED, 0x4DF9, 0x4DFB, 0x4E05, 0x4E0B, 0x4E17,
    0x4E19, 0x4E1D, 0x4E2B, 0x4E35, 0x4E37, 0x4E3D, 0x4E4F, 0x4E53,
    0x4E5F, 0x4E67, 0x4E79, 0x4E85, 0x4E8B, 0x4E91, 0x4E95, 0x4E9B,
    0x4EA1, 0x4EAF, 0x4EB3, 0x4EB5, 0x4EC1, 0x4ECD, 0x4ED1, 0x4ED7,
    0x4EE9, 0x4EFB, 0x4F07, 0x4F09, 0x4F19, 0x4F25, 0x4F2D, 0x4F3F,
    0x4F49, 0x4F63, 0x4F67, 0x4F6D, 0x4F75, 0x4F7B, 0x4F81, 0x4F85,
    0x4F87, 0x4F91, 0x4FA5, 0x4FA9, 0x4FAF, 0x4FB7, 0x4FBB, 0x4FCF,
    0x4FD9, 0x4FDB, 0x4FFD, 0x4FFF, 0x5003, 0x501B, 0x501D, 0x5029,
    0x5035, 0x503F, 0x5045, 0x5047, 0x5053, 0x5071, 0x5077, 0x5083,
    0x5093, 0x509F, 0x50A1, 0x50B7, 0x50C9, 0x50D5, 0x50E3, 0x50ED,
    0x50EF, 0x50FB, 0x5107, 0x510B, 0x510D, 0x5111, 0x5117, 0x5123,
    0x5125, 0x5135, 0x5147, 0x5149, 0x5171, 0x5179, 0x5189, 0x518F,
    0x5197, 0x51A1, 0x51A3, 0x51A7, 0x51B9, 0x51C1, 0x51CB, 0x51D3,
    0x51DF, 0x51E3, 0x51F5, 0x51F7, 0x5209, 0x5213, 0x5215, 0x5219,
    0x521B, 0x521F, 0x5227, 0x5243, 0x5245, 0x524B, 0x5261, 0x526D,
    0x5273, 0x5281, 0x5293, 0x5297, 0x529D, 0x52A5, 0x52AB, 0x52B1,
    0x52BB, 0x52C3, 0x52C7, 0x52C9, 0x52DB, 0x52E5, 0x52EB, 0x52FF,
    0x5315, 0x531D, 0x5323, 0x5341, 0x5345, 0x5347, 0x534B, 0x535D,
    0x5363, 0x5381, 0x5383, 0x5387, 0x538F, 0x5395, 0x5399, 0x539F,
    0x53AB, 0x53B9, 0x53DB, 0x53E9, 0x53EF, 0x53F3, 0x53F5, 0x53FB,
    0x53FF, 0x540D, 0x5411, 0x5413, 0x5419, 0x5435, 0x5437, 0x543B,
    0x5441, 0x5449, 0x5453, 0x5455, 0x545F, 0x5461, 0x546B, 0x546D,
    0x5471, 0x548F, 0x5491, 0x549D, 0x54A9, 0x54B3, 0x54C5, 0x54D1,
    0x54DF, 0x54E9, 0x54EB, 0x54F7, 0x54FD, 0x5507, 0x550D, 0x551B,
    0x5527, 0x552B, 0x5539, 0x553D, 0x554F, 0x5551, 0x555B, 0x5563,
    0x5567, 0x556F, 0x5579, 0x5585, 0x5597, 0x55A9, 0x55B1, 0x55B7,
    0x55C9, 0x55D9, 0x55E7, 0x55ED, 0x55F3, 0x55FD, 0x560B, 0x560F,
    0x5615, 0x5617, 0x5623, 0x562F, 0x5633, 0x5639, 0x563F, 0x564B,
    0x564D, 0x565D, 0x565F, 0x566B, 0x5671, 0x5675, 0x5683, 0x5689,
    0x568D, 0x568F, 0x569B, 0x56AD, 0x56B1, 0x56D5, 0x56E7, 0x56F3,
    0x56FF, 0x5701, 0x5705, 0x5707, 0x570B, 0x5713, 0x571F, 0x5723,
    0x5747, 0x574D, 0x575F, 0x5761, 0x576D, 0x5777, 0x577D, 0x5789,
    0x57A1, 0x57A9, 0x57AF, 0x57B5, 0x57C5, 0x57D1, 0x57D3, 0x57E5,
    0x57EF, 0x5803, 0x580D, 0x580F, 0x5815, 0x5827, 0x582B, 0x582D,
    0x5855, 0x585B, 0x585D, 0x586D, 0x586F, 0x5873, 0x587B, 0x588D,
    0x5897, 0x58A3, 0x58A9, 0x58AB, 0x58B5, 0x58BD, 0x58C1, 0x58C7,
    0x58D3, 0x58D5, 0x58DF, 0x58F1, 0x58F9, 0x58FF, 0x5903, 0x5917,
    0x591B, 0x5921, 0x5945, 0x594B, 0x594D, 0x5957, 0x595D, 0x5975,
    0x597B, 0x5989, 0x5999, 0x599F, 0x59B1, 0x59B3, 0x59BD, 0x59D1,
    0x59DB, 0x59E3, 0x59E9, 0x59ED, 0x59F3, 0x59F5, 0x59FF, 0x5A01,
    0x5A0D, 0x5A11, 0x5A13, 0x5A17, 0x5A1F, 0x5A29, 0x5A2F, 0x5A3B,
    0x5A4D, 0x5A5B, 0x5A67, 0x5A77, 0x5A7F, 0x5A85, 0x5A95, 0x5A9D,
    0x5AA1, 0x5AA3, 0x5AA9, 0x5ABB, 0x5AD3, 0x5AE5, 0x5AEF, 0x5AFB,
    0x5AFD, 0x5B01, 0x5B0F, 0x5B19, 0x5B1F, 0x5B25, 0x5B2B, 0x5B3D,
    0x5B49, 0x5B4B, 0x5B67, 0x5B79, 0x5B87, 0x5B97, 0x5BA3, 0x5BB1,
    0x5BC9, 0x5BD5, 0x5BEB, 0x5BF1, 0x5BF3, 0x5BFD, 0x5C05, 0x5C09,
    0x5C0B, 0x5C0F, 0x5C1D, 0x5C29, 0x5C2F, 0x5C33, 0x5C39, 0x5C47,
    0x5C4B, 0x5C4D, 0x5C51, 0x5C6F, 0x5C75, 0x5C77, 0x5C7D, 0x5C87,
    0x5C89, 0x5CA7, 0x5CBD, 0x5CBF, 0x5CC3, 0x5CC9, 0x5CD1, 0x5CD7,
    0x5CDD, 0x5CED, 0x5CF9, 0x5D05, 0x5D0B, 0x5D13, 0x5D17, 0x5D19,
    0x5D31, 0x5D3D, 0x5D41, 0x5D47, 0x5D4F, 0x5D55, 0x5D5B, 0x5D65,
    0x5D67, 0x5D6D, 0x5D79, 0x5D95, 0x5DA3, 0x5DA9, 0x5DAD, 0x5DB9,
    0x5DC1, 0x5DC7, 0x5DD3, 0x5DD7, 0x5DDD, 0x5DEB, 0x5DF1, 0x5DFD,
    0x5E07, 0x5E0D, 0x5E13, 0x5E1B, 0x5E21, 0x5E27, 0x5E2B, 0x5E2D,
    0x5E31, 0x5E39, 0x5E45, 0x5E49, 0x5E57, 0x5E69, 0x5E73, 0x5E75,
    0x5E85, 0x5E8B, 0x5E9F, 0x5EA5, 0x5EAF, 0x5EB7, 0x5EBB, 0x5ED9,
    0x5EFD, 0x5F09, 0x5F11, 0x5F27, 0x5F33, 0x5F35, 0x5F3B, 0x5F47,
    0x5F57, 0x5F5D, 0x5F63, 0x5F65, 0x5F77, 0x5F7B, 0x5F95, 0x5F99,
    0x5FA1, 0x5FB3, 0x5FBD, 0x5FC5, 0x5FCF, 0x5FD5, 0x5FE3, 0x5FE7,
    0x5FFB, 0x6011, 0x6023, 0x602F, 0x6037, 0x6053, 0x605F, 0x6065,
    0x606B, 0x6073, 0x6079, 0x6085, 0x609D, 0x60AD, 0x60BB, 0x60BF,
    0x60CD, 0x60D9, 0x60DF, 0x60E9, 0x60F5, 0x6109, 0x610F, 0x6113,
    0x611B, 0x612D, 0x6139, 0x614B, 0x6155, 0x6157, 0x615B, 0x616F,
    0x6179, 0x6187, 0x618B, 0x6191, 0x6193, 0x619D, 0x61B5, 0x61C7,
    0x61C9, 0x61CD, 0x61E1, 0x61F1, 0x61FF, 0x6209, 0x6217, 0x621D,
    0x6221, 0x6227, 0x623B, 0x6241, 0x624B, 0x6251, 0x6253, 0x625F,
    0x6265, 0x6283, 0x628D, 0x6295, 0x629B, 0x629F, 0x62A5, 0x62AD,
    0x62D5, 0x62D7, 0x62DB, 0x62DD, 0x62E9, 0x62FB, 0x62FF, 0x6305,
    0x630D, 0x6317, 0x631D, 0x632F, 0x6341, 0x6343, 0x634F, 0x635F,
    0x6367, 0x636D, 0x6371, 0x6377, 0x637D, 0x637F, 0x63B3, 0x63C1,
    0x63C5, 0x63D9, 0x63E9, 0x63EB, 0x63EF, 0x63F5, 0x6401, 0x6403,
    0x6409, 0x6415, 0x6421, 0x6427, 0x642B, 0x6439, 0x6443, 0x6449,
    0x644F, 0x645D, 0x6467, 0x6475, 0x6485, 0x648D, 0x6493, 0x649F,
    0x64A3, 0x64AB, 0x64C1, 0x64C7, 0x64C9, 0x64DB, 0x64F1, 0x64F7,
    0x64F9, 0x650B, 0x6511, 0x6521, 0x652F, 0x6539, 0x653F, 0x654B,
    0x654D, 0x6553, 0x6557, 0x655F, 0x6571, 0x657D, 0x658D, 0x658F,
    0x6593, 0x65A1, 0x65A5, 0x65AD, 0x65B9, 0x65C5, 0x65E3, 0x65F3,
    0x65FB, 0x65FF, 0x6601, 0x6607, 0x661D, 0x6629, 0x6631, 0x663B,
    0x6641, 0x6647, 0x664D, 0x665B, 0x6661, 0x6673, 0x667D, 0x6689,
    0x668B, 0x6695, 0x6697, 0x669B, 0x66B5, 0x66B9, 0x66C5, 0x66CD,
    0x66D1, 0x66E3, 0x66EB, 0x66F5, 0x6703, 0x6713, 0x6719, 0x671F,
    0x6727, 0x6731, 0x6737, 0x673F, 0x6745, 0x6751, 0x675B, 0x676F,
    0x6779, 0x6781, 0x6785, 0x6791, 0x67AB, 0x67BD, 0x67C1, 0x67CD,
    0x67DF, 0x67E5, 0x6803, 0x6809, 0x6811, 0x6817, 0x682D, 0x6839
};
static const char skg_hex_format[8] = "%08X";
static const char adxb_ahx_error[32] = "E1060101 ADXB_DecodeHeaderAdx: ";
static const char adxb_ahx_detail[36] =
    "can't play AHX data by this handle";
static const char skg_signature[12] = "CRI-MW";

static inline void ADXB_CopySamples(short* output, const short* extra,
                                    int count)
{
    for (; count > 0; count--) {
        *output++ = *extra++;
    }
}

#define ADXB_MAKE_ENCRYPTION_KEY(id, key)                                      \
    do {                                                                        \
        char key_text[16];                                                      \
        short factor0;                                                          \
        short factor1;                                                          \
        short factor2;                                                          \
        short factor3;                                                          \
        short factor4;                                                          \
        short factor5;                                                          \
        short factor6;                                                          \
        short factor7;                                                          \
        int value;                                                              \
        sprintf(key_text, skg_hex_format, (id));                                \
        if (skg_init_count == 0) {                                              \
            skg_init_count++;                                                   \
        }                                                                       \
        factor0 = skg_prim_tbl[0x80 + (signed char)key_text[0]];                \
        factor1 = skg_prim_tbl[0x80 + (signed char)key_text[1]];                \
        factor2 = skg_prim_tbl[0x80 + (signed char)key_text[2]];                \
        factor3 = skg_prim_tbl[0x80 + (signed char)key_text[3]];                \
        factor4 = skg_prim_tbl[0x80 + (signed char)key_text[4]];                \
        factor5 = skg_prim_tbl[0x80 + (signed char)key_text[5]];                \
        factor6 = skg_prim_tbl[0x80 + (signed char)key_text[6]];                \
        factor7 = skg_prim_tbl[0x80 + (signed char)key_text[7]];                \
        value = skg_prim_tbl[0x100];                                            \
        value = skg_prim_tbl[(value * factor0) % 1024];                         \
        value = skg_prim_tbl[(value * factor1) % 1024];                         \
        value = skg_prim_tbl[(value * factor2) % 1024];                         \
        value = skg_prim_tbl[(value * factor3) % 1024];                         \
        value = skg_prim_tbl[(value * factor4) % 1024];                         \
        value = skg_prim_tbl[(value * factor5) % 1024];                         \
        value = skg_prim_tbl[(value * factor6) % 1024];                         \
        (key).state = skg_prim_tbl[(value * factor7) % 1024];                   \
        value = skg_prim_tbl[0x200];                                            \
        value = skg_prim_tbl[(value * factor0) % 1024];                         \
        value = skg_prim_tbl[(value * factor1) % 1024];                         \
        value = skg_prim_tbl[(value * factor2) % 1024];                         \
        value = skg_prim_tbl[(value * factor3) % 1024];                         \
        value = skg_prim_tbl[(value * factor4) % 1024];                         \
        value = skg_prim_tbl[(value * factor5) % 1024];                         \
        value = skg_prim_tbl[(value * factor6) % 1024];                         \
        (key).multiplier = skg_prim_tbl[(value * factor7) % 1024];              \
        value = skg_prim_tbl[0x300];                                            \
        value = skg_prim_tbl[(value * factor0) % 1024];                         \
        value = skg_prim_tbl[(value * factor1) % 1024];                         \
        value = skg_prim_tbl[(value * factor2) % 1024];                         \
        value = skg_prim_tbl[(value * factor3) % 1024];                         \
        value = skg_prim_tbl[(value * factor4) % 1024];                         \
        value = skg_prim_tbl[(value * factor5) % 1024];                         \
        value = skg_prim_tbl[(value * factor6) % 1024];                         \
        (key).increment = skg_prim_tbl[(value * factor7) % 1024];               \
    } while (0)

void ADXB_ExecOneAdx(AdxBasicDecoderExt*);
void ADXB_EvokeDecode(AdxBasicDecoderExt*);
int ADXB_DecodeHeaderAdx(AdxBasicDecoderExt*, signed char*, int);
void ADXB_Destroy(AdxBasicDecoderExt*);
void adxb_DefAddWr(void*, int, int);
short* adxb_DefGetWr(void*, int*, int*, int*);

void ADXB_ExecHndl(AdxBasicDecoderExt* decoder)
{
    AdxBasicDecoder* base = &decoder->base;
    int delta;

    if (base->format_type == 0) {
        ADXB_ExecOneAdx(decoder);
    } else if (base->format_type == 10) {
        ADXB_ExecOneAhx((AdxBasicAhx*)decoder);
    } else if (base->format_type == 2) {
        ADXB_ExecOneSpsd(base);
    } else if (base->format_type == 3) {
        ADXB_ExecOneAiff(base);
    } else if (base->format_type == 4) {
        ADXB_ExecOneAu(base);
    } else if (base->format_type == 1) {
        ADXB_ExecOneWav(base);
    }
    if (decoder->notify != 0) {
        delta = base->decoded_data_length - decoder->last_notified_data_length;
        if (delta < 0) {
            delta = 0x7FFFFFFF - decoder->last_notified_data_length +
                    base->decoded_data_length;
        }
        decoder->notify(decoder->notify_object, delta,
                        base->channel_count * base->decoded_samples * 2);
        decoder->last_notified_data_length = base->decoded_data_length;
    }
}

void ADXB_ExecOneAdx(AdxBasicDecoderExt* decoder)
{
    AdxBasicDecoder* base = &decoder->base;
    AdxDecodeParams* params = &base->decode;
    int block_samples;
    int block_size;
    int loop_samples;
    int write_position;
    short* pcm_buffer;
    int pcm_size;
    int pcm_distance;
    int decoded_blocks;
    int decoded_samples;
    int trailing_samples;
    int i;

    if (base->status == 1 && ADXPD_GetStat(base->expander) == 0) {
        base->get_write_info(base->get_write_object, &params->write_position,
                             &params->room, &params->loop_samples);
        ADXB_EvokeDecode(decoder);
        base->status = 2;
    }
    if (base->status == 2) {
        ADXPD_ExecHndl(base->expander);
        if (ADXPD_GetStat(base->expander) == 3) {
            if (decoder->pl2_context != 0) {
                AdxXpnd* expander = base->expander;
                ADXCRS_Lock();
                for (i = 0; i < expander->params.num_blocks * 32; i++) {
                    short* left = &expander->params.output_left[i];
                    short* right = &expander->params.output_right[i];
                    pl2encodefunc(decoder, *left, left, right);
                }
                ADXCRS_Unlock();
            }

            block_samples = params->samples_per_block;
            block_size = params->block_size;
            loop_samples = params->loop_samples;
            pcm_buffer = params->pcm_buffer;
            pcm_size = base->pcm_size;
            pcm_distance = base->pcm_distance;
            write_position = params->write_position;

            decoded_samples = (loop_samples + block_samples - 1) % block_samples;
            trailing_samples = block_samples - 1 - decoded_samples;
            loop_samples = (loop_samples + block_samples - 1) / block_samples;
            decoded_blocks = ADXPD_GetNumBlk(base->expander);
            block_samples = decoded_blocks * block_samples /
                            params->channel_count;
            decoded_samples = decoded_blocks < loop_samples * params->channel_count
                                  ? block_samples
                                  : block_samples - trailing_samples;
            base->decoded_samples = decoded_samples;
            base->decoded_data_length = decoded_blocks * block_size;
            write_position += decoded_samples;
            if (write_position >= pcm_size) {
                write_position -= pcm_size;
                if (params->channel_count == 2 || decoder->pl2_context != 0) {
                    ADXB_CopySamples(pcm_buffer, &pcm_buffer[pcm_size],
                                     write_position);
                    ADXB_CopySamples(&pcm_buffer[pcm_distance],
                                     &pcm_buffer[pcm_distance + pcm_size],
                                     write_position);
                } else {
                    ADXB_CopySamples(pcm_buffer, &pcm_buffer[pcm_size],
                                     write_position);
                }
            }
            ADXPD_Reset(base->expander);
            base->add_write_info(base->add_write_object,
                                 base->decoded_data_length,
                                 base->decoded_samples);
            base->status = 3;
        }
    }
}

void ADXB_EvokeDecode(AdxBasicDecoderExt* decoder)
{
    AdxBasicDecoder* base = &decoder->base;
    AdxDecodeParams* params;
    int pcm_size;
    int loop_position;
    int delete_samples;
    int room;
    int end_blocks;
    int end_samples;
    int write_position;
    int input_blocks;
    int block_samples;
    int temp;

    params = &base->decode;
    input_blocks = params->input_blocks / params->channel_count;
    pcm_size = params->pcm_size;
    write_position = params->write_position;
    room = params->room;
    block_samples = params->samples_per_block;
    loop_position = params->loop_samples;

    delete_samples = (loop_position + block_samples - 1) / block_samples;
    end_samples = (loop_position + block_samples - 1) % block_samples;
    end_samples = block_samples - 1 - end_samples;
    end_blocks = (pcm_size - write_position + block_samples - 1) /
                 block_samples;
    temp = end_blocks * block_samples;
    if (delete_samples < end_blocks &&
        write_position + temp - end_samples < pcm_size) {
        end_blocks++;
    }
    if (loop_position < room) {
        room += end_samples;
    }
    temp = room / block_samples;
    if (input_blocks > temp) input_blocks = temp;
    if (input_blocks > delete_samples) input_blocks = delete_samples;
    if (input_blocks > end_blocks) input_blocks = end_blocks;

    if (params->channel_count == 2) {
        AdxXpnd* expander = base->expander;
        short* output = &params->pcm_buffer[params->write_position];
        ADXPD_EntrySte(expander, (const signed char*)params->input,
                       input_blocks * 2, output,
                       &output[params->pcm_distance]);
        ADXPD_Start(expander);
    } else if (decoder->pl2_context != 0) {
        AdxXpnd* expander = base->expander;
        short* output = &params->pcm_buffer[params->write_position];
        ADXPD_EntryPl2(expander, (const signed char*)params->input,
                       input_blocks, output, &output[params->pcm_distance]);
        ADXPD_Start(expander);
    } else {
        AdxXpnd* expander = base->expander;
        short* output = &params->pcm_buffer[params->write_position];
        ADXPD_EntryMono(expander, (const signed char*)params->input,
                        input_blocks, output, 0);
        ADXPD_Start(expander);
    }
}

int ADXB_GetDecNumSmpl(AdxBasicDecoderExt* decoder) { return decoder->base.decoded_samples; }
int ADXB_GetDecDtLen(AdxBasicDecoderExt* decoder) { return decoder->base.decoded_data_length; }

void ADXB_Reset(AdxBasicDecoderExt* decoder)
{
    if (decoder->base.status == 3) {
        ADXPD_Reset(decoder->base.expander);
        decoder->base.current_write_position = 0;
        decoder->base.status = 0;
    }
}

void ADXB_Stop(AdxBasicDecoderExt* decoder)
{
    if (decoder->pl2_context != 0) pl2resetfunc(decoder);
    ADXPD_Stop(decoder->base.expander);
    decoder->base.status = 0;
}

void ADXB_Start(AdxBasicDecoderExt* decoder)
{
    if (decoder->base.status == 0) decoder->base.status = 1;
}

void ADXB_EntryData(AdxBasicDecoderExt* decoder, signed char* input, int length)
{
    AdxBasicDecoder* base = &decoder->base;
    if (base->format_type == 0) {
        base->decode.input = (const unsigned short*)input;
        base->decode.input_blocks = length / base->block_length;
        ((AdxBasicDecoderProgressView*)base)->state = 0;
    } else {
        base->decode.input = (const unsigned short*)input;
        base->decode.input_blocks =
            length / ((base->bits_per_sample / 8) * base->channel_count);
        ((AdxBasicDecoderProgressView*)base)->state = 0;
    }
    base->decoded_samples = 0;
    base->decoded_data_length = 0;
    decoder->field_EC = 0;
    decoder->last_notified_data_length = 0;
}

int ADXB_GetStat(AdxBasicDecoderExt* decoder) { return decoder->base.status; }

void ADXB_RestoreSnapshot(AdxBasicDecoderExt* decoder)
{
    ADXPD_SetDly(decoder->base.expander, decoder->delay_left, decoder->delay_right);
    ADXPD_SetExtPrm(decoder->base.expander, decoder->snapshot_key[0],
                    decoder->snapshot_key[1], decoder->snapshot_key[2]);
}

void ADXB_TakeSnapshot(AdxBasicDecoderExt* decoder)
{
    ADXPD_GetDly(decoder->base.expander, decoder->delay_left, decoder->delay_right);
    ADXPD_GetExtPrm(decoder->base.expander, &decoder->snapshot_key[0],
                    &decoder->snapshot_key[1], &decoder->snapshot_key[2]);
}

short ADXB_GetDefPan(AdxBasicDecoderExt* decoder, int channel) { return decoder->default_pan[channel]; }
short ADXB_GetDefOutVol(AdxBasicDecoderExt* decoder) { return decoder->default_out_volume; }
int ADXB_GetAinfLen(AdxBasicDecoderExt* decoder) { return decoder->ainf_length; }
int ADXB_GetLpEndOfst(AdxBasicDecoderExt* decoder) { return decoder->base.loop_end_offset; }
int ADXB_GetLpEndPos(AdxBasicDecoderExt* decoder) { return decoder->base.loop_end_sample; }

int ADXB_GetLpStartOfst(AdxBasicDecoderExt* decoder)
{
    return decoder == 0 ? 0 : decoder->base.loop_start_offset;
}

int ADXB_GetLpStartPos(AdxBasicDecoderExt* decoder) { return decoder->base.loop_start_sample; }
int ADXB_GetNumLoop(AdxBasicDecoderExt* decoder) { return decoder->base.loop_count; }
int ADXB_GetTotalNumSmpl(AdxBasicDecoderExt* decoder) { return decoder->base.total_samples; }
int ADXB_GetBlkSmpl(AdxBasicDecoderExt* decoder) { return decoder->base.samples_per_block; }

int ADXB_GetOutBps(AdxBasicDecoderExt* decoder)
{
    AdxBasicDecoder* base = &decoder->base;
    if (base->format_type == 0) return 16;
    if (base->format_type == 2) {
        if (base->codec_type == 2) return 4;
        if (base->codec_type == 1) return 8;
        return 16;
    }
    if (base->format_type == 1) return base->codec_type == 2 ? 4 : 16;
    return 16;
}

int ADXB_GetNumChan(AdxBasicDecoderExt* decoder)
{
    if (decoder->base.channel_count == 1 && decoder->pl2_context != 0) return 2;
    return decoder->base.channel_count;
}

int ADXB_GetSfreq(AdxBasicDecoderExt* decoder) { return decoder->base.sample_rate; }
short ADXB_GetFormat(AdxBasicDecoderExt* decoder) { return decoder->base.format_type; }
short* ADXB_GetPcmBuf(AdxBasicDecoderExt* decoder) { return decoder->base.pcm_buffer; }

void ADXB_EntryGetWrFunc(AdxBasicDecoderExt* decoder, AdxGetWriteInfo function,
                         void* object)
{
    decoder->base.get_write_info = function;
    decoder->base.get_write_object = object;
}

int ADXB_DecodeHeader(AdxBasicDecoderExt* decoder, signed char* input, int length)
{
    if (*(unsigned short*)input == 0x8000) return ADXB_DecodeHeaderAdx(decoder, input, length);
    if (ADXB_CheckSpsd(input) != 0) return ADXB_DecodeHeaderSpsd(&decoder->base, input, length);
    if (ADXB_CheckWav(input) != 0) return ADXB_DecodeHeaderWav(&decoder->base, input, length);
    if (ADXB_CheckAiff(input) != 0) return ADXB_DecodeHeaderAiff(&decoder->base, input, length);
    if (ADXB_CheckAu(input) != 0) return ADXB_DecodeHeaderAu(&decoder->base, input, length);
    return -1;
}

void ADXB_SetDefPrm(AdxBasicDecoderExt* decoder)
{
    AdxBasicDecoder* base = &decoder->base;
    base->header_decoded = 1;
    base->sample_rate = 48000;
    base->channel_count = 2;
    base->bits_per_sample = 16;
    base->total_samples = 0x7FFFFFFF;
    base->block_length = 127;
    base->samples_per_block = 1024;
    base->format_type = base->field_9A;
    base->decode.channel_count = base->channel_count;
    base->decode.block_size = base->block_length;
    base->decode.samples_per_block = base->samples_per_block;
    base->decode.pcm_buffer = base->pcm_buffer;
    base->decode.pcm_size = base->pcm_size;
    base->decode.pcm_distance = base->pcm_distance;
    base->current_write_position = 0;
    base->coefficient = 0;
    base->loop_count = 0;
    base->loop_type = 0;
    base->loop_insert_samples = 0;
    base->loop_start_sample = 0;
    base->loop_start_offset = 0;
    base->loop_end_sample = 0;
    base->loop_end_offset = 0;
    base->total_decoded_samples = 0;
}

#define ADXB_SELECT_ENCRYPTION_KEY(decoder, version, revision, key)             \
    do {                                                                        \
        (key).type = 0;                                                         \
        if ((version) < 4) {                                                    \
            (key).state = (key).multiplier = (key).increment = 0;               \
        } else if ((revision) >= 16) {                                          \
            ADXB_MAKE_ENCRYPTION_KEY((decoder)->base.total_samples, (key));     \
        } else if ((revision) >= 8) {                                           \
            if ((decoder)->default_key[0] == 0 &&                               \
                (decoder)->default_key[1] == 0 &&                               \
                (decoder)->default_key[2] == 0) {                               \
                (decoder)->default_key[0] = adxb_def_k0;                        \
                (decoder)->default_key[1] = adxb_def_km;                        \
                (decoder)->default_key[2] = adxb_def_ka;                        \
            }                                                                   \
            (key).state = (decoder)->default_key[0];                            \
            (key).multiplier = (decoder)->default_key[1];                       \
            (key).increment = (decoder)->default_key[2];                        \
        } else {                                                                \
            (key).state = (key).multiplier = (key).increment = 0;               \
        }                                                                       \
    } while (0)

int ADXB_DecodeHeaderAdx(AdxBasicDecoderExt* decoder, signed char* input,
                         int length)
{
    AdxBasicDecoder* base = &decoder->base;
    AdxEncryptionKey key;
    short data_length;
    short delay_left[2];
    short delay_right[2];
    unsigned char version;
    unsigned char revision;

    base->header_decoded = 1;
    if (ADX_DecodeInfo((AdxHeader*)input, length, &data_length, &base->encoding,
                       &base->bits_per_sample, &base->block_length,
                       &base->channel_count, &base->sample_rate,
                       &base->total_samples, &base->samples_per_block) < 0) return 0;
    if (base->encoding > 4) {
        if (decoder->ahx_decoder == 0) {
            ADXERR_CallErrFunc2(adxb_ahx_error, adxb_ahx_detail);
            return -1;
        }
        base->bits_per_sample = 8;
        base->block_length = base->channel_count * 0xC0;
        base->samples_per_block = 0x60;
        base->format_type = 10;
        base->coefficient = 0;
        base->loop_count = 0;
        base->loop_type = 0;
        base->loop_insert_samples = 0;
        base->loop_start_sample = 0;
        base->loop_start_offset = 0;
        base->loop_end_sample = 0;
        base->loop_end_offset = 0;
        base->total_decoded_samples = 0;
        if (ADX_DecodeInfoExVer((AdxHeader*)input, length, &version, &revision) < 0) return 0;
        ADXB_SELECT_ENCRYPTION_KEY(decoder, version, revision, key);
        if (ahxsetextfunc != 0) ahxsetextfunc(decoder->ahx_decoder, &key.type);
    } else {
        if (ADX_DecodeInfoExVer((AdxHeader*)input, length, &version, &revision) < 0) return 0;
        ADXB_SELECT_ENCRYPTION_KEY(decoder, version, revision, key);
        ADXPD_SetExtPrm(base->expander, key.state, key.multiplier, key.increment);
        if (ADX_DecodeInfoExADPCM2((AdxHeader*)input, length, &base->coefficient) < 0) return 0;
        if (ADX_DecodeInfoExIdly((AdxHeader*)input, length, delay_left, delay_right) < 0) return 0;
        ADXPD_SetCoef(base->expander, base->sample_rate, base->coefficient);
        ADXPD_SetDly(base->expander, delay_left, delay_right);
        ADX_DecodeInfoExLoop(input, length, &base->loop_insert_samples,
                             &base->loop_count, &base->loop_type,
                             &base->loop_start_sample, &base->loop_start_offset,
                             &base->loop_end_sample, &base->loop_end_offset);
        ADX_DecodeInfoAinf(input, length, &decoder->ainf_length, decoder->ainf,
                           &decoder->default_out_volume, decoder->default_pan);
        base->format_type = 0;
    }
    base->decode.channel_count = base->channel_count;
    base->decode.block_size = base->block_length;
    base->decode.samples_per_block = base->samples_per_block;
    base->decode.pcm_buffer = base->pcm_buffer;
    base->decode.pcm_size = base->pcm_size;
    base->decode.pcm_distance = base->pcm_distance;
    base->current_write_position = 0;
    return data_length;
}

void ADXB_Destroy(AdxBasicDecoderExt* decoder)
{
    AdxXpnd* expander;
    if (decoder != 0) {
        expander = decoder->base.expander;
        decoder->base.expander = 0;
        ADXPD_Destroy(expander);
        memset(decoder, 0, sizeof(*decoder));
        decoder->base.used = 0;
    }
}

AdxBasicDecoderExt* ADXB_Create(int max_channels, short* pcm_buffer,
                                int pcm_size, int pcm_distance)
{
    AdxBasicDecoderExt* decoder;
    int i;
    for (i = 0; i < 16; i++) {
        if (adxb_obj[i].base.used == 0) break;
    }
    if (i == 16) return 0;
    decoder = &adxb_obj[i];
    memset(decoder, 0, sizeof(*decoder));
    decoder->base.used = 1;
    decoder->base.expander = ADXPD_Create();
    if (decoder->base.expander == 0) {
        ADXB_Destroy(decoder);
        return 0;
    }
    decoder->base.max_channels = max_channels;
    decoder->base.pcm_buffer = pcm_buffer;
    decoder->base.pcm_size = pcm_size;
    decoder->base.pcm_distance = pcm_distance;
    decoder->base.get_write_info = (AdxGetWriteInfo)adxb_DefGetWr;
    decoder->base.get_write_object = decoder;
    decoder->base.add_write_info = adxb_DefAddWr;
    decoder->base.add_write_object = decoder;
    decoder->ainf_length = 0;
    decoder->default_out_volume = 0;
    decoder->default_pan[0] = -128;
    decoder->default_pan[1] = -128;
    memset(decoder->ainf, 0, sizeof(decoder->ainf));
    return decoder;
}

void adxb_DefAddWr(void* object, int data_length, int samples)
{
    AdxBasicDecoderExt* decoder = (AdxBasicDecoderExt*)object;
    decoder->base.current_write_position += samples;
    decoder->base.total_decoded_samples += samples;
}

short* adxb_DefGetWr(void* object, int* write_position, int* room,
                     int* loop_samples)
{
    AdxBasicDecoderExt* decoder = (AdxBasicDecoderExt*)object;
    *write_position = decoder->base.current_write_position;
    *room = decoder->base.pcm_size - decoder->base.current_write_position;
    *loop_samples = decoder->base.total_samples - decoder->base.total_decoded_samples;
    return decoder->base.pcm_buffer;
}

void ADXB_Init(void)
{
    ADXPD_Init();
    skg_init_count++;
    memset(adxb_obj, 0, sizeof(adxb_obj));
}
