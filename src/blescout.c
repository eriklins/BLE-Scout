/* BLE scanning */

#include <nappgui.h>
#include <simpleble_c/simpleble.h>
#include <simpleble_c/utils.h>
#include "util.h"
#include "logwin.h"
#include "help.h"
#include "ble.h"
#include "connect.h"

#define BLESCOUT_VERSION "0.3"

#define UDPATE_RATE_sec    0.05
#define BT_CHECK_RATE_sec  1.0

#define SH_DEVICE    24
#define SH_SVC_DATA  16
#define SH_MNF_DATA   8



typedef struct _devicepanel_t DevicePanel;

struct _devicepanel_t
{
    uint32_t row_index;
    String* mac_address;
    String* dev_name;

    Layout* sublayout;
    Edit* edit_dev_name;
    Button* button_connect;
    Cell* cell_connect;
    Label* label_connectable;
    Label* label_tx_power;
    Label* label_rssi;
    Label* label_mac_address;

    uint32_t svc_count;
    Edit* edit_svc_data[14];
    Button* check_svc_ascii[14];
    Cell* cell_svc_ascii[14];
    uint32_t mnf_count;
    Edit* edit_mnf_data[14];
    Button* check_mnf_ascii[14];
    Cell* cell_mnf_ascii[14];

    bool_t do_create;
    bool_t do_update;
};

DeclSt(DevicePanel);

typedef struct _app_t App;

struct _app_t
{
    Util* util;
    LogWin* logwin;
    Help* help;
    Ble* ble;
    Connect* connect;

    Window* window;
    Label* label_nof_devices;
    Button* button_scan_startstop;
    Cell* cell_scan_startstop;
    Button* button_scan_clear;
    Cell* cell_scan_clear;
    Button* button_open_close_log;
    Edit* edit_device_name;
    Cell* cell_device_name;
    Edit* edit_mac_address;
    Cell* cell_mac_address;
    Edit* edit_rssi;
    Cell* cell_rssi;
    Button* check_connectable;
    Cell* cell_connectable;
    Button* check_paired;
    Cell* cell_paired;

    Layout* dev_layout;

    ArrSt(DevicePanel)* device_panel;

    simpleble_adapter_t adapter_handle;
    bool_t scanning_is_active;
    bool_t logwin_is_open;
    bool_t logwin_is_open_prev;
    bool_t help_is_open;
    bool_t bt_initialized;
    bool bt_enabled;
    bool bt_enabled_prev;
    int16_t rssi_threshold;
    uint32_t bt_check_count;
};

static App* i_app = NULL;

enum simpleble_init_result { BT_SUCCESS, BT_NO_ADAPTER, BT_DISABLED, BT_NO_HANDLE, BT_NO_CALLBACKS };


/*---------------------------------------------------------------------------*/

static void i_SimpleBleOnScanStart(simpleble_adapter_t adapter, void* userdata)
{
    char* id = simpleble_adapter_identifier(adapter);

    if (id != NULL)
    {
        log_printf("> Scanning started.\n");
    }
    i_app->scanning_is_active = TRUE;

    simpleble_free(id);
    unref(userdata);
}

static void i_SimpleBleOnScanStop(simpleble_adapter_t adapter, void* userdata)
{
    char* id = simpleble_adapter_identifier(adapter);

    if (id != NULL)
    {
        log_printf("> Scanning stopped.\n");
    }
    i_app->scanning_is_active = FALSE;

    simpleble_free(id);
    unref(userdata);
}

static int i_compare_mac(const DevicePanel* dev_panel, const String* s)
{
    return str_scmp(dev_panel->mac_address, s);
}

