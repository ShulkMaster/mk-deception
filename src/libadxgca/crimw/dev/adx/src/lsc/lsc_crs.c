extern void SJCRS_Unlock(void);
extern void SJCRS_Lock(void);

void LSC_UnlockCrs(void)
{
    SJCRS_Unlock();
}

void LSC_LockCrs(void)
{
    SJCRS_Lock();
}
