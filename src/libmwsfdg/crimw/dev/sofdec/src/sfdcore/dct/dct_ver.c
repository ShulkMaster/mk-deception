/*
  .bbs header missing 4 bytes
  .rodata missing 4 bytes
*/

const char DCT_version_str[] =
"\nCRI DCT/GC Ver.1.932 Build:Sep  3 2004 11:38:24\n"
"\0Append: MW2407 GC20Apr2004Patch1\n";

const char* cri_verstr_ptr;

const char* DCT_GetVerStr(void) {
    cri_verstr_ptr = DCT_version_str;
    return cri_verstr_ptr;
}