static void i_UpdateDevice(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata, bool_t updated)
{
    char* adapter_identifier = simpleble_adapter_identifier(adapter);
    char* peripheral_identifier = simpleble_peripheral_identifier(peripheral);
    char* peripheral_address = simpleble_peripheral_address(peripheral);

    if (adapter_identifier != NULL && peripheral_identifier != NULL && peripheral_address != NULL)
    {
        String* mac_address = str_c(peripheral_address);

        bool b = false;
        DevState is_connectable = UNDEF;
        DevState is_paired = UNDEF;
        if (simpleble_peripheral_is_connectable(peripheral, &b) == SIMPLEBLE_SUCCESS)
            is_connectable = b ? YES : NO;
        if (simpleble_peripheral_is_paired(peripheral, &b) == SIMPLEBLE_SUCCESS)
            is_paired = b ? YES : NO;

        bool_t match_filter = TRUE;

        if (!str_empty_c(edit_get_text(i_app->edit_device_name)) &&
            str_str(peripheral_identifier, edit_get_text(i_app->edit_device_name)) == NULL)
            match_filter = FALSE;

        String* cmp = str_c(edit_get_text(i_app->edit_mac_address));
        str_lower(cmp);
        if (!str_empty_c(edit_get_text(i_app->edit_mac_address)) &&
            str_str(tc(mac_address), tc(cmp)) == NULL)
            match_filter = FALSE;
        str_destroy(&cmp);

        if (simpleble_peripheral_rssi(peripheral) < i_app->rssi_threshold)
            match_filter = FALSE;

        if ((button_get_state(i_app->check_connectable) == ekGUI_ON) && (is_connectable == NO))
            match_filter = FALSE;

        if ((button_get_state(i_app->check_paired) == ekGUI_ON) && (is_paired == NO))
            match_filter = FALSE;

        if (match_filter)
        {
            if (!updated)
                ble_update_peripheral(peripheral, tc(mac_address), (uint8_t)simpleble_peripheral_address_type(peripheral),
                    simpleble_peripheral_rssi(peripheral), simpleble_peripheral_tx_power(peripheral),
                    (DevState)is_connectable, (DevState)is_paired, peripheral_identifier);
            else
                ble_update_peripheral(NULL, tc(mac_address), (uint8_t)simpleble_peripheral_address_type(peripheral),
                    simpleble_peripheral_rssi(peripheral), simpleble_peripheral_tx_power(peripheral),
                    (DevState)is_connectable, (DevState)is_paired, peripheral_identifier);

            bmutex_lock(log_mutex);
            log_printf("%s \a2\"%s\" \a3%ddB", tc(mac_address), peripheral_identifier, simpleble_peripheral_rssi(peripheral));

            if (simpleble_peripheral_tx_power(peripheral) != INT16_MIN)
            {
                log_printf(" \a4tx=%ddBm", simpleble_peripheral_tx_power(peripheral));
            }
            else
            {
                log_printf(" \a4tx=n/a");
            }

            if (is_connectable == YES)
            {
                log_printf(" \a5Connectable");
            }
            else
            {
                log_printf(" \a5Non-Connectable");
            }

            if (is_paired == YES)
            {
                log_printf(" \a5Paired");
            }
            else
            {
                log_printf(" \a5Non-Paired");
            }

            bmutex_unlock(log_mutex);
            
            if (simpleble_peripheral_services_count(peripheral) > 0)
            {
                size_t i;
                simpleble_service_t svc;

                for (i = 0; i < simpleble_peripheral_services_count(peripheral); ++i)
                {
                    simpleble_peripheral_services_get(peripheral, i, &svc);
                    ble_update_service_data(tc(mac_address), svc.uuid.value, (uint8_t)svc.data_length, svc.data);
                    bmutex_lock(log_mutex);
                    log_printf(" \a6svc-uuid=%s", svc.uuid.value);

                    if (svc.data_length > 0)
                    {
                        char d[55];
                        util_data_to_hex(svc.data, d, (byte_t)svc.data_length);
                        log_printf(" \a6svc-data=%s", d);
                    }

                    String* name = util_service_uuid_to_name(svc.uuid.value);
                    if (!str_empty(name))
                        log_printf(" \a7(%s)", tc(name));
                    bmutex_unlock(log_mutex);
                    str_destroy(&name);
                }
            }

            if (simpleble_peripheral_manufacturer_data_count(peripheral) > 0)
            {
                size_t i;
                simpleble_manufacturer_data_t manuf_data;

                for (i = 0; i < simpleble_peripheral_manufacturer_data_count(peripheral); ++i)
                {
                    simpleble_peripheral_manufacturer_data_get(peripheral, i, &manuf_data);
                    ble_update_manufacturer_data(tc(mac_address), manuf_data.manufacturer_id, (uint8_t)manuf_data.data_length, manuf_data.data);
                    bmutex_lock(log_mutex);
                    log_printf(" \a6company-id=0x%04X", manuf_data.manufacturer_id);

                    if (manuf_data.data_length > 0)
                    {
                        char d[55];
                        util_data_to_hex(manuf_data.data, d, (byte_t)manuf_data.data_length);
                        log_printf(" \a6manuf-data=%s", d);
                    }

                    String* name = util_company_id_to_name(manuf_data.manufacturer_id);
                    if (!str_empty(name))
                        log_printf(" \a7(%s)", tc(name));
                    bmutex_unlock(log_mutex);
                    str_destroy(&name);
                }
            }

            uint32_t pos;
            DevicePanel* device_panel = arrst_search(i_app->device_panel, i_compare_mac, mac_address, &pos, DevicePanel, String);

            if (device_panel == NULL)
            {
                device_panel = arrst_new0(i_app->device_panel, DevicePanel);
                device_panel->mac_address = mac_address;
                device_panel->dev_name = str_c(peripheral_identifier);
                device_panel->do_create = TRUE;
            }
            device_panel->do_update = TRUE;

            bmutex_lock(log_mutex);
            log_printf("\n");
            bmutex_unlock(log_mutex);
        }
        str_destroy(&mac_address);
    }

    simpleble_free(peripheral_address);
    simpleble_free(peripheral_identifier);

    unref(userdata);
}

static void i_SimpleBleOnFound(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata)
{
    i_UpdateDevice(adapter, peripheral, userdata, FALSE);
}

static void i_SimpleBleOnUpdated(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata)
{
    i_UpdateDevice(adapter, peripheral, userdata, TRUE);

    simpleble_peripheral_release_handle(peripheral);
}

/*---------------------------------------------------------------------------*/

static void i_enable_controls(App* app, bool_t state)
{
    cell_enabled(app->cell_scan_clear, state);
    cell_enabled(app->cell_connectable, state);
    cell_enabled(app->cell_paired, state);
    cell_enabled(app->cell_device_name, state);
    cell_enabled(app->cell_mac_address, state);
    cell_enabled(app->cell_rssi, state);
}

static void i_OnScanStartStopButton(App* app, Event* e)
{
    Button* button = event_sender(e, Button);

    if (app->scanning_is_active)
    {
        if (simpleble_adapter_scan_stop(app->adapter_handle) != SIMPLEBLE_SUCCESS)
        {
            log_printf("> \a1Could not stop scanning.\n");
        }
        else
        {
            button_text(button, "Start Scan");
            i_enable_controls(app, TRUE);
        }
    }
    else
    {
        if (simpleble_adapter_scan_start(app->adapter_handle) != SIMPLEBLE_SUCCESS)
        {
            log_printf("> \a1Could not start scanning.\n");
        }
        else
        {
            button_text(button, "Stop Scan");
            i_enable_controls(app, FALSE);
        }
    }

    unref(e);
}

