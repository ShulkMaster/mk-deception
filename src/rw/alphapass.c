extern int alphaPassPluginOffset;

typedef struct RpMaterial RpMaterial;
typedef struct RwTexture RwTexture;

typedef struct RpMaterialAlphaPass {
    RwTexture* texture;
} RpMaterialAlphaPass;

RwTexture* RpMaterialGetAlphaPassTexture(RpMaterial* material) {
    RpMaterialAlphaPass* plugin;

    plugin = (RpMaterialAlphaPass*)(((unsigned long)material + alphaPassPluginOffset + 15) & ~15);
    return plugin->texture;
}

RwTexture* RpMaterialSetAlphaPassTexture(RpMaterial* material, RwTexture* texture) {
    RpMaterialAlphaPass* plugin;

    plugin = (RpMaterialAlphaPass*)(((unsigned long)material + alphaPassPluginOffset + 15) & ~15);
    plugin->texture = texture;
    return plugin->texture;
}
