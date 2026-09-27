#include "dolphin/types.h"

/*
 * Retail restores GQR2 through GQR7 from saved[2..7].  Those special-purpose
 * register writes have no portable C representation.
 */
/* TODO: [blocked] 7.69%; retail writes GQR2-GQR7 with mtspr, unavailable in honest C under the source rules. */
void UTY_PopGqr(u32 saved[8])
{
}

/*
 * Retail saves GQR2 through GQR7 into saved[2..7].  Those special-purpose
 * register reads have no portable C representation.
 */
/* TODO: [blocked] 7.69%; retail reads GQR2-GQR7 with mfspr and stores them to saved[2..7]; there is no honest C
 * form (no mfspr intrinsic); the assembly-sequence path needs explicit user permission. */
void UTY_PushGqr(u32 saved[8])
{
}
