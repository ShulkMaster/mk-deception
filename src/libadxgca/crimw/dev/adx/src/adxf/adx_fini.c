#include "runtime/cstring.h"

extern void ADXF_CloseAll(void);

typedef struct ADXFCommandRecord {
    unsigned char command;
    unsigned char phase;
    unsigned short sequence;
    void* file;
    int position;
    int length;
} ADXFCommandRecord;

typedef struct ADXFFile {
    unsigned char used;
    signed char state;
    signed char callback_suppressed;
    unsigned char stop_requested;
    void* stream;
    void* callback;
    unsigned char reserved_0C[8];
    int base_position;
    unsigned char reserved_18[8];
    int transferred;
    void* cache_address;
    int cache_size;
    unsigned char reserved_2C[0x18];
} ADXFFile;

typedef char ADXFCommandRecordSizeCheck[
    sizeof(ADXFCommandRecord) == 0x10 ? 1 : -1];
typedef char ADXFFileSizeCheck[sizeof(ADXFFile) == 0x44 ? 1 : -1];

int adxf_init_cnt;
int adxf_ldptnw_ptid;
void* adxf_ldptnw_hn;
int adxf_flno;
int adxf_hstry_no;
unsigned short adxf_cmd_ncall[16];
ADXFCommandRecord adxf_cmd_hstry[16];
int adxf_ocbi_fg;
unsigned char adxf_ptinfo[0x400];
ADXFFile adxf_obj[16];

const char* const adxf_build =
    "\nADXF/GC Ver.7.17 Build:Sep  3 2004 17:48:09\n";

void ADXF_Finish(void)
{
    adxf_init_cnt--;
    if (adxf_init_cnt == 0) {
        ADXF_CloseAll();
        adxf_ldptnw_ptid = -1;
        adxf_ldptnw_hn = 0;
        adxf_flno = 0;
        adxf_ocbi_fg = 0;
        adxf_hstry_no = 0;
        memset(adxf_cmd_ncall, 0, sizeof(adxf_cmd_ncall));
        memset(adxf_cmd_hstry, -1, sizeof(adxf_cmd_hstry));
        memset(adxf_ptinfo, 0, sizeof(adxf_ptinfo));
        memset(adxf_obj, 0, sizeof(adxf_obj));
    }
}

void ADXF_Init(void)
{
    if (adxf_init_cnt == 0) {
        memset(adxf_obj, 0, sizeof(adxf_obj));
        memset(adxf_ptinfo, 0, sizeof(adxf_ptinfo));
        memset(adxf_cmd_hstry, -1, sizeof(adxf_cmd_hstry));
        memset(adxf_cmd_ncall, 0, sizeof(adxf_cmd_ncall));
        adxf_hstry_no = 0;
        adxf_ocbi_fg = 0;
        adxf_flno = 0;
        adxf_ldptnw_hn = 0;
        adxf_ldptnw_ptid = -1;
    }
    adxf_init_cnt++;
}
