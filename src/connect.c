/* Connect to BLE peripheral */

#include <nappgui.h>
#include "logwin.h"
#include "ble.h"
#include "connect.h"
#include "util.h"
#include <simpleble_c/simpleble.h>


#define SHIFT_DEV 24
#define SHIFT_SVC 16
#define SHIFT_CHR 8
#define SHIFT_DSC 0


typedef struct _conn_descriptor_t ConnDescriptor;

struct _conn_descriptor_t {
    simpleble_uuid_t uuid;
    size_t data_len;
    byte_t data[512];

    Edit* edit;
};

DeclSt(ConnDescriptor);


typedef struct _conn_characteristic_t ConnCharacteristic;

struct _conn_characteristic_t {
    simpleble_uuid_t uuid;
    bool_t can_read;
    bool_t can_write_request;
    bool_t can_write_command;
    bool_t can_notify;
    bool_t can_indicate;
    size_t data_len;
    byte_t data[512];
    uint32_t descriptor_count;
    ArrSt(ConnDescriptor)* descriptor;

    Button* check_ascii;
    Edit* edit;
    Cell* cell_edit;
    GuiControl* control_edit;
    Button* button_read;
    Cell* cell_read;
    Button* button_write_cmd;
    Cell* cell_write_cmd;
    Button* button_write_req;
    Cell* cell_write_req;
    Button* button_notify;
    Cell* cell_notify;
    Button* button_indicate;
    Cell* cell_indicate;

    bool_t notification_active;
    bool_t indication_active;
};

DeclSt(ConnCharacteristic);


typedef struct _conn_service_t ConnService;

struct _conn_service_t {
    simpleble_uuid_t uuid;
    uint32_t characteristic_count;
    ArrSt(ConnCharacteristic)* characteristic;

    Button* button_terminal;
    Cell* cell_terminal;

    bool_t is_vsp_service;
};

DeclSt(ConnService);


typedef struct _conndevice_t ConnDevice;

struct _conndevice_t
{
    bool_t is_connected;

    Window* window;
    Layout* layout_gatt;
    Button* button_disconnect;
    Cell* cell_disconnect;
    Label* label_mtu_size;

    simpleble_peripheral_t handle;
    String* device_name;
    String* mac_address;
    int16_t mtu_size;

    uint32_t service_count;
    ArrSt(ConnService)* service;
};

DeclSt(ConnDevice);


struct _connect_t
{
    ArrSt(ConnDevice)* device;
    V2Df origin;
};

static Connect* i_connect = NULL;


/*---------------------------------------------------------------------------*/

static int i_compare_mac(const ConnDevice* conn_device, const String* s)
{
    return str_scmp(conn_device->mac_address, s);
}

static void i_SimpleBleOnConnected(simpleble_peripheral_t peripheral, void* userdata)
{
    char* peripheral_address = simpleble_peripheral_address(peripheral);
    char* peripheral_identifier = simpleble_peripheral_identifier(peripheral);

    String* mac = str_c(peripheral_address);
    uint32_t pos;
    ConnDevice* conn_device = arrst_search(i_connect->device, i_compare_mac, mac, &pos, ConnDevice, String);

    if (conn_device != NULL)
    {
        String* msg = str_printf("BLE connected to \"%s\"", peripheral_identifier);
        window_title(conn_device->window, tc(msg));
        str_destroy(&msg);

        button_text(conn_device->button_disconnect, "Disconnect");
        conn_device->is_connected = TRUE;

        arrst_foreach(con_svc, conn_device->service, ConnService)

            arrst_foreach(con_chr, con_svc->characteristic, ConnCharacteristic)

                if (con_chr->button_read != NULL)
                    cell_enabled(con_chr->cell_read, TRUE);
                if (con_chr->button_write_cmd != NULL)
                    cell_enabled(con_chr->cell_write_cmd, TRUE);
                if (con_chr->button_write_req != NULL)
                    cell_enabled(con_chr->cell_write_req, TRUE);
                if (con_chr->button_notify != NULL)
                    cell_enabled(con_chr->cell_notify, TRUE);
                if (con_chr->button_indicate != NULL)
                    cell_enabled(con_chr->cell_indicate, TRUE);

            arrst_end()

        arrst_end()
    }

    log_printf("> Connected to \"%s\" %s\n", peripheral_identifier, tc(mac));

    str_destroy(&mac);

    unref(userdata);
}