static void i_remove_mac_address(DevicePanel* device_panel)
{
    str_destroy(&device_panel->mac_address);
}

static void i_OnScanClearButton(App* app, Event* e)
{
    int i;
    int32_t nrows = layout_nrows(app->dev_layout);
    for (i = nrows-1; i > 0; --i)
    {
        layout_remove_row(app->dev_layout, i);
    }
    arrst_clear(app->device_panel, i_remove_mac_address, DevicePanel);

    ble_clear_device_list();

    log_printf("> Cleared BLE device list.\n");

    label_text(app->label_nof_devices, "Press Start Scan to Discover Devices...");

    layout_update(app->dev_layout);

    unref(e);
}

static void i_OnHelpButton(App* app, Event* e)
{
    Button* b = event_sender(e, Button);

    if (help_is_open())
    {
        help_hide(app->help);
    }
    else
    {
        V2Df origin = window_get_origin(i_app->window);
        S2Df size = window_get_size(i_app->window);
        help_show(app->help, v2df(origin.x + size.width, origin.y));
    }

    unref(e);
}

static void i_OnShowLogWindow(App* app, Event* e)
{
    Button* b = event_sender(e, Button);

    if (logwin_is_open())
    {
        logwin_hide(app->logwin);
        app->logwin_is_open = TRUE;
        button_text(b, "Hide Log");
    }
    else
    {
        V2Df origin = window_get_origin(i_app->window);
        S2Df size = window_get_size(i_app->window);
        logwin_show(app->logwin, v2df(origin.x + size.width, origin.y));
        app->logwin_is_open = FALSE;
        button_text(b, "Show Log");
    }

    unref(e);
}

static void i_OnChangeRssi(App* app, Event* e)
{
    bool_t err;

    app->rssi_threshold = str_to_i16(edit_get_text(app->edit_rssi), 10, &err);
    if (err || (app->rssi_threshold > 0) || (app->rssi_threshold < -100))
    {
        app->rssi_threshold = -100;
        log_printf("> \a1Invalid rssi (-100 <= rssi <= 0).\n");
    }

    unref(e);
}

static void i_OnChangeMacAddress(App* app, Event* e)
{
    String* mac = str_c(edit_get_text(app->edit_mac_address));

    if (util_buffer_is_mac_address(tc(mac), (size_t)str_len(mac)) != TRUE)
    {
        edit_bgcolor(app->edit_mac_address, color_html("#F1948A"));
        log_printf("> \a1Invalid MAC address in filter.\n");
    }
    else
    {
        edit_bgcolor(app->edit_mac_address, kCOLOR_DEFAULT);
    }

//    edit_text(app->edit_mac_address, tc(mac));
    str_destroy(&mac);

    unref(e);
}

/*---------------------------------------------------------------------------*/

