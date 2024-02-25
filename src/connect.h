/* Connect to BLE peripheral */

#include "connect.hxx"


Connect* connect_create(void);

void connect_destroy(Connect** con);

bool_t connect_device(String* mac, const V2Df pos);
