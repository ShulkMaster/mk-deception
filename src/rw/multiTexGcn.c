#include "rw/rpmatfx.h"

extern int _rpGameCubeMTPipePluginAttach(void);

int _rpMultiTexturePlatformPluginsAttach(void) {
    int result = _rpMultiTexturePluginAttach();
    if (result == 0) {
        return 0;
    }

    result = _rpMaterialRegisterMultiTexturePlugin(6, 0x129, 0xC);
    if (result == 0) {
        return 0;
    }

    result = _rpGameCubeMTDataPluginAttach();
    if (result == 0) {
        return 0;
    }

    result = _rpGameCubeMTPipePluginAttach();
    return result;
}
