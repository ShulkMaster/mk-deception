#ifndef MWSCREENENGINE_SCREENRESOURCELIBRARY_H
#define MWSCREENENGINE_SCREENRESOURCELIBRARY_H
class ScreenResourceLibrary {
public:
    ScreenResourceLibrary(ScreenResourceLibrary*);
    virtual ~ScreenResourceLibrary();
    virtual const char* GetString(int);
    virtual const char* GetString(char*);
    ScreenResourceLibrary* m_parent;
};
#endif
