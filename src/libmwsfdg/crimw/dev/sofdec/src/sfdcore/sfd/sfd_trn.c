#include "sofdec/sfd_transport.h"

typedef int (*SfdTransportRawCallback)(SfdHandle* handle, int parameter,
                                       int value, int reserved);

/* Indexed retail dispatch supplies four argument registers for every slot. */

int SFTRN_IsSetup(SfdHandle* handle, int transport_index)
{
    return handle->transports[transport_index].interface != 0;
}

int SFTRN_GetTermFlg(SfdHandle* handle, int transport_index)
{
    return handle->transports[transport_index].terminated;
}

void SFTRN_SetTermFlg(SfdHandle* handle, int transport_index, int terminated)
{
    handle->transports[transport_index].terminated = terminated;
}

int SFTRN_GetPrepFlg(SfdHandle* handle, int transport_index)
{
    return handle->transports[transport_index].prepared;
}

void SFTRN_SetPrepFlg(SfdHandle* handle, int transport_index, int prepared)
{
    handle->transports[transport_index].prepared = prepared;
}

int SFTRN_CallTrtTrif(SfdHandle* handle, int transport_index,
                      int callback_index, SfdTransportValue parameter,
                      int value)
{
    const SfdTransportInterface* interface;
    SfdTransportRawCallback callback;

    interface = handle->transports[transport_index].interface;
    if (interface == 0) {
        return 0;
    }

    callback = ((const SfdTransportRawCallback*)interface)[callback_index];
    return callback(handle, parameter, value, 0);
}

int SFTRN_CallTrSetup(SfdHandle* handle, int callback_index)
{
    int i;
    SfdTransportState* transport;
    SfdTransportRawCallback callback;
    int result;

    transport = handle->transports;
    result = 0;
    for (i = 0; i < 9; i++, transport++) {
        if (transport->interface != 0) {
            callback = ((const SfdTransportRawCallback*)transport->interface)
                [callback_index];
            result = callback(handle, 0, 0, 0);
            if (result != 0) {
                break;
            }
        }
    }
    return result;
}

static void sftrn_BuildSystem(SfdHandle* handle,
                              const SfdTransportSetup* setup)
{
    handle->buffers[0].output_transport = 1;
    handle->transports[1].parameter_10 = 0;

    if (setup->entries[2] != 0) {
        handle->transports[1].parameter_14 = 1;
        handle->buffers[1].input_transport = 1;
        handle->buffers[1].output_transport = 2;
        handle->transports[2].parameter_10 = 1;
        handle->transports[2].parameter_14 = 3;
        handle->buffers[3].input_transport = 2;
        if (setup->entries[4] != 0) {
            handle->buffers[3].output_transport = 4;
            handle->transports[4].parameter_10 = 3;
            handle->transports[4].parameter_14 = 5;
            handle->buffers[5].input_transport = 4;
            handle->buffers[5].output_transport = 6;
            handle->transports[6].parameter_10 = 5;
        } else {
            handle->buffers[3].output_transport = 6;
            handle->transports[6].parameter_10 = 3;
        }
    } else {
        SFSET_SetCond(handle, 5, 0);
        handle->conditions_secondary[5] = 0;
    }

    if (setup->entries[3] != 0) {
        handle->transports[1].parameter_18 = 2;
        handle->buffers[2].input_transport = 1;
        handle->buffers[2].output_transport = 3;
        handle->transports[3].parameter_10 = 2;
        handle->transports[3].parameter_14 = 4;
        handle->buffers[4].input_transport = 3;
        if (setup->entries[5] != 0) {
            handle->buffers[4].output_transport = 5;
            handle->transports[5].parameter_10 = 4;
            handle->transports[5].parameter_14 = 6;
            handle->buffers[6].input_transport = 5;
            handle->buffers[6].output_transport = 7;
            handle->transports[7].parameter_10 = 6;
        } else {
            handle->buffers[4].output_transport = 7;
            handle->transports[7].parameter_10 = 4;
        }
    } else {
        SFSET_SetCond(handle, 6, 0);
        handle->conditions_secondary[6] = 0;
    }

    if (setup->entries[8] != 0) {
        handle->transports[1].parameter_1C = 7;
        handle->buffers[7].input_transport = 1;
        handle->buffers[7].output_transport = 8;
        handle->transports[8].parameter_10 = 7;
    }
}

