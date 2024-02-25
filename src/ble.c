/* BLE basics */

#include <nappgui.h>
#include <simpleble_c/simpleble.h>
#include "util.h"
#include "logwin.h"
#include "ble.h"


struct _ble_t
{
	ArrSt(BlePeripheral)* peripheral;
};

static Ble* i_ble = NULL;


/*---------------------------------------------------------------------------*/

static int i_compare_mac(const BlePeripheral* p1, const String* p2)
{
	return str_scmp(p1->mac_address, p2);
}

static int i_compare_service(const BleService* p1, const char_t* p2)
{
	return str_cmp_c(p1->uuid.value, p2);
}

static int i_compare_comp_id(const BleManufacturerData* p1, const uint16_t* p2)
{
	if (p1->comp_id == *p2)
		return 0;
	return 1;
}

void ble_update_peripheral(void* handle, const char_t* mac_address, const uint8_t address_type,
	const int16_t rssi, const int16_t tx_power,	const DevState is_connectable, const DevState is_paired, const char_t* identifier)
{
	uint32_t pos;

	String* mac = str_c(mac_address);
	BlePeripheral* per = arrst_search(i_ble->peripheral, i_compare_mac, mac, &pos, BlePeripheral, String);
	str_destroy(&mac);

	if (per == NULL)
	{
		per = arrst_new0(i_ble->peripheral, BlePeripheral);
		per->services = arrst_create(BleService);
		per->service_count = 0;
		per->manufacturer_data = arrst_create(BleManufacturerData);
		per->manufacturer_data_count = 0;
		per->handle = handle;
		per->mac_address = str_c(mac_address);
	}
	else
	{
		str_destroy(&per->identifier);
	}

	per->identifier = str_c(identifier);
	per->address_type = address_type;
	per->rssi = rssi;
	per->tx_power = tx_power;
	per->is_connectable = is_connectable;
	per->is_paired = is_paired;
	per->is_updated = TRUE;
}

void ble_update_service_data(const char_t* mac_address, const char_t* uuid, const byte_t len, const uint8_t* data)
{
	uint32_t pos;

	String* mac = str_c(mac_address);
	BlePeripheral* per = arrst_search(i_ble->peripheral, i_compare_mac, mac, &pos, BlePeripheral, String);
	str_destroy(&mac);

	if (per == NULL)
	{
		return;
	}

	BleService* svc = arrst_search(per->services, i_compare_service, uuid, &pos, BleService, char_t);
	if (svc == NULL)
	{
		svc = arrst_new0(per->services, BleService);
		bmem_copy_n(svc->uuid.value, uuid, SIMPLEBLE_UUID_STR_LEN, char);
		per->service_count = per->service_count + 1;
	}

	svc->data_len = len;
	if (len > 0)
		bmem_copy(svc->data, data, len);
	per->is_updated = TRUE;
}

void ble_update_manufacturer_data(const char_t* mac_address, const uint16_t comp_id, const byte_t len, const uint8_t* data)
{
	uint32_t pos;

	String* mac = str_c(mac_address);
	BlePeripheral* per = arrst_search(i_ble->peripheral, i_compare_mac, mac, &pos, BlePeripheral, String);
	str_destroy(&mac);

	if (per == NULL)
		return;

	BleManufacturerData* mfd = arrst_search(per->manufacturer_data, i_compare_comp_id, &comp_id, &pos, BleManufacturerData, uint16_t);
	if (mfd == NULL)
	{
		mfd = arrst_new0(per->manufacturer_data, BleManufacturerData);
		mfd->comp_id = comp_id;
		per->manufacturer_data_count = per->manufacturer_data_count + 1;
	}

	mfd->data_len = len;
	if (len > 0)
		bmem_copy(mfd->data, data, len);
	per->is_updated = TRUE;
}

/*---------------------------------------------------------------------------*/

BlePeripheral* ble_get_peripheral(const char_t* mac_address)
{
	uint32_t pos;

	String* mac = str_c(mac_address);
	BlePeripheral* per = arrst_search(i_ble->peripheral, i_compare_mac, mac, &pos, BlePeripheral, String);
	str_destroy(&mac);

	if (per == NULL)
		return NULL;
	
	return per;
}

static void i_remove_peripheral(BlePeripheral* per)
{
	str_destroy(&per->mac_address);
	str_destroy(&per->identifier);
	arrst_destroy(&per->services, NULL, BleService);
	arrst_destroy(&per->manufacturer_data, NULL, BleManufacturerData);
}

void ble_clear_device_list(void)
{
	arrst_clear(i_ble->peripheral, i_remove_peripheral, BlePeripheral);
}

/*---------------------------------------------------------------------------*/

Ble* ble_create(void)
{
	Ble* ble = heap_new0(Ble);

	i_ble = ble;

	ble->peripheral = arrst_create(BlePeripheral);

	return ble;
}

void ble_destroy(Ble** ble)
{
	arrst_destroy(&(*ble)->peripheral, i_remove_peripheral, BlePeripheral);
	heap_delete(ble, Ble);
}
