extern int alphaPassPluginOffset;

void* RpMaterialGetAlphaPassTexture(void* material) {
    void** plugin;

    plugin = (void**)(((unsigned long)material + alphaPassPluginOffset + 15) & ~15);
    return plugin[0];
}

void* RpMaterialSetAlphaPassTexture(void* material, void* texture) {
    void** plugin;

    plugin = (void**)(((unsigned long)material + alphaPassPluginOffset + 15) & ~15);
    plugin[0] = texture;
    return plugin[0];
}
