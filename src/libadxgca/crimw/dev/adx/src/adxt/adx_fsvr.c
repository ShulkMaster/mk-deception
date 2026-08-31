extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void cvFsExecServer(void);
extern void ADXSTM_ExecServer(void);
extern void ADXF_ExecServer(void);

int adxt_fssvr_enter_cnt;

void ADXT_ExecFsSvr(void)
{
    ADXCRS_Lock();
    if (adxt_fssvr_enter_cnt != 0) {
        ADXCRS_Unlock();
        return;
    }
    adxt_fssvr_enter_cnt = 1;
    ADXCRS_Unlock();

    adxt_fssvr_enter_cnt = 3;
    cvFsExecServer();
    adxt_fssvr_enter_cnt = 4;
    ADXSTM_ExecServer();
    adxt_fssvr_enter_cnt = 5;
    ADXF_ExecServer();
    adxt_fssvr_enter_cnt = 6;
    ADXSTM_ExecServer();
    adxt_fssvr_enter_cnt = 7;
    cvFsExecServer();
    adxt_fssvr_enter_cnt = 0;
}
