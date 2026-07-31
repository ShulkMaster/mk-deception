#ifndef MWSCREENENGINE_SCREENMATRIXSTACK_H
#define MWSCREENENGINE_SCREENMATRIXSTACK_H
struct Screen3DVector { float x; float y; float z; };
class ScreenMatrixStack {
public:
    ScreenMatrixStack();
    virtual ~ScreenMatrixStack();
    virtual void Init();
    virtual void Dispose();
    virtual void SetIdentity() = 0;
    virtual void AddChild(ScreenMatrixStack*) = 0;
    virtual void Rotate(Screen3DVector*, float) = 0;
    virtual void Scale(Screen3DVector*) = 0;
    virtual void Translate(Screen3DVector*) = 0;
};
#endif
