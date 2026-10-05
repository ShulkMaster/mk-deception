#ifndef RUNTIME_MTRAND2_H
#define RUNTIME_MTRAND2_H

extern int reseed_rnd_tbl;
void sgenrand(unsigned int seed);
unsigned int genlrand(void);
void reload_rnd_tbl(void);

#endif
