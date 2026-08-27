#include "dolphin/types.h"

/*
 * Retail restores GQR2 through GQR7 from saved[2..7].  Those special-purpose
 * register writes have no portable C representation.
 */
void UTY_PopGqr(u32 saved[8])
{
}

/*
 * Retail saves GQR2 through GQR7 into saved[2..7].  Those special-purpose
 * register reads have no portable C representation.
 */
void UTY_PushGqr(u32 saved[8])
{
}