static Panel* i_panel(App* app)
{
    Panel* panel_static = panel_create();

    Layout* layout = layout_create(1, 6);
    Layout* layout_progname = layout_create(2, 1);
    Layout* layout_buttons = layout_create(3, 1);
    Layout* layout_filtertitle = layout_create(1, 1);
    Layout* layout_filtervalues = layout_create(4, 3);
    Layout* layout_nof_devices = layout_create(1, 1);


    layout_layout(layout, layout_progname, 0, 0);
    layout_layout(layout, layout_buttons, 0, 1);
    layout_layout(layout, layout_filtertitle, 0, 2);
    layout_layout(layout, layout_filtervalues, 0, 3);
    layout_layout(layout, layout_nof_devices, 0, 4);
    layout_vexpand(layout, 5);


    Font* font_progname = font_system(20, ekFBOLD);
    Label* label_progname = label_create();
    label_text(label_progname, "BLE Scout - Bluetooth LE Tool");
    label_font(label_progname, font_progname);
    label_color(label_progname, color_html("#2E86C1"));
    layout_label(layout_progname, label_progname, 0, 0);
    layout_hexpand(layout_progname, 0);
    cell_padding4(layout_cell(layout_progname, 0, 0), 0, 0, 5, 10);

    Button* button_help = button_push();
    button_text(button_help, "Help");
    button_OnClick(button_help, listener(app, i_OnHelpButton, App));
    layout_button(layout_progname, button_help, 1, 0);
    cell_padding4(layout_cell(layout_progname, 1, 0), 0, 10, 5, 0);

    app->button_scan_startstop = button_push();
    button_text(app->button_scan_startstop, "Start Scan");
    button_OnClick(app->button_scan_startstop, listener(app, i_OnScanStartStopButton, App));
    layout_button(layout_buttons, app->button_scan_startstop, 0, 0);
    app->cell_scan_startstop = layout_cell(layout_buttons, 0, 0);
    cell_padding4(layout_cell(layout_buttons, 0, 0), 0, 10, 0, 0);

    app->button_scan_clear = button_push();
    button_text(app->button_scan_clear, "Clear");
    button_OnClick(app->button_scan_clear, listener(app, i_OnScanClearButton, App));
    layout_button(layout_buttons, app->button_scan_clear, 1, 0);
    app->cell_scan_clear = layout_cell(layout_buttons, 1, 0);
    cell_padding4(layout_cell(layout_buttons, 1, 0), 0, 10, 0, 10);

    app->button_open_close_log = button_push();
    button_text(app->button_open_close_log, "Show Log");
    button_OnClick(app->button_open_close_log, listener(app, i_OnShowLogWindow, App));
    layout_button(layout_buttons, app->button_open_close_log, 2, 0);
    cell_padding4(layout_cell(layout_buttons, 2, 0), 0, 0, 0, 10);

    layout_margin4(layout_buttons, 5, 10, 10, 10);


    Font* font_filters = font_system(14, ekFBOLD);
    Label* label_filters = label_create();
    label_text(label_filters, "Filter Devices by");
    label_font(label_filters, font_filters);
    label_color(label_filters, color_html("#566573"));
    layout_label(layout_filtertitle, label_filters, 0, 0);
    cell_padding4(layout_cell(layout_filtertitle, 0, 0), 0, 0, 5, 10);
    layout_halign(layout, 0, 2, ekCENTER);


    Label* label_devicename = label_create();
    label_text(label_devicename, "Device Name");
    layout_label(layout_filtervalues, label_devicename, 0, 0);
    cell_padding4(layout_cell(layout_filtervalues, 0, 0), 0, 5, 0, 0);

    Label* label_macaddress = label_create();
    label_text(label_macaddress, "MAC Address");
    layout_label(layout_filtervalues, label_macaddress, 1, 0);
    cell_padding4(layout_cell(layout_filtervalues, 1, 0), 0, 5, 0, 5);

    Label* label_rssi = label_create();
    label_text(label_rssi, "RSSI");
    layout_label(layout_filtervalues, label_rssi, 2, 0);
    cell_padding4(layout_cell(layout_filtervalues, 2, 0), 0, 5, 0, 5);

    app->edit_device_name = edit_create();
    layout_edit(layout_filtervalues, app->edit_device_name, 0, 1);
    app->cell_device_name = layout_cell(layout_filtervalues, 0, 1);
    cell_padding4(layout_cell(layout_filtervalues, 0, 1), 0, 5, 0, 0);

    app->edit_mac_address = edit_create();
    edit_OnChange(app->edit_mac_address, listener(app, i_OnChangeMacAddress, App));
    layout_edit(layout_filtervalues, app->edit_mac_address, 1, 1);
    app->cell_mac_address = layout_cell(layout_filtervalues, 1, 1);
    cell_padding4(layout_cell(layout_filtervalues, 1, 1), 0, 5, 0, 5);

    app->edit_rssi = edit_create();
    edit_text(app->edit_rssi, "-100");
    edit_OnChange(app->edit_rssi, listener(app, i_OnChangeRssi, App));
    layout_edit(layout_filtervalues, app->edit_rssi, 2, 1);
    app->cell_rssi = layout_cell(layout_filtervalues, 2, 1);
    cell_padding4(layout_cell(layout_filtervalues, 2, 1), 0, 5, 0, 5);

    app->check_connectable = button_check();
    button_text(app->check_connectable, "connectable");
    layout_button(layout_filtervalues, app->check_connectable, 3, 0);
    app->cell_connectable = layout_cell(layout_filtervalues, 3, 0);
    cell_padding4(layout_cell(layout_filtervalues, 3, 0), 0, 5, 0, 0);

    app->check_paired = button_check();
    button_text(app->check_paired, "paired");
    layout_button(layout_filtervalues, app->check_paired, 3, 1);
    app->cell_paired = layout_cell(layout_filtervalues, 3, 1);
    cell_padding4(layout_cell(layout_filtervalues, 3, 1), 0, 5, 0, 0);

    layout_margin4(layout_filtervalues, 0, 10, 5, 10);

    Font* font_nof_devices = font_system(14, ekFBOLD);
    app->label_nof_devices = label_create();
    label_text(app->label_nof_devices, "Press Start Scan to Discover Devices...");
    label_font(app->label_nof_devices, font_nof_devices);
    label_color(app->label_nof_devices, color_html("#566573"));
    label_align(app->label_nof_devices, ekCENTER);
    layout_label(layout_nof_devices, app->label_nof_devices, 0, 0);
    layout_halign(layout_nof_devices, 0, 0, ekCENTER);
    layout_valign(layout_nof_devices, 0, 0, ekTOP);
    cell_padding4(layout_cell(layout_nof_devices, 0, 0), 5, 5, 5, 5);


    Panel* panel_dynamic = panel_scroll(FALSE, TRUE);
    real32_t scrw = panel_scroll_width(panel_dynamic);

    layout_panel(layout, panel_dynamic, 0, 5);

    Layout* layout_devices = layout_create(1, 2);
    app->dev_layout = layout_create(1, 1);
    layout_layout(layout_devices, app->dev_layout, 0, 0);
    layout_margin4(layout_devices, 0, scrw, 0, 0);
    layout_vexpand(layout_devices, 1);


    panel_size(panel_dynamic, s2df(400, 500));

    panel_layout(panel_static, layout);
    panel_layout(panel_dynamic, layout_devices);


    return panel_static;
}

/*---------------------------------------------------------------------------*/

static void i_OnClose(App* app, Event* e)
{
    logwin_hide(app->logwin);

    osapp_finish();
    
    unref(app);
    unref(e);
}

/*---------------------------------------------------------------------------*/

