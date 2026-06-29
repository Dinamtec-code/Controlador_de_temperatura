#ifndef COMM_IFACE_REGISTRY_H
#define COMM_IFACE_REGISTRY_H

#include "comm_driver_api.h"

void comm_register_iface(comm_iface_t *iface);
void comm_unregister_iface(comm_iface_t *iface);
comm_iface_t *comm_get_iface(comm_iface_id_t id);

#endif /* COMM_IFACE_REGISTRY_H */