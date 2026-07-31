#include "mwScreenEngine/ScreenClient.h"

ScreenClient::ScreenClient() {}

ScreenClient::~ScreenClient() {}

void ScreenClient::HandleEvent(ScreenObject*, int, int) {}

void ScreenClient::HandleAction(ScreenMgr*, const ScreenAction*, int) {}

ScreenAction* ScreenClient::CreateAction(int) {
    return 0;
}

int ScreenClient::IsControllerActive(int) {
    return 1;
}

void ScreenClient::PreloadData(int) {}

int ScreenClient::IsPreloadDataDone(int) {
    return 1;
}

void ScreenClient::PrintObjectDepth(ScreenRenderInfo*, int, int) {}
