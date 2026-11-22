#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenUtil.h"

ScreenNode::ScreenNode() {
    typeTag = 0x42415345;
    next = 0;
}

ScreenNode::~ScreenNode() {}

void ScreenNode::Init() {}
void ScreenNode::Dispose() {}
void ScreenNode::Render(ScreenRenderInfo*) {}
void ScreenNode::SetComponent(ScreenAnimControl*, float*,
                              int) {}

void* ScreenNode::operator new(size_t size) {
    return ScreenUtil::Malloc(size, 0x494e4954, (char*)"SS-Node");
}

void ScreenNode::operator delete(void* p) {
    ScreenUtil::Free(p);
}
