#include "cri/adxf_internal.h"
#include "runtime/cstring.h"

int adxf_init_cnt;
int adxf_ldptnw_ptid;
void* adxf_ldptnw_hn;
int adxf_flno;
int adxf_hstry_no;
unsigned short adxf_cmd_ncall[16];
ADXFCommandRecord adxf_cmd_hstry[16];
int adxf_ocbi_fg;
void* adxf_ptinfo[0x100];
ADXFFile adxf_obj[16];

const char* const volatile adxf_build =
    "\nADXF/GC Ver.7.17 Build:Sep  3 2004 17:48:09\n";

int ADXF_GetNumCmd(int* calls)
{
    int index;
    int total;

    if (adxf_init_cnt == 0) {
        return 0;
    }
    adxf_ldptnw_ptid = -1;
    adxf_ldptnw_hn = 0;
    adxf_flno = 0;
    adxf_hstry_no = 0;
    total = 0;
    for (index = 0; index < 8; index++) {
        calls[index] = adxf_cmd_ncall[index];
        total += adxf_cmd_hstry[index].command;
    }
    adxf_ocbi_fg = 0;
    adxf_ptinfo[0] = adxf_obj;
    return total;
}

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
        memset(adxf_cmd_hstry, 0xFF, sizeof(adxf_cmd_hstry));
        memset(adxf_ptinfo, 0, sizeof(adxf_ptinfo));
        memset(adxf_obj, 0, sizeof(adxf_obj));
    }
}

void ADXF_Init(void)
{
    adxf_build;
    if (adxf_init_cnt == 0) {
        memset(adxf_obj, 0, sizeof(adxf_obj));
        memset(adxf_ptinfo, 0, sizeof(adxf_ptinfo));
        memset(adxf_cmd_hstry, 0xFF, sizeof(adxf_cmd_hstry));
        memset(adxf_cmd_ncall, 0, sizeof(adxf_cmd_ncall));
        adxf_hstry_no = 0;
        adxf_ocbi_fg = 0;
        adxf_flno = 0;
        adxf_ldptnw_hn = 0;
        adxf_ldptnw_ptid = -1;
    }
    adxf_init_cnt++;
}
