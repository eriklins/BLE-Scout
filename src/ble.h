/* BLE basics */

#include "ble.hxx"


Ble* ble_create(void);

void ble_destroy(Ble** ble);

void ble_update_peripheral(void* handle, const char_t* mac_address, const uint8_t address_type,
	const int16_t rssi, const int16_t tx_power, const DevState is_connectable, const DevState is_paired, const char_t* identifier);

void ble_update_service_data(const char_t* mac_address, const char_t* uuid, const byte_t len, const uint8_t* data);

void ble_update_manufacturer_data(const char_t* mac_address, const uint16_t comp_id, const byte_t len, const uint8_t* data);

BlePeripheral* ble_get_peripheral(const char_t* mac_address);

void ble_clear_device_list(void);
