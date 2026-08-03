#ifndef MWSCREENENGINE_MOVIE_C_ABI_H
#define MWSCREENENGINE_MOVIE_C_ABI_H

typedef struct RwTexture RwTexture;
typedef struct ScreenPoly ScreenPoly;

int GetArtSlot__Fv(void);
RwTexture* GetScreenPolyTexture__FPv(ScreenPoly* poly);
void SetScreenPolyTexture__FPvP9RwTexture(ScreenPoly* poly,
                                          RwTexture* texture);

#endif
