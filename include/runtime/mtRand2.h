#ifndef RUNTIME_MTRAND2_H
#define RUNTIME_MTRAND2_H

/* mtRand2.o */
void sgenrand(unsigned int seed);
unsigned int genlrand(void);
void reload_rnd_tbl(void);

#endif
