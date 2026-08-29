#include "rw/rxpipeline.h"

RxExecutionContext _rxExecCtxGlobal;

/* Soft ceiling: _rxPacketDestroy ~94.17% -- null-compare instruction selection
 * and countdown-result register coloring; memory operations and CFG match. */
void _rxPacketDestroy(RxPacket* packet) {
    RxPipeline* pipeline;
    unsigned int numClusters;
    RxCluster* cluster;

    pipeline = packet->pipeline;
    pipeline->embeddedPacketState = 1;
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
    } while (--numClusters != 0);
    packet->flags = 0;
}
