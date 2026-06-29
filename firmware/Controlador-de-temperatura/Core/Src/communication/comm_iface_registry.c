#include "comm_iface_registry.h"

#define MAX_IFACES 2

static comm_iface_t *iface_table[MAX_IFACES];

void comm_register_iface(comm_iface_t *iface)
{
    if (iface && iface->id < COMM_IFACE_MAX)
    {
        iface_table[iface->id] = iface;
    }
}

void comm_unregister_iface(comm_iface_t *iface)
{
    if (iface && iface->id < COMM_IFACE_MAX)
    {
        iface_table[iface->id] = NULL;
    }
}

comm_iface_t *comm_get_iface(comm_iface_id_t id)
{
    if (id < COMM_IFACE_MAX)
        return iface_table[id];
    return NULL;
}