static void i_SimpleBleOnDisconnected(simpleble_peripheral_t peripheral, void* userdata)
{
    char* peripheral_address = simpleble_peripheral_address(peripheral);
    char* peripheral_identifier = simpleble_peripheral_identifier(peripheral);

    String* mac = str_c(peripheral_address);
    uint32_t pos;
    ConnDevice* conn_device = arrst_search(i_connect->device, i_compare_mac, mac, &pos, ConnDevice, String);

    if (conn_device != NULL)
    {
        String* msg = str_printf("BLE disconnected from %s", tc(mac));
        window_title(conn_device->window, tc(msg));
        str_destroy(&msg);

        button_text(conn_device->button_disconnect, "Connect");
        conn_device->is_connected = FALSE;

        arrst_foreach(con_svc, conn_device->service, ConnService)

            arrst_foreach(con_chr, con_svc->characteristic, ConnCharacteristic)

                con_chr->notification_active = FALSE;
                con_chr->indication_active = FALSE;

                if (con_chr->button_read != NULL)
                    cell_enabled(con_chr->cell_read, FALSE);
                if (con_chr->button_write_cmd != NULL)
                    cell_enabled(con_chr->cell_write_cmd, FALSE);
                if (con_chr->button_write_req != NULL)
                    cell_enabled(con_chr->cell_write_req, FALSE);
                if (con_chr->button_notify != NULL)
                {
                    Font* font = font_system(10, ekFNORMAL);
                    button_font(con_chr->button_notify, font);
                    cell_enabled(con_chr->cell_notify, FALSE);
                }
                if (con_chr->button_indicate != NULL)
                {
                    Font* font = font_system(10, ekFNORMAL);
                    button_font(con_chr->button_indicate, font);
                    cell_enabled(con_chr->cell_indicate, FALSE);
                }

            arrst_end()

        arrst_end()
    }

    log_printf("> Disconnected from \"%s\" %s\n", peripheral_identifier, tc(mac));

    str_destroy(&mac);

    unref(userdata);
}

static int i_compare_service_uuid(const ConnService* con_svc, const simpleble_uuid_t* uuid)
{
    return str_cmp_c(con_svc->uuid.value, uuid->value);
}

static int i_compare_characteristic_uuid(const ConnCharacteristic* con_chr, const simpleble_uuid_t* uuid)
{
    return str_cmp_c(con_chr->uuid.value, uuid->value);
}

static void i_SimpleBleOnNotification(simpleble_uuid_t service, simpleble_uuid_t characteristic,
    const uint8_t* data, size_t data_length, void* userdata)
{
    ConnDevice* con_dev = NULL;
    ConnService* con_svc = NULL;
    ConnCharacteristic* con_chr = NULL;

    uint32_t pos;
    uint32_t i;
    for (i = 0; i < arrst_size(i_connect->device, ConnDevice); ++i)
    {
        con_dev = arrst_get(i_connect->device, i, ConnDevice);
        con_svc = arrst_search(con_dev->service, i_compare_service_uuid, &service, &pos, ConnService, simpleble_uuid_t);
        if (con_svc != NULL)
        {
            con_chr = arrst_search(con_svc->characteristic, i_compare_characteristic_uuid, &characteristic, &pos, ConnCharacteristic, simpleble_uuid_t);
            if (con_chr != NULL)
            {
                break;
            }
        }
    }

    if ((con_svc != NULL) && (con_chr != NULL))
    {
        bmem_copy(con_chr->data, (byte_t*)data, (uint32_t)data_length);
        con_chr->data_len = data_length;

        char_t d[1024];
        gui_state_t btn_state = button_get_state(con_chr->check_ascii);
        if (btn_state == ekGUI_ON)
            util_data_to_ascii(con_chr->data, d, con_chr->data_len);
        else
            util_data_to_hex(con_chr->data, d, con_chr->data_len);
        edit_text(con_chr->edit, d);
    }

    unref(userdata);
}

static void i_SimpleBleOnIndication(simpleble_uuid_t service, simpleble_uuid_t characteristic,
    const uint8_t* data, size_t data_length, void* userdata)
{
    ConnDevice* con_dev = NULL;
    ConnService* con_svc = NULL;
    ConnCharacteristic* con_chr = NULL;

    uint32_t pos;
    uint32_t i;
    for (i = 0; i < arrst_size(i_connect->device, ConnDevice); ++i)
    {
        con_dev = arrst_get(i_connect->device, i, ConnDevice);
        con_svc = arrst_search(con_dev->service, i_compare_service_uuid, &service, &pos, ConnService, simpleble_uuid_t);
        if (con_svc != NULL)
        {
            con_chr = arrst_search(con_svc->characteristic, i_compare_characteristic_uuid, &characteristic, &pos, ConnCharacteristic, simpleble_uuid_t);
            if (con_chr != NULL)
            {
                break;
            }
        }
    }

    if ((con_svc != NULL) && (con_chr != NULL))
    {
        bmem_copy(con_chr->data, (byte_t*)data, (uint32_t)data_length);
        con_chr->data_len = data_length;

        char_t d[1024];
        gui_state_t btn_state = button_get_state(con_chr->check_ascii);
        if (btn_state == ekGUI_ON)
            util_data_to_ascii(con_chr->data, d, con_chr->data_len);
        else
            util_data_to_hex(con_chr->data, d, con_chr->data_len);
        edit_text(con_chr->edit, d);
    }

    unref(userdata);
}

