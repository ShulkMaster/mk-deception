#ifndef MKD_RUNTIME_CRITICAL_REGIONS_H
#define MKD_RUNTIME_CRITICAL_REGIONS_H

#ifdef __cplusplus
extern "C" {
#endif

void __begin_critical_region(int region);
void __end_critical_region(int region);
void __kill_critical_regions(void);

#ifdef __cplusplus
}
#endif

#endif
