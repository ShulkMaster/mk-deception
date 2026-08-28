#ifndef CRI_MPV_H
#define CRI_MPV_H

const unsigned char* MPV_SearchDelim(const unsigned char* data, int length,
                                     int mask);
int MPV_CheckDelim(const unsigned char* data);

#endif