static int sftrn_BuildAll(SfdHandle* handle,
                          const SfdTransportSetup* setup)
{
    if (setup->entries[1] != 0) {
        handle->transports[0].parameter_14 = 0;
        handle->buffers[0].input_transport = 0;
        sftrn_BuildSystem(handle, setup);
    } else if (setup->entries[2] != 0) {
        handle->transports[0].parameter_14 = 1;
        handle->buffers[1].input_transport = 0;
        handle->buffers[1].output_transport = 2;
        handle->transports[2].parameter_10 = 1;
        handle->transports[2].parameter_14 = 3;
        handle->buffers[3].input_transport = 2;
        if (setup->entries[4] != 0) {
            handle->buffers[3].output_transport = 4;
            handle->transports[4].parameter_10 = 3;
            handle->transports[4].parameter_14 = 5;
            handle->buffers[5].input_transport = 4;
            handle->buffers[5].output_transport = 6;
            handle->transports[6].parameter_10 = 5;
        } else {
            handle->buffers[3].output_transport = 6;
            handle->transports[6].parameter_10 = 3;
        }
        SFSET_SetCond(handle, 6, 0);
        handle->conditions_secondary[6] = 0;
    } else if (setup->entries[3] != 0) {
        handle->transports[0].parameter_14 = 2;
        handle->buffers[2].input_transport = 0;
        handle->buffers[2].output_transport = 3;
        handle->transports[3].parameter_10 = 2;
        handle->transports[3].parameter_14 = 4;
        handle->buffers[4].input_transport = 3;
        if (setup->entries[5] != 0) {
            handle->buffers[4].output_transport = 5;
            handle->transports[5].parameter_10 = 4;
            handle->transports[5].parameter_14 = 6;
            handle->buffers[6].input_transport = 5;
            handle->buffers[6].output_transport = 7;
            handle->transports[7].parameter_10 = 6;
        } else {
            handle->buffers[4].output_transport = 7;
            handle->transports[7].parameter_10 = 4;
        }
        SFSET_SetCond(handle, 5, 0);
        handle->conditions_secondary[5] = 0;
    } else if (setup->entries[8] != 0) {
        handle->transports[0].parameter_14 = 7;
        handle->buffers[7].input_transport = 0;
        handle->buffers[7].output_transport = 8;
        handle->transports[8].parameter_10 = 7;
        SFSET_SetCond(handle, 6, 0);
        SFSET_SetCond(handle, 5, 0);
        handle->conditions_secondary[6] = 0;
        handle->conditions_secondary[5] = 0;
    } else {
        return -1;
    }

    return 0;
}

int SFTRN_InitHn(SfdHandle* handle, SfdTransportState* transports,
                 const SfdBufferCreateConfig* create,
                 const void* buffer_setup)
{
    const SfdTransportSetup* setup;
    SfdTransportState* transport;
    int i;

    (void)buffer_setup;
    setup = create->transport_setup;
    transport = transports;
    for (i = 0; i < 9; i++, transport++) {
        transport->context = 0;
        transport->terminated = 0;
        transport->prepared = 0;
        transport->interface = setup->entries[i];
        transport->parameter_10 = 8;
        transport->parameter_14 = 8;
        transport->parameter_18 = 8;
        transport->parameter_1C = 8;
        transport->state = -1;
    }

    if (sftrn_BuildAll(handle, setup) != 0) {
        return SFLIB_SetErr(handle, 0xFF000302);
    }
    return 0;
}

int SFTRN_Finish(SfdTransportRegistry* registry)
{
    int i;
    const SfdTransportInterface** entry;
    SfdTransportRawCallback callback;
    int result;

    result = 0;
    entry = registry->entries;
    for (i = 0; i < 15; i++, entry++) {
        if (*entry != 0) {
            callback = ((const SfdTransportRawCallback*)*entry)[1];
            result = callback(0, 0, 0, 0);
            if (result != 0) {
                break;
            }
        }
    }
    return result;
}

int SFTRN_Init(SfdTransportRegistry* registry,
               const SfdTransportRegistry* source)
{
    int i;
    const SfdTransportInterface* const* entry;
    SfdTransportRawCallback callback;
    int result;

    *registry = *source;
    result = 0;
    entry = source->entries;
    for (i = 0; i < 15; i++, entry++) {
        if (*entry != 0) {
            callback = ((const SfdTransportRawCallback*)*entry)[0];
            result = callback(0, 0, 0, 0);
            if (result != 0) {
                break;
            }
        }
    }
    return result;
}
