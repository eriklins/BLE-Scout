/* BLE basics */

#include <simpleble_c/types.h>


typedef struct _ble_t Ble;


typedef enum _dev_state_t DevState;

enum _dev_state_t { YES, NO, UNDEF };


typedef struct _ble_service_t BleService;

struct _ble_service_t {
	simpleble_uuid_t uuid;
	byte_t data_len;
	byte_t data[27];
};

DeclSt(BleService);


typedef struct _ble_manufacturer_data_t BleManufacturerData;

struct _ble_manufacturer_data_t {
	uint16_t comp_id;
	byte_t data_len;
	byte_t data[27];
};

DeclSt(BleManufacturerData);


typedef struct _ble_peripheral_t BlePeripheral;

struct _ble_peripheral_t {
	simpleble_peripheral_t handle;
	String* mac_address;
	uint8_t address_type;
	int16_t rssi;
	int16_t tx_power;
	int16_t mtu_size;
	DevState is_connectable;
	DevState is_paired;
	String* identifier;
	uint32_t service_count;
	ArrSt(BleService)* services;
	uint32_t manufacturer_data_count;
	ArrSt(BleManufacturerData)* manufacturer_data;
	bool_t is_updated;
};

DeclSt(BlePeripheral);
