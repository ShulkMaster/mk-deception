#ifndef MWSCREENENGINE_SCREENRESOURCELIBRARY_H
#define MWSCREENENGINE_SCREENRESOURCELIBRARY_H
class ScreenResourceLibrary {
public:
    ScreenResourceLibrary(ScreenResourceLibrary*);
    virtual ~ScreenResourceLibrary();
    virtual const char* GetString(int) { return 0; }
    virtual const char* GetString(char*) { return 0; }
    ScreenResourceLibrary* m_parent;
};
#endif