static int i_init_simpleble(App* app)
{
    if (simpleble_adapter_get_count() == 0)
        return BT_NO_ADAPTER;

    if (simpleble_adapter_is_bluetooth_enabled() != true)
        return BT_DISABLED;

    app->adapter_handle = simpleble_adapter_get_handle(0);
    if (app->adapter_handle == 0)
        return BT_NO_HANDLE;

    int i = simpleble_adapter_set_callback_on_scan_start(app->adapter_handle, i_SimpleBleOnScanStart, NULL);
    i += simpleble_adapter_set_callback_on_scan_stop(app->adapter_handle, i_SimpleBleOnScanStop, NULL);
    i += simpleble_adapter_set_callback_on_scan_found(app->adapter_handle, i_SimpleBleOnFound, NULL);
    i += simpleble_adapter_set_callback_on_scan_updated(app->adapter_handle, i_SimpleBleOnUpdated, NULL);
    if (i > 0)
        return BT_NO_CALLBACKS;

    app->bt_initialized = TRUE;

    return BT_SUCCESS;
}

static App* i_create(void)
{
    log_file("blescout.log");

    App* app = heap_new0(App);
    i_app = app;

    app->scanning_is_active = FALSE;
    app->logwin_is_open = FALSE;
    app->bt_initialized = FALSE;
    app->bt_check_count = (uint32_t)(BT_CHECK_RATE_sec / UDPATE_RATE_sec);
    app->rssi_threshold = -100;

    app->util = util_create();
    app->logwin = logwin_create();
    app->help = help_create();
    app->ble = ble_create();
    app->connect = connect_create();

    app->device_panel = arrst_create(DevicePanel);

    Panel* panel = i_panel(app);

    app->window = window_create(ekWINDOW_STD | ekWINDOW_RESIZE);
    window_panel(app->window, panel);
    String* title = str_printf("BLE Scout V%s", BLESCOUT_VERSION);
    window_title(app->window, tc(title));
    str_destroy(&title);
    window_origin(app->window, v2df(200, 200));
    window_OnClose(app->window, listener(app, i_OnClose, App));
    window_show(app->window);

    cell_enabled(app->cell_scan_startstop, FALSE);

    log_printf("> BLE Scout - Bluetooth LE Tool version %s  (https://github.com/eriklins/BLE-Scout)\n", BLESCOUT_VERSION);

    log_printf("> Uses SimpleBLE library version %s  (https://github.com/OpenBluetoothToolbox/SimpleBLE)\n", simpleble_get_version());

    int ret = i_init_simpleble(app);
    if (ret != BT_SUCCESS)
    {
        switch (ret)
        {
        case BT_NO_ADAPTER:
            log_printf("> \a1No Bluetooth adapter found.\n");
            label_text(app->label_nof_devices, "No Bluetooth adapter found.");
            label_color(app->label_nof_devices, kCOLOR_RED);
            app->bt_enabled = false;
            break;
        case BT_DISABLED:
            log_printf("> \a1Bluetooth is disabled.\n");
            label_text(app->label_nof_devices, "Bluetooth is disabled.");
            label_color(app->label_nof_devices, kCOLOR_RED);
            app->bt_enabled = false;
            break;
        case BT_NO_HANDLE:
            log_printf("> \a1Could not get Bluetooth adapter handle.\n");
            label_text(app->label_nof_devices, "Could not get Bluetooth adapter handle.");
            label_color(app->label_nof_devices, kCOLOR_RED);
            break;
        case BT_NO_CALLBACKS:
            log_printf("> \a1Could not register BLE callback functions.\n");
            label_text(app->label_nof_devices, "Could not register BLE callback functions.");
            label_color(app->label_nof_devices, kCOLOR_RED);
            break;
        }
        log_printf("> SimpleBLE not initialized.\n");
    }
    else
    {
        log_printf("> SimpleBLE initialized.\n");
        app->bt_initialized = TRUE;
        app->bt_enabled = true;
        cell_enabled(app->cell_scan_startstop, TRUE);
    }
    
    app->bt_enabled_prev = app->bt_enabled;
    app->logwin_is_open_prev = app->logwin_is_open;

    return app;
}

static void i_destroy(App** app)
{
    arrst_destroy(&(*app)->device_panel, i_remove_mac_address, DevicePanel);
    window_destroy(&(*app)->window);
    connect_destroy(&(*app)->connect);
    ble_destroy(&(*app)->ble);
    logwin_destroy(&(*app)->logwin);
    util_destroy(&(*app)->util);
    heap_delete(app, App);
}

/*---------------------------------------------------------------------------*/

static void i_OnCheckButtonHexAscii(App* app, Event* e)
{
    Button* b = event_sender(e, Button);
    gui_state_t btn_state = button_get_state(b);
    uint32_t tag = button_get_tag(b);
    uint32_t dev_idx = (tag >> SH_DEVICE) & 0xff;
    uint32_t svc_idx = (tag >> SH_SVC_DATA) & 0xff;
    uint32_t mnf_idx = (tag >> SH_MNF_DATA) & 0xff;

    DevicePanel* device_panel = arrst_get(app->device_panel, dev_idx, DevicePanel);
    cassert(device_panel != NULL);

    BlePeripheral* per = ble_get_peripheral(tc(device_panel->mac_address));
    cassert(per != NULL);

    char_t d[55];

    if (svc_idx > 0)
    {
        --svc_idx;
        BleService* svc = arrst_get(per->services, svc_idx, BleService);
        cassert(svc != NULL);

        if (btn_state == ekGUI_ON)
        {
            util_data_to_ascii(svc->data, d, svc->data_len);
            edit_text(device_panel->edit_svc_data[svc_idx], d);
        }
        else
        {
            util_data_to_hex(svc->data, d, svc->data_len);
            edit_text(device_panel->edit_svc_data[svc_idx], d);

        }
    }
    else if (mnf_idx > 0)
    {
        --mnf_idx;
        BleManufacturerData* mnf = arrst_get(per->manufacturer_data, mnf_idx, BleManufacturerData);
        if (btn_state == ekGUI_ON)
        {
            util_data_to_ascii(mnf->data, d, mnf->data_len);
            edit_text(device_panel->edit_mnf_data[mnf_idx], d);
        }
        else
        {
            util_data_to_hex(mnf->data, d, mnf->data_len);
            edit_text(device_panel->edit_mnf_data[mnf_idx], d);

        }
    }

    unref(e);
}

