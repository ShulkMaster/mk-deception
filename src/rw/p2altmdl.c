#include "rw/rxpipeline.h"

RxExecutionContext _rxExecCtxGlobal;

void _rxPacketDestroy(RxPacket* packet) {
    RxPipeline* pipeline;
    unsigned int numClusters;
    RxCluster* cluster;

    pipeline = packet->pipeline;
    pipeline->embeddedPacketState = 1;
    numClusters = packet->numClusters;
    cluster = packet->clusters;
    do {
        if (cluster->clusterRef != (RxPipelineCluster*)0) {
            if (cluster->data != 0 && (cluster->flags & 2) == 0) {
                RxHeapFree(_rxHeapGlobal, cluster->data);
            }
            cluster->flagsAndStride = 0;
            cluster->data = 0;
            cluster->numAlloced = 0;
            cluster->numUsed = 0;
            cluster->clusterRef = 0;
        }
    } while (cluster++, --numClusters);
    packet->flags = 0;
}