/*---------------------------------------------------------------------------*/

static void i_OnCheckButtonHexAscii(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);
    gui_state_t btn_state = button_get_state(b);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    char_t d[1024];
    if (btn_state == ekGUI_ON)
    {
        if (con_chr->cell_write_cmd != NULL)
            cell_enabled(con_chr->cell_write_cmd, TRUE);
        if (con_chr->cell_write_req != NULL)
            cell_enabled(con_chr->cell_write_req, TRUE);
        edit_bgcolor(con_chr->edit, kCOLOR_DEFAULT);

        util_data_to_ascii(con_chr->data, d, con_chr->data_len);
    }
    else
        util_data_to_hex(con_chr->data, d, con_chr->data_len);

    edit_text(con_chr->edit, d);

    unref(e);
}

static void i_OnChangeEditCharacteristic(Connect* connect, Event* e)
{
    Edit* edit = event_sender(e, Edit);
    GuiControl* control = guicontrol(edit);

    uint32_t tag = guicontrol_get_tag(control);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    const char_t* buf = edit_get_text(edit);
    uint32_t buf_len = str_len_c(buf);
    if (buf_len > 1024)
        buf_len = 1024;

    if (button_get_state(con_chr->check_ascii) == ekGUI_OFF)
    {
        if ((util_buffer_is_hex(buf, buf_len) == TRUE) || (buf_len == 0))
        {
            if (con_chr->cell_write_cmd != NULL)
                cell_enabled(con_chr->cell_write_cmd, TRUE);
            if (con_chr->cell_write_req != NULL)
                cell_enabled(con_chr->cell_write_req, TRUE);
            edit_bgcolor(edit, kCOLOR_DEFAULT);

            util_hex_to_data(buf, con_chr->data, buf_len);
            con_chr->data_len = buf_len / 2;
        }
        else
        {
            if (con_chr->cell_write_cmd != NULL)
                cell_enabled(con_chr->cell_write_cmd, FALSE);
            if (con_chr->cell_write_req != NULL)
                cell_enabled(con_chr->cell_write_req, FALSE);
            edit_bgcolor(edit, color_html("#F1948A"));
        }
    }
    else
    {
        if (buf_len > 512)
            buf_len = 512;
        bmem_copy(con_chr->data, (byte_t*)buf, buf_len);
        con_chr->data_len = buf_len;
    }

    unref(e);
}

static void i_OnClickCharacteristicRead(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    byte_t* data[1];
    size_t data_len;

    if (simpleble_peripheral_read(con_dev->handle, con_svc->uuid, con_chr->uuid, data, &data_len) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Read failed: DEV \"%s\" %s -> SVC %s -> CHR %s\n",
            tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);
    
        return;
    }

    if (data_len > 512)
        data_len = 512;
    bmem_copy(con_chr->data, data[0], (uint32_t)data_len);
    con_chr->data_len = data_len;

    char_t d[1024];
    if (button_get_state(con_chr->check_ascii) == ekGUI_ON)
        util_data_to_ascii(con_chr->data, d, data_len);
    else
        util_data_to_hex(con_chr->data, d, data_len);

    edit_text(con_chr->edit, d);

    util_data_to_hex(con_chr->data, d, data_len);
    log_printf("> Read %s from DEV \"%s\" %s -> SVC %s -> CHR %s\n", d,
        tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

    unref(e);
}