static int i_compare_tag(const DevicePanel* device_panel, const uint32_t *tag)
{
    if (device_panel->row_index == *tag)
        return 0;
    return 1;
}

static void i_OnClickConnectDevice(DevicePanel* dev, Event* e)
{
    cell_enabled(dev->cell_connect, FALSE);

    V2Df origin = window_get_origin(i_app->window);
    S2Df size = window_get_size(i_app->window);

    if(connect_device(dev->mac_address, v2df(origin.x + size.width, origin.y)) != TRUE)
        cell_enabled(dev->cell_connect, TRUE);

    unref(e);
}

static void i_create_device(App* app, DevicePanel* device_panel, BlePeripheral* peripheral, int32_t row)
{
    int32_t sub_rows = layout_nrows(app->dev_layout);
    cassert(sub_rows >= 1);

    if (row == -1)
    {
        layout_insert_row(app->dev_layout, sub_rows);
        device_panel->row_index = sub_rows;
    }
    else
    {
        layout_remove_row(app->dev_layout, row);
        layout_insert_row(app->dev_layout, row);
        device_panel->row_index = row;
    }

    device_panel->sublayout = layout_create(1, 3);
    layout_margin(device_panel->sublayout, 10);
    layout_skcolor(device_panel->sublayout, color_html("#ABB2B9"));
    layout_bgcolor(device_panel->sublayout, color_html("#EBF5FB"));

    Layout* layout_name_connect = layout_create(3, 1);
    Layout* layout_conn_tx_rssi = layout_create(3, 1);
    Layout* layout_mac_address = layout_create(1, 1);


    Font* font_dev_name = font_system(16, ekFBOLD);
    device_panel->edit_dev_name = edit_create();
    edit_font(device_panel->edit_dev_name, font_dev_name);
    edit_text(device_panel->edit_dev_name, "\"Device Name\"");
    edit_editable(device_panel->edit_dev_name, FALSE);
    layout_edit(layout_name_connect, device_panel->edit_dev_name, 0, 0);
    layout_hsize(layout_name_connect, 0, 250.0);

    device_panel->button_connect = button_push();
    button_text(device_panel->button_connect, "Connect");
    button_tag(device_panel->button_connect, device_panel->row_index);
    button_OnClick(device_panel->button_connect, listener(device_panel, i_OnClickConnectDevice, DevicePanel));

    layout_button(layout_name_connect, device_panel->button_connect, 2, 0);
    device_panel->cell_connect = layout_cell(layout_name_connect, 2, 0);

    layout_hexpand(layout_name_connect, 1);
    layout_layout(device_panel->sublayout, layout_name_connect, 0, 0);
    layout_margin4(layout_name_connect, 0, 0, 5, 0);


    device_panel->label_connectable = label_create();
    label_text(device_panel->label_connectable, "Connectable: xxx");
    layout_label(layout_conn_tx_rssi, device_panel->label_connectable, 0, 0);
    device_panel->label_tx_power = label_create();
    label_text(device_panel->label_tx_power, "TX Power: xxdB");
    layout_label(layout_conn_tx_rssi, device_panel->label_tx_power, 1, 0);
    device_panel->label_rssi = label_create();
    label_text(device_panel->label_rssi, "RSSI: -xxdBm");
    layout_label(layout_conn_tx_rssi, device_panel->label_rssi, 2, 0);

    layout_layout(device_panel->sublayout, layout_conn_tx_rssi, 0, 1);


    device_panel->label_mac_address = label_create();
    label_text(device_panel->label_mac_address, "MAC Address: XX:XX:XX:XX:XX:XX");
    layout_label(layout_mac_address, device_panel->label_mac_address, 0, 0);

    layout_layout(device_panel->sublayout, layout_mac_address, 0, 2);


    device_panel->svc_count = (uint32_t)peripheral->service_count;
    device_panel->mnf_count = (uint32_t)peripheral->manufacturer_data_count;

    if (device_panel->svc_count > 0)
    {
        uint32_t rows = layout_nrows(device_panel->sublayout);
        cassert(rows >= 1);

        layout_insert_row(device_panel->sublayout, rows);

        Layout* layout_svc = layout_create(3, device_panel->svc_count + 1);

        Font* font_title = font_system(12, ekFBOLD);

        Label* label_title = label_create();
        label_text(label_title, "Advertised Services:");
        label_font(label_title, font_title);
        layout_label(layout_svc, label_title, 0, 0);
        layout_margin4(layout_svc, 5, 0, 0, 0);

        Label* label_svc_uuid[14];
        uint32_t i;
        for (i = 0; i < device_panel->svc_count; ++i)
        {
            label_svc_uuid[i] = label_create();
            BleService* svc = arrst_get(peripheral->services, i, BleService);

            String* name = util_service_uuid_to_name(svc->uuid.value);
            if (str_empty(name))
            {
                str_destroy(&name);
                name = util_vsp_service_uuid_to_name(svc->uuid.value);
                if (str_empty(name))
                {
                    str_destroy(&name);
                    name = str_c(svc->uuid.value);
                }
            }
            label_text(label_svc_uuid[i], tc(name));
            str_destroy(&name);
            layout_label(layout_svc, label_svc_uuid[i], 0, i + 1);
            layout_hsize(layout_svc, 0, 220.0);

            if (svc->data_len > 0)
            {
                device_panel->edit_svc_data[i] = edit_create();
                edit_editable(device_panel->edit_svc_data[i], FALSE);
                layout_edit(layout_svc, device_panel->edit_svc_data[i], 1, i + 1);
                device_panel->check_svc_ascii[i] = button_check();
                button_tag(device_panel->check_svc_ascii[i], (device_panel->row_index << SH_DEVICE) | ((i+1) << SH_SVC_DATA));
                button_OnClick(device_panel->check_svc_ascii[i], listener(app, i_OnCheckButtonHexAscii, App));
                button_text(device_panel->check_svc_ascii[i], "ASCII");
                layout_button(layout_svc, device_panel->check_svc_ascii[i], 2, i + 1);
                device_panel->cell_svc_ascii[i] = layout_cell(layout_svc, 2, i + 1);
            }
            layout_hmargin(layout_svc, 0, 5);
            layout_hmargin(layout_svc, 1, 5);
        }

        layout_hexpand(layout_svc, 0);
        layout_layout(device_panel->sublayout, layout_svc, 0, rows);
    }

    if (device_panel->mnf_count > 0)
    {
        uint32_t rows = layout_nrows(device_panel->sublayout);
        cassert(rows >= 1);

        layout_insert_row(device_panel->sublayout, rows);

        Layout* layout_mnf = layout_create(2, 2* device_panel->mnf_count + 1);

        Font* font_title = font_system(12, ekFBOLD);

        Label* label_title = label_create();
        label_text(label_title, "Manufacturer Specific Data:");
        label_font(label_title, font_title);
        layout_label(layout_mnf, label_title, 0, 0);
        layout_margin4(layout_mnf, 5, 0, 0, 0);

        Label* label_mnf_uuid[14];
        uint32_t i;
        for (i = 0; i < device_panel->mnf_count; ++i)
        {
            label_mnf_uuid[i] = label_create();
            BleManufacturerData* mnf = arrst_get(peripheral->manufacturer_data, i, BleManufacturerData);
            String* name = util_company_id_to_name(mnf->comp_id);
            if (str_empty(name))
            {
                String* s = str_printf("Company ID: 0x%04X", mnf->comp_id);
                label_text(label_mnf_uuid[i], tc(s));
                str_destroy(&s);
            }
            else
                label_text(label_mnf_uuid[i], tc(name));
            str_destroy(&name);
            layout_label(layout_mnf, label_mnf_uuid[i], 0, 2*i + 1);
            layout_hsize(layout_mnf, 0, 200.0);

            if (mnf->data_len > 0)
            {
                device_panel->edit_mnf_data[i] = edit_create();
                edit_editable(device_panel->edit_mnf_data[i], FALSE);
                layout_edit(layout_mnf, device_panel->edit_mnf_data[i], 0, 2*i + 2);
                device_panel->check_mnf_ascii[i] = button_check();
                button_tag(device_panel->check_mnf_ascii[i], (device_panel->row_index << SH_DEVICE) | ((i+1) << SH_MNF_DATA));
                button_OnClick(device_panel->check_mnf_ascii[i], listener(app, i_OnCheckButtonHexAscii, App));
                button_text(device_panel->check_mnf_ascii[i], "ASCII");
                layout_button(layout_mnf, device_panel->check_mnf_ascii[i], 1, 2*i + 2);
                device_panel->cell_mnf_ascii[i] = layout_cell(layout_mnf, 2, i + 1);
            }
            layout_hmargin(layout_mnf, 0, 5);
        }

        layout_hexpand(layout_mnf, 0);
        layout_layout(device_panel->sublayout, layout_mnf, 0, rows);
    }

    layout_layout(app->dev_layout, device_panel->sublayout, 0, device_panel->row_index);

    cell_padding4(layout_cell(app->dev_layout, 0, device_panel->row_index), 5, 10, 5, 5);

    layout_hsize(app->dev_layout, device_panel->row_index, 400);

    layout_update(app->dev_layout);
}

