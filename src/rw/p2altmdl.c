#include "rw/rxpipeline.h"

extern RxHeap* _rxHeapGlobal;

RxExecutionContext _rxExecCtxGlobal;

void _rxPacketDestroy(RxPacket* packet) {
    RwUInt32 numClusters;
    RxPipeline* pipeline;
    RwUInt32 moreClusters;
    RxCluster* cluster;

    pipeline = packet->pipeline;
    pipeline->embeddedPacketState = rxPKST_UNUSED;
    numClusters = packet->numClusters;
    cluster = packet->clusters;
    do {
        if (cluster->clusterRef != 0) {
            if (cluster->data != 0 && (cluster->flags & 2) == 0) {
                RxHeapFree(_rxHeapGlobal, cluster->data);
            }
            cluster->flagsAndStride = 0;
            cluster->data = 0;
            cluster->numAlloced = 0;
            cluster->numUsed = 0;
            cluster->clusterRef = 0;
        }
        cluster++;
        numClusters--;
        moreClusters = numClusters;
    } while (moreClusters != FALSE);
    packet->flags = 0;
}
