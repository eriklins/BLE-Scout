/* Utilities */

#include <nappgui.h>
#include "util.h"
#include "ble.h"
#include "bt_assigned_numbers.h"
#include "vsp_services.h"
#include "logwin.h"

struct _util_t
{
	Window* win_modal;
};

static Util* i_util = NULL;

/*---------------------------------------------------------------------------*/

String* util_vsp_service_uuid_to_name(const char_t* uuid)
{
	uint32_t i;

	for (i = 0; i < vsp_service_uuids_len; ++i)
	{
		if (str_cmp_c(uuid, vsp_service_uuids[i].uuid) == 0)
			return str_c(vsp_service_uuids[i].name);
	}
	return str_c("");
}

String* util_vsp_characteristic_uuid_to_name(const char_t* uuid)
{
	uint32_t i;

	for (i = 0; i < vsp_service_uuids_len; ++i)
	{
		if (str_cmp_c(uuid, vsp_service_uuids[i].Rx.uuid) == 0)
			return str_c(vsp_service_uuids[i].Rx.name);
		if (str_cmp_c(uuid, vsp_service_uuids[i].Tx.uuid) == 0)
			return str_c(vsp_service_uuids[i].Tx.name);
		if (str_cmp_c(uuid, vsp_service_uuids[i].ModemIn.uuid) == 0)
			return str_c(vsp_service_uuids[i].ModemIn.name);
		if (str_cmp_c(uuid, vsp_service_uuids[i].ModemOut.uuid) == 0)
			return str_c(vsp_service_uuids[i].ModemOut.name);
	}
	return str_c("");
}

String* util_service_uuid_to_name(const char_t* uuid)
{
	int i;

	for (i = 0; i < service_uuids_len; ++i)
	{
		if (str_cmp_c(uuid, service_uuids[i].uuid) == 0)
			return str_c(service_uuids[i].name);
	}
	return str_c("");
}

String* util_characteristic_uuid_to_name(const char_t* uuid)
{
	int i;

	for (i = 0; i < characteristic_uuids_len; ++i)
	{
		if (str_cmp_c(uuid, characteristic_uuids[i].uuid) == 0)
			return str_c(characteristic_uuids[i].name);
	}
	return str_c("");
}

String* util_descriptor_uuid_to_name(const char_t* uuid)
{
	int i;

	for (i = 0; i < descriptor_uuids_len; ++i)
	{
		if (str_cmp_c(uuid, descriptor_uuids[i].uuid) == 0)
			return str_c(descriptor_uuids[i].name);
	}
	return str_c("");
}

String* util_company_id_to_name(const uint16_t id)
{
	int i;

	for (i = 0; i < company_identifier_len; ++i)
	{
		if (id == company_identifier[i].code)
			return str_c(company_identifier[i].name);
	}

	return str_c("");
}

String* util_gap_appearance_to_name(const uint16_t catval)
{
	return str_c("");

	unref(&catval);
}

/*---------------------------------------------------------------------------*/

void util_data_to_hex(const byte_t* in, char* out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		if ((in[i] >> 4) <= 9)
			out[2 * i] = '0' + (in[i] >> 4);
		else
			out[2 * i] = 'A' + ((in[i] >> 4) - 10);

		if ((in[i] & 0x0f) <= 9)
			out[2 * i + 1] = '0' + (in[i] & 0x0f);
		else
			out[2 * i + 1] = 'A' + ((in[i] & 0x0f) - 10);
	}

	out[2*i] = '\0';
}

void util_hex_to_data(const char* in, byte_t* out, const size_t len)
{
	size_t i;

	for (i = 0; i < (len/2); i++)
	{
		out[i] = 0;

		if ((in[2*i] >= '0') && (in[2*i] <= '9'))
			out[i] = (in[2*i] - '0') << 4;
		else if ((in[2 * i] >= 'A') && (in[2 * i] <= 'F'))
			out[i] = (in[2 * i] - 'A' + 10) << 4;
		else if ((in[2 * i] >= 'a') && (in[2 * i] <= 'f'))
			out[i] = (in[2 * i] - 'a' + 10) << 4;

		if ((in[2*i+1] >= '0') && (in[2*i+1] <= '9'))
			out[i] |= (in[2*i+1] - '0');
		else if ((in[2 * i + 1] >= 'A') && (in[2 * i + 1] <= 'F'))
			out[i] |= (in[2 * i + 1] - 'A' + 10);
		else if ((in[2 * i + 1] >= 'a') && (in[2 * i + 1] <= 'f'))
			out[i] |= (in[2 * i + 1] - 'a' + 10);
	}
}

void util_data_to_ascii(const byte_t* in, char* out, const size_t len)
{
	size_t i;
	
	for (i = 0; i < len; ++i)
	{
		if ((in[i] >= 32) && (in[i] <= 127))
		{
			out[i] = in[i];
		}
		else
			out[i] = '#';
	}
	out[i] = '\0';
}

/*---------------------------------------------------------------------------*/

bool_t util_buffer_is_hex(const char* in, const size_t len)
{
	size_t i;

	if ((len % 2) != 0)
		return FALSE;

	for (i = 0; i < len; ++i)
	{
		if ((in[i] < '0') || (in[i] > 'f') || ((in[i] > '9') && (in[i] < 'A')) || ((in[i] > 'F') && (in[i] < 'a')))
			return FALSE;
	}
	return TRUE;
}

bool_t util_buffer_is_mac_address(const char* in, const size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i)
	{
		if ((in[i] < '0') || (in[i] > 'f') || ((in[i] > ':') && (in[i] < 'A')) || ((in[i] > 'F') && (in[i] < 'a')))
			return FALSE;
	}
	return TRUE;
}

/*---------------------------------------------------------------------------*/

static void i_OnCloseModal(Window* window, Event* e)
{
	Button* button = event_sender(e, Button);
	window_stop_modal(window, button_get_tag(button));
}

static void i_OnClickModal(Util* util, Event* e)
{
	window_stop_modal(util->win_modal, 0);

	unref(e);
}

void util_message_window(Window* window, const char* message)
{
	i_util->win_modal = window_create(ekWINDOW_TITLE | ekWINDOW_CLOSE);
	window_title(window, "Alert");

	Panel* panel = panel_create();
	Layout* layout = layout_create(1, 2);

	Label* label = label_create();
	label_text(label, message);
	layout_label(layout, label, 0, 0);

	Button* button = button_push();
	button_text(button, "OK");
	layout_button(layout, button, 0, 1);

	panel_layout(panel, layout);

	window_panel(i_util->win_modal, panel);

	V2Df pos = window_get_origin(window);
	window_origin(i_util->win_modal, v2df(pos.x + 20, pos.y + 20));

	window_modal(i_util->win_modal, window);
}

/*---------------------------------------------------------------------------*/

Util* util_create(void)
{
	Util* util = heap_new0(Util);

	i_util = util;

	return util;
}

void util_destroy(Util** util)
{
	heap_delete(util, Util);
}
