static int sfa_init_cnt;

void SFA_Finish(void) {
    sfa_init_cnt--;
}

void SFA_Init(void) {
    sfa_init_cnt++;
}