static void i_update(App* app, const real64_t prtime, const real64_t ctime)
{
    logwin_update();

    if (!app->bt_initialized)
    {
        if (app->bt_check_count == 0)
        {
            int i = i_init_simpleble(app);

            if (i == 0)
            {
                app->bt_initialized = TRUE;
                cell_enabled(app->cell_scan_startstop, TRUE);
                log_printf("> SimpleBLE initialized.\n");
                label_text(app->label_nof_devices, "Press Start Scan to Discover Devices...");
                label_color(app->label_nof_devices, color_html("#566573"));
                layout_update(app->dev_layout);
            }
            app->bt_check_count = (uint32_t)(BT_CHECK_RATE_sec / UDPATE_RATE_sec);
        }
        else
            app->bt_check_count--;
    }

    app->bt_enabled = simpleble_adapter_is_bluetooth_enabled();
    if (app->bt_enabled != app->bt_enabled_prev)
    {
        if (!app->bt_enabled) {
            app->scanning_is_active = FALSE;
            button_text(app->button_scan_startstop, "Start Scan");
            i_enable_controls(app, TRUE);
            cell_enabled(app->cell_scan_startstop, FALSE);
            log_printf("> \a1Bluetooth is disabled.\n");
            label_text(app->label_nof_devices, "Bluetooth is disabled.");
            label_color(app->label_nof_devices, kCOLOR_RED);
        }
        else
        {
            cell_enabled(app->cell_scan_startstop, TRUE);
            log_printf("> Bluetooth is enabled.\n");
            label_text(app->label_nof_devices, "Press Start Scan to Discover Devices...");
            label_color(app->label_nof_devices, color_html("#566573"));
            layout_update(app->dev_layout);
        }
        app->bt_enabled_prev = app->bt_enabled;
    }

    app->logwin_is_open = logwin_is_open();
    if (app->logwin_is_open != app->logwin_is_open_prev)
    {
        if (logwin_is_open())
        {
            app->logwin_is_open = TRUE;
            button_text(app->button_open_close_log, "Hide Log");
        }
        else
        {
            app->logwin_is_open = FALSE;
            button_text(app->button_open_close_log, "Show Log");
        }
        app->logwin_is_open_prev = app->logwin_is_open;
    }

    if (app->scanning_is_active)
    {
        arrst_foreach(device_panel, app->device_panel, DevicePanel)

            if (device_panel->do_update)
            {
                BlePeripheral* ble_peripheral = ble_get_peripheral(tc(device_panel->mac_address));
                if (ble_peripheral != NULL)
                {
                    if (device_panel->do_create)
                    {
                        i_create_device(app, device_panel, ble_peripheral, -1);
                        device_panel->do_create = FALSE;
                    }
                    else
                    {
                        if (ble_peripheral->service_count > device_panel->svc_count)
                        {
                            i_create_device(app, device_panel, ble_peripheral, device_panel->row_index);
                            device_panel->do_create = FALSE;
                        }
                        if (ble_peripheral->manufacturer_data_count > device_panel->mnf_count)
                        {
                            i_create_device(app, device_panel, ble_peripheral, device_panel->row_index);
                            device_panel->do_create = FALSE;
                        }
                    }

                    if (str_empty(ble_peripheral->identifier))
                        edit_text(device_panel->edit_dev_name, "unknown");
                    else
                        edit_text(device_panel->edit_dev_name, tc(ble_peripheral->identifier));
                    String* msg = str_printf("MAC Address: %s", tc(ble_peripheral->mac_address));
                    label_text(device_panel->label_mac_address, tc(msg));
                    str_destroy(&msg);
                    if (ble_peripheral->is_connectable == YES)
                    {
                        label_text(device_panel->label_connectable, "Connectable: Yes");
                        cell_enabled(device_panel->cell_connect, TRUE);
                    }
                    else if (ble_peripheral->is_connectable == NO)
                    {
                        label_text(device_panel->label_connectable, "Connectable: No");
                        cell_enabled(device_panel->cell_connect, FALSE);
                    }
                    else
                    {
                        label_text(device_panel->label_connectable, "Connectable: n/a");
                        cell_enabled(device_panel->cell_connect, FALSE);
                    }
                    if (ble_peripheral->tx_power != INT16_MIN)
                        msg = str_printf("TX Power: %ddBm", ble_peripheral->tx_power);
                    else
                        msg = str_printf("TX Power: n/a");
                    label_text(device_panel->label_tx_power, tc(msg));
                    str_destroy(&msg);
                    msg = str_printf("RSSI: %ddB", ble_peripheral->rssi);
                    label_text(device_panel->label_rssi, tc(msg));
                    str_destroy(&msg);

                    uint32_t i;
                    char data[55];
                    for (i = 0; i < ble_peripheral->service_count; ++i)
                    {
                        BleService* svc = arrst_get(ble_peripheral->services, i, BleService);
                        if (svc->data_len > 0)
                        {
                            if (button_get_state(device_panel->check_svc_ascii[i]) == ekGUI_ON)
                                util_data_to_ascii(svc->data, data, svc->data_len);
                            else
                                util_data_to_hex(svc->data, data, svc->data_len);
                            edit_text(device_panel->edit_svc_data[i], data);
                        }
                    }

                    for (i = 0; i < ble_peripheral->manufacturer_data_count; ++i)
                    {
                        BleManufacturerData* mnf = arrst_get(ble_peripheral->manufacturer_data, i, BleManufacturerData);
                        if (mnf->data_len > 0)
                        {
                            if (button_get_state(device_panel->check_mnf_ascii[i]) == ekGUI_ON)
                                util_data_to_ascii(mnf->data, data, mnf->data_len);
                            else
                                util_data_to_hex(mnf->data, data, mnf->data_len);
                            edit_text(device_panel->edit_mnf_data[i], data);
                        }
                    }
                }
                device_panel->do_update = FALSE;
            }

        arrst_end()

        if (arrst_size(app->device_panel, DevicePanel) > 0)
        {
            String* msg = str_printf("%d Devices Discovered.", arrst_size(app->device_panel, DevicePanel));
            label_text(app->label_nof_devices, tc(msg));
            str_destroy(&msg);
        }
    }

    unref(prtime);
    unref(ctime);
}

/*---------------------------------------------------------------------------*/

#include "osmain.h"
osmain_sync(UDPATE_RATE_sec, i_create, i_destroy, i_update, "", App)
