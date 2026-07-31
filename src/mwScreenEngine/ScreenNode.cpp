#include "mwScreenEngine/ScreenNode.h"
#include "mwScreenEngine/ScreenUtil.h"

ScreenNode::ScreenNode() {
    /* Retail leaves +0x04 (m_flags) untouched; ScreenObject ctor sets it. */
    typeTag = 0x42415345; /* 'BASE' */
    next = 0;
}

ScreenNode::~ScreenNode() {}

void ScreenNode::Init() {}
void ScreenNode::Dispose() {}
void ScreenNode::Render(ScreenRenderInfo* /*info*/) {}
void ScreenNode::SetComponent(ScreenAnimControl* /*ctrl*/, float* /*values*/,
                              int /*unused*/) {}

void* ScreenNode::operator new(unsigned long size) {
    char* name;
    int tag;
    name = (char*)"SS-Node";
    tag = 0x494e4954;
    return ScreenUtil::Malloc(size, tag, name);
}

void ScreenNode::operator delete(void* p) {
    ScreenUtil::Free(p);
}
