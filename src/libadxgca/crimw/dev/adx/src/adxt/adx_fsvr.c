extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void cvFsExecServer(void);
extern void ADXSTM_ExecServer(void);
extern void ADXF_ExecServer(void);

int adxt_fssvr_enter_cnt;

/* Run the two-pass CRI file/stream service sequence. The phase values are
 * observed by callbacks and deliberately leave gaps for the lock/re-entry
 * states represented by the retail scheduler. */
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