static void i_OnClickCharacteristicWriteCommand(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    if (simpleble_peripheral_write_command(con_dev->handle, con_svc->uuid, con_chr->uuid, con_chr->data, con_chr->data_len) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Write-command failed: DEV \"%s\" %s -> SVC %s -> CHR %s\n",
            tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

        return;
    }

    char_t d[1024];
    util_data_to_hex(con_chr->data, d, con_chr->data_len);
    log_printf("> Write-command %s to DEV \"%s\" %s -> SVC %s -> CHR %s\n", d,
        tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

    unref(e);
}

static void i_OnClickCharacteristicWriteRequest(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    if (simpleble_peripheral_write_request(con_dev->handle, con_svc->uuid, con_chr->uuid, con_chr->data, con_chr->data_len) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Write-request failed: DEV \"%s\" %s -> SVC %s -> CHR %s\n",
            tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

        return;
    }

    char_t d[1024];
    util_data_to_hex(con_chr->data, d, con_chr->data_len);
    log_printf("> Write-request %s to DEV \"%s\" %s -> SVC %s -> CHR %s\n", d,
        tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

    unref(e);
}

static void i_OnClickCharacteristicSubscribeNotification(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    void* userdata = NULL;

    if (simpleble_peripheral_notify(con_dev->handle, con_svc->uuid, con_chr->uuid, i_SimpleBleOnNotification, userdata) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Notifications failed: DEV \"%s\" %s -> SVC %s -> CHR %s\n",
            tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

        return;
    }

    Font* font = font_system(10, ekFBOLD);
    button_font(con_chr->button_notify, font);

    log_printf("> Notifications active for DEV \"%s\" %s -> SVC %s -> CHR %s\n",
        tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

    unref(e);
}

static void i_OnClickCharacteristicSubscribeIndication(Connect* connect, Event* e)
{
    Button* b = event_sender(e, Button);

    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SHIFT_DEV) & 0xff;
    uint32_t svc_idx = (tag >> SHIFT_SVC) & 0xff;
    uint32_t chr_idx = (tag >> SHIFT_CHR) & 0xff;

    ConnDevice* con_dev = arrst_get(connect->device, dev_idx, ConnDevice);
    ConnService* con_svc = arrst_get(con_dev->service, svc_idx, ConnService);
    ConnCharacteristic* con_chr = arrst_get(con_svc->characteristic, chr_idx, ConnCharacteristic);

    void* userdata = NULL;

    if (simpleble_peripheral_indicate(con_dev->handle, con_svc->uuid, con_chr->uuid, i_SimpleBleOnNotification, userdata) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Indications failed: DEV \"%s\" %s -> SVC %s -> CHR %s\n",
            tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

        return;
    }

    Font* font = font_system(10, ekFBOLD);
    button_font(con_chr->button_indicate, font);

    log_printf("> Indications active for DEV \"%s\" %s -> SVC %s -> CHR %s\n",
        tc(con_dev->device_name), tc(con_dev->mac_address), con_svc->uuid.value, con_chr->uuid.value);

    unref(e);
}

/*---------------------------------------------------------------------------*/

static bool_t i_connect_peripheral(ConnDevice* conn_device)
{
    uint32_t dev_idx;
    arrst_search(i_connect->device, i_compare_mac, conn_device->mac_address, &dev_idx, ConnDevice, String);

    if (simpleble_peripheral_connect(conn_device->handle) != SIMPLEBLE_SUCCESS)
    {
        log_printf("> \a1Could not connect to \"%s\" %s.\n", tc(conn_device->device_name), tc(conn_device->mac_address));
        return FALSE;
    }

    button_text(conn_device->button_disconnect, "Disconnect");
    cell_enabled(conn_device->cell_disconnect, TRUE);

    conn_device->mtu_size = simpleble_peripheral_mtu(conn_device->handle) + 3;
    String* msg = str_printf("MTU Size: %d", conn_device->mtu_size);
    label_text(conn_device->label_mtu_size, tc(msg));
    str_destroy(&msg);

    log_printf("> Reading GATT table from \"%s\" %s\n", tc(conn_device->device_name), tc(conn_device->mac_address));

    conn_device->service_count = (uint32_t)simpleble_peripheral_services_count(conn_device->handle);
    uint32_t svc_idx;
    for (svc_idx = 0; svc_idx < conn_device->service_count; ++svc_idx)
    {
        ConnService* conn_svc = arrst_new0(conn_device->service, ConnService);
        conn_svc->characteristic = arrst_create(ConnCharacteristic);

        simpleble_service_t svc;
        simpleble_peripheral_services_get(conn_device->handle, svc_idx, &svc);

        bmem_copy_n(conn_svc->uuid.value, svc.uuid.value, SIMPLEBLE_UUID_STR_LEN, char);

        uint32_t nrows = layout_nrows(conn_device->layout_gatt);
        cassert(nrows >= 1);
        layout_insert_row(conn_device->layout_gatt, nrows);


        Layout* layout_service = layout_create(1, 2);
        Layout* layout_uuid_terminal = layout_create(2, 1);
        layout_hexpand(layout_uuid_terminal, 0);
        layout_skcolor(layout_service, color_html("#ABB2B9"));
        layout_bgcolor(layout_service, color_html("#EBF5FB"));

        Font* font_service = font_system(14, ekFBOLD);
        Label* label_service = label_create();
        label_font(label_service, font_service);
        String* svc_name = util_service_uuid_to_name(conn_svc->uuid.value);
        if (str_empty(svc_name))
        {
            str_destroy(&svc_name);
            svc_name = util_vsp_service_uuid_to_name(conn_svc->uuid.value);
            if (str_empty(svc_name))
            {
                str_destroy(&svc_name);
                svc_name = str_c(conn_svc->uuid.value);
                conn_svc->is_vsp_service = FALSE;
            }
            else
            {
                conn_svc->is_vsp_service = TRUE;
            }
        }
        label_text(label_service, tc(svc_name));
        log_printf(">   \a2SVC_____ %s\n", tc(svc_name));
        str_destroy(&svc_name);
        layout_label(layout_uuid_terminal, label_service, 0, 0);
        cell_padding4(layout_cell(layout_uuid_terminal, 0, 0), 5, 5, 0, 10);

        if (conn_svc->is_vsp_service == TRUE)
        {
            Font* font_button = font_system(10, ekFNORMAL);
            conn_svc->button_terminal = button_push();
            button_font(conn_svc->button_terminal, font_button);
            button_text(conn_svc->button_terminal, "VSP Terminal");
            layout_button(layout_uuid_terminal, conn_svc->button_terminal, 1, 0);
            conn_svc->cell_terminal = layout_cell(layout_uuid_terminal, 1, 0);
            cell_enabled(conn_svc->cell_terminal, FALSE);
        }
        cell_padding4(layout_cell(layout_uuid_terminal, 1, 0), 5, 10, 0, 5);
        layout_layout(layout_service, layout_uuid_terminal, 0, 0);

        conn_svc->characteristic_count = (uint32_t)svc.characteristic_count;

        if (conn_svc->characteristic_count > 0)
        {
            Layout* layout_chr_list = layout_create(2, 1);
            layout_layout(layout_service, layout_chr_list, 0, 1);
            cell_padding4(layout_cell(layout_service, 0, 1), 5, 0, 0, 20);

            uint32_t chr_rows = 0;
            uint32_t chr_idx;
            for (chr_idx = 0; chr_idx < conn_svc->characteristic_count; ++chr_idx)
            {
                ConnCharacteristic* conn_chr = arrst_new0(conn_svc->characteristic, ConnCharacteristic);
                conn_chr->descriptor = arrst_create(ConnDescriptor);
                bmem_copy_n(conn_chr->uuid.value, svc.characteristics[chr_idx].uuid.value, SIMPLEBLE_UUID_STR_LEN, char);
                conn_chr->can_read = svc.characteristics[chr_idx].can_read ? TRUE : FALSE;
                conn_chr->can_write_command = svc.characteristics[chr_idx].can_write_command ? TRUE : FALSE;
                conn_chr->can_write_request = svc.characteristics[chr_idx].can_write_request ? TRUE : FALSE;
                conn_chr->can_notify = svc.characteristics[chr_idx].can_notify ? TRUE : FALSE;
                conn_chr->can_indicate = svc.characteristics[chr_idx].can_indicate ? TRUE : FALSE;
                conn_chr->data_len = 0;
                conn_chr->notification_active = FALSE;
                conn_chr->indication_active = FALSE;

                cell_padding4(layout_cell(layout_chr_list, 0, chr_rows), 5, 10, 0, 0);
                cell_padding4(layout_cell(layout_chr_list, 1, chr_rows), 5, 0, 0, 0);

                Font* font_buttons = font_system(10, ekFNORMAL);

                Label* label_characteristic = label_create();
                String* chr_name = util_characteristic_uuid_to_name(conn_chr->uuid.value);
                if (str_empty(chr_name))
                {
                    str_destroy(&chr_name);
                    chr_name = util_vsp_characteristic_uuid_to_name(conn_chr->uuid.value);
                    if (str_empty(chr_name))
                    {
                        str_destroy(&chr_name);
                        chr_name = str_c(conn_chr->uuid.value);
                    }
                }
                label_text(label_characteristic, tc(chr_name));
                log_printf(">     \a3CHR___ %s\n", tc(chr_name));
                str_destroy(&chr_name);
                layout_label(layout_chr_list, label_characteristic, 0, chr_rows);

                conn_chr->check_ascii = button_check();
                button_text(conn_chr->check_ascii, "ASCII");
                button_font(conn_chr->check_ascii, font_buttons);
                button_tag(conn_chr->check_ascii, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                button_OnClick(conn_chr->check_ascii, listener(i_connect, i_OnCheckButtonHexAscii, Connect));
                layout_button(layout_chr_list, conn_chr->check_ascii, 1, chr_rows);

                chr_rows++;
                layout_insert_row(layout_chr_list, chr_rows);
                cell_padding4(layout_cell(layout_chr_list, 0, chr_rows), 0, 10, 0, 0);
                cell_padding4(layout_cell(layout_chr_list, 1, chr_rows), 0, 0, 0, 0);

                conn_chr->edit = edit_create();
                edit_editable(conn_chr->edit, FALSE);
                conn_chr->control_edit = guicontrol(conn_chr->edit);
                guicontrol_tag(conn_chr->control_edit, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                edit_OnChange(conn_chr->edit, listener(i_connect, i_OnChangeEditCharacteristic, Connect));
                layout_edit(layout_chr_list, conn_chr->edit, 0, chr_rows);
                layout_hsize(layout_chr_list, 0, 220.0);

                uint8_t* data[1];
                if (simpleble_peripheral_read(conn_device->handle, conn_svc->uuid, conn_chr->uuid, data, &conn_chr->data_len) == SIMPLEBLE_SUCCESS)
                {
                    bmem_copy(conn_chr->data, data[0], (uint32_t)conn_chr->data_len);
                    char_t d[1024];
                    util_data_to_hex(conn_chr->data, d, conn_chr->data_len);
                    edit_text(conn_chr->edit, d);
                }

                conn_chr->cell_write_cmd = NULL;
                conn_chr->cell_write_req = NULL;

                Layout* layout_buttons = layout_create(5, 1);

                uint32_t col = 0;
                if (conn_chr->can_read)
                {
                    conn_chr->button_read = button_push();
                    button_text(conn_chr->button_read, "RD");
                    button_font(conn_chr->button_read, font_buttons);
                    button_tag(conn_chr->button_read, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                    button_OnClick(conn_chr->button_read, listener(i_connect, i_OnClickCharacteristicRead, Connect));
                    layout_button(layout_buttons, conn_chr->button_read, col, 0);
                    layout_hsize(layout_buttons, col, 20.0);
                    conn_chr->cell_read = layout_cell(layout_buttons, col, 0);
                    col++;
                }
                if (conn_chr->can_write_command)
                {
                    edit_editable(conn_chr->edit, TRUE);
                    conn_chr->button_write_cmd = button_push();
                    button_text(conn_chr->button_write_cmd, "WRc");
                    button_font(conn_chr->button_write_cmd, font_buttons);
                    button_tag(conn_chr->button_write_cmd, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                    button_OnClick(conn_chr->button_write_cmd, listener(i_connect, i_OnClickCharacteristicWriteCommand, Connect));
                    layout_button(layout_buttons, conn_chr->button_write_cmd, col, 0);
                    layout_hsize(layout_buttons, col, 20.0);
                    conn_chr->cell_write_cmd = layout_cell(layout_buttons, col, 0);
                    col++;
                }
                if (conn_chr->can_write_request)
                {
                    edit_editable(conn_chr->edit, TRUE);
                    conn_chr->button_write_req = button_push();
                    button_text(conn_chr->button_write_req, "WRr");
                    button_font(conn_chr->button_write_req, font_buttons);
                    button_tag(conn_chr->button_write_req, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                    button_OnClick(conn_chr->button_write_req, listener(i_connect, i_OnClickCharacteristicWriteRequest, Connect));
                    layout_button(layout_buttons, conn_chr->button_write_req, col, 0);
                    layout_hsize(layout_buttons, col, 20.0);
                    conn_chr->cell_write_req = layout_cell(layout_buttons, col, 0);
                    col++;
                }
                if (conn_chr->can_notify)
                {
                    conn_chr->button_notify = button_push();
                    button_text(conn_chr->button_notify, "NOT");
                    button_font(conn_chr->button_notify, font_buttons);
                    button_tag(conn_chr->button_notify, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                    button_OnClick(conn_chr->button_notify, listener(i_connect, i_OnClickCharacteristicSubscribeNotification, Connect));
                    layout_button(layout_buttons, conn_chr->button_notify, col, 0);
                    layout_hsize(layout_buttons, col, 20.0);
                    conn_chr->cell_notify = layout_cell(layout_buttons, col, 0);
                    col++;
                }
                if (conn_chr->can_indicate)
                {
                    conn_chr->button_indicate = button_push();
                    button_text(conn_chr->button_indicate, "IND");
                    button_font(conn_chr->button_indicate, font_buttons);
                    button_tag(conn_chr->button_indicate, (dev_idx << SHIFT_DEV) | (svc_idx << SHIFT_SVC) | (chr_idx << SHIFT_CHR));
                    button_OnClick(conn_chr->button_indicate, listener(i_connect, i_OnClickCharacteristicSubscribeIndication, Connect));
                    layout_button(layout_buttons, conn_chr->button_indicate, col, 0);
                    conn_chr->cell_indicate = layout_cell(layout_buttons, col, 0);
                    layout_hsize(layout_buttons, col, 20.0);
                    col++;
                }
                layout_layout(layout_chr_list, layout_buttons, 1, chr_rows);

                chr_rows++;
                layout_insert_row(layout_chr_list, chr_rows);


                conn_chr->descriptor_count = (uint32_t)svc.characteristics[chr_idx].descriptor_count;

                if (conn_chr->descriptor_count > 0)
                {
                    Layout* layout_dsc_list = layout_create(2, 1);
                    layout_hexpand(layout_dsc_list, 1);
                    layout_layout(layout_chr_list, layout_dsc_list, 0, chr_rows);

                    uint32_t dsc_rows = 0;
                    uint32_t dsc_idx;
                    for (dsc_idx = 0; dsc_idx < conn_chr->descriptor_count; ++dsc_idx)
                    {
                        ConnDescriptor* conn_dsc = arrst_new0(conn_chr->descriptor, ConnDescriptor);
                        bmem_copy_n(conn_dsc->uuid.value, svc.characteristics[chr_idx].descriptors[dsc_idx].uuid.value, SIMPLEBLE_UUID_STR_LEN, char);
                        conn_dsc->data_len = 0;

                        cell_padding4(layout_cell(layout_dsc_list, 0, dsc_rows), 5, 10, 0, 20);

                        Label* label_descriptor = label_create();
                        String* dsc_name = util_descriptor_uuid_to_name(conn_dsc->uuid.value);
                        if (str_empty(dsc_name))
                            label_text(label_descriptor, conn_dsc->uuid.value);
                        else
                            label_text(label_descriptor, tc(dsc_name));
                        layout_label(layout_dsc_list, label_descriptor, 0, dsc_rows);
                        log_printf(">       \a4DSC_ %s\n", tc(dsc_name));
                        str_destroy(&dsc_name);

                        dsc_rows++;
                        layout_insert_row(layout_dsc_list, dsc_rows);
                        cell_padding4(layout_cell(layout_dsc_list, 0, dsc_rows), 0, 10, 0, 20);

                        conn_dsc->edit = edit_create();
                        edit_editable(conn_dsc->edit, FALSE);
                        layout_edit(layout_dsc_list, conn_dsc->edit, 0, dsc_rows);

                        if (simpleble_peripheral_read_descriptor(conn_device->handle, conn_svc->uuid, conn_chr->uuid, conn_dsc->uuid, data, &conn_dsc->data_len) == SIMPLEBLE_SUCCESS)
                        {
                            bmem_copy(conn_dsc->data, data[0], (uint32_t)conn_dsc->data_len);
                            char_t d[1024];
                            util_data_to_hex(conn_dsc->data, d, conn_dsc->data_len);
                            edit_text(conn_dsc->edit, d);
                        }

                        dsc_rows++;
                        layout_insert_row(layout_dsc_list, dsc_rows);
                    }

                }
                chr_rows++;
                layout_insert_row(layout_chr_list, chr_rows);
            }

            cell_padding4(layout_cell(layout_chr_list, 0, chr_rows), 0, 10, 10, 0);
            cell_padding4(layout_cell(layout_chr_list, 1, chr_rows), 0, 0, 10, 0);
        }

        if(conn_svc->characteristic_count == 0)
            cell_padding4(layout_cell(layout_service, 0, 1), 0, 10, 10, 10);

        layout_layout(conn_device->layout_gatt, layout_service, 0, nrows);
        cell_padding4(layout_cell(conn_device->layout_gatt, 0, nrows), 5, 5, 5, 5);

        layout_update(conn_device->layout_gatt);
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/

static void i_OnClickConnectDisconnectDevice(ConnDevice* conn_device, Event* e)
{
    if (conn_device->is_connected)
    {
        if (simpleble_peripheral_disconnect(conn_device->handle) != SIMPLEBLE_SUCCESS)
        {
            log_printf("> \a1Could not disconnect from \"%s\" %s.\n", tc(conn_device->device_name), tc(conn_device->mac_address));
        }
    }
    else
    {
        if (simpleble_peripheral_connect(conn_device->handle) != SIMPLEBLE_SUCCESS)
        {
            log_printf("> \a1Could not connect to \"%s\" %s.\n", tc(conn_device->device_name), tc(conn_device->mac_address));
        }
    }

    unref(e);
}

/*---------------------------------------------------------------------------*/

static Panel* i_panel(ConnDevice* conn_device)
{
    Panel* panel = panel_create();

    Layout* layout = layout_create(1, 3);
    layout_vexpand(layout, 2);

    Layout* layout_name_discon = layout_create(3, 1);
    layout_hexpand(layout_name_discon, 1);
    Layout* layout_mac_mtu = layout_create(3, 1);
    layout_hexpand(layout_mac_mtu, 1);

    layout_layout(layout, layout_name_discon, 0, 0);
    layout_layout(layout, layout_mac_mtu, 0, 1);


    Font* font_dev_name = font_system(16, ekFBOLD);
    Edit* edit_dev_name = edit_create();
    edit_font(edit_dev_name, font_dev_name);
    edit_text(edit_dev_name, tc(conn_device->device_name));
    edit_editable(edit_dev_name, FALSE);
    layout_edit(layout_name_discon, edit_dev_name, 0, 0);
    layout_hsize(layout_name_discon, 0, 220.0);

    conn_device->button_disconnect = button_push();
    button_text(conn_device->button_disconnect, "Connecting...");
    conn_device->cell_disconnect = layout_cell(layout_name_discon, 2, 0);
    cell_enabled(conn_device->cell_disconnect, FALSE);
    button_OnClick(conn_device->button_disconnect, listener(conn_device, i_OnClickConnectDisconnectDevice, ConnDevice));
    layout_button(layout_name_discon, conn_device->button_disconnect, 2, 0);
    cell_padding4(layout_cell(layout, 0, 0), 10, 20, 5, 10);


    Label* label_mac = label_create();
    String* msg = str_printf("MAC Address: %s", tc(conn_device->mac_address));
    label_text(label_mac, tc(msg));
    str_destroy(&msg);
    layout_label(layout_mac_mtu, label_mac, 0, 0);
    
    conn_device->label_mtu_size = label_create();
    label_text(conn_device->label_mtu_size, "MTU Size: n/a");
    layout_label(layout_mac_mtu, conn_device->label_mtu_size, 2, 0);
    cell_padding4(layout_cell(layout, 0, 1), 10, 20, 10, 10);


    Panel* panel_gatt = panel_scroll(FALSE, TRUE);
    real32_t scrw = panel_scroll_width(panel_gatt);

    layout_panel(layout, panel_gatt, 0, 2);

    Layout* layout_services = layout_create(1, 2);
    conn_device->layout_gatt = layout_create(1, 1);
    layout_layout(layout_services, conn_device->layout_gatt, 0, 0);
    layout_margin4(layout_services, 0, scrw, 0, 0);
    layout_vexpand(layout_services, 1);

    panel_size(panel_gatt, s2df(420, 500));
    panel_layout(panel, layout);
    panel_layout(panel_gatt, layout_services);


    return panel;
}

/*---------------------------------------------------------------------------*/

static void i_OnClose(ConnDevice* conn_device, Event* e)
{
    unref(conn_device);
    unref(e);
}

/*---------------------------------------------------------------------------*/

bool_t connect_device(String* mac_address, const V2Df pos)
{
    ConnDevice* conn_dev = arrst_new0(i_connect->device, ConnDevice);

    BlePeripheral* per = ble_get_peripheral(tc(mac_address));

    if (per == 0)
    {
        log_printf("> \a1Cannot connect unknown device %s.\n", tc(mac_address));

        return FALSE;
    }

    if (str_empty(per->identifier))
        conn_dev->device_name = str_c("unknown");
    else
        conn_dev->device_name = per->identifier;
    conn_dev->mac_address = per->mac_address;
    conn_dev->handle = per->handle;
    conn_dev->mtu_size = 0;
    conn_dev->service_count = 0;
    conn_dev->service = arrst_create(ConnService);


    Panel* panel = i_panel(conn_dev);

    conn_dev->window = window_create(ekWINDOW_STD | ekWINDOW_RESIZE);
    window_panel(conn_dev->window, panel);
    String* s = str_printf("BLE connecting to %s...", tc(conn_dev->mac_address));
    window_title(conn_dev->window, tc(s));
    str_destroy(&s);
    window_OnClose(conn_dev->window, listener(conn_dev, i_OnClose, ConnDevice));

    window_origin(conn_dev->window, pos);
    window_show(conn_dev->window);

    conn_dev->is_connected = FALSE;

    uint32_t i = simpleble_peripheral_set_callback_on_connected(conn_dev->handle, i_SimpleBleOnConnected, NULL);
    i += simpleble_peripheral_set_callback_on_disconnected(conn_dev->handle, i_SimpleBleOnDisconnected, NULL);
    if (i > 0)
        log_printf("> \a1Cannot register BLE connect/disconnect callback functions.\n");

    return i_connect_peripheral(conn_dev);
}

/*---------------------------------------------------------------------------*/

Connect* connect_create(void)
{
    Connect* connect = heap_new0(Connect);
    i_connect = connect;

    connect->device = arrst_create(ConnDevice);

    return connect;
}

static void i_remove_characteristic(ConnCharacteristic* conn_chr)
{
    arrst_destroy(&conn_chr->descriptor, NULL, ConnDescriptor);
}

static void i_remove_service(ConnService* conn_svc)
{
    arrst_destroy(&conn_svc->characteristic, i_remove_characteristic, ConnCharacteristic);
}

static void i_remove_device(ConnDevice* conn_device)
{
    str_destroy(&conn_device->device_name);
    str_destroy(&conn_device->mac_address);
    arrst_destroy(&conn_device->service, i_remove_service, ConnService);
    window_destroy(&conn_device->window);
}

void connect_destroy(Connect** connect)
{
    arrst_destroy(&(*connect)->device, i_remove_device, ConnDevice);
    heap_delete(connect, Connect);
}
