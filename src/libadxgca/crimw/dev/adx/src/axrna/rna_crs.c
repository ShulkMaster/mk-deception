extern void SJCRS_Unlock(void);
extern void SJCRS_Lock(void);

void GCRNA_UnlockCs(void)
{
    SJCRS_Unlock();
}

void GCRNA_LockCs(void)
{
    SJCRS_Lock();
}
