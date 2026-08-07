#include "rw/rwplcore.h"

extern RwBool _rpMultiTexturePluginAttach(void);
extern RwBool _rpMaterialRegisterMultiTexturePlugin(int platform, int pluginID,
                                                     int dataSize);
extern RwBool _rpGameCubeMTDataPluginAttach(void);
extern RwBool _rpGameCubeMTPipePluginAttach(void);

RwBool _rpMultiTexturePlatformPluginsAttach(void) {
    RwBool result = _rpMultiTexturePluginAttach();
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
