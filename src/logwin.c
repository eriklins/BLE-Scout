/* Log window */

#include <nappgui.h>
#include "ble.h"
#include "logwin.h"

#define LOG_STREAM_SIZE 1024

const color_t colors[] = {
    0xFF503E2C,  // 0x2C3E50FF, dark grey
    0xFF2B39C0,  // 0xC0392BFF, dark red
    0xFFA67428,  // 0x2874A6FF, blue
    0xFF1E6FCA,  // 0xCA6F1EFF, orange
    0xFF49841E,  // 0x1E8449FF, green
    0xFF0A7D9A,  // 0x9A7D0AFF, brown
    0xFFA04E88,  // 0x884EA0FF, light violet
    0xFF83346C   // 0x6C3483FF, dark violet
};


struct _logwin_t
{
    Window* window;
    TextView* text_log;

    V2Df origin;
    bool_t add_timestamp;

    bool_t is_open;

    Stream* stream;
    Mutex* mutex;
};

static LogWin* i_logwin = NULL;

Stream* log_stream = NULL;
Mutex* log_mutex = NULL;

/*---------------------------------------------------------------------------*/

void logwin_update(void)
{
    char_t* line = NULL;

    do
    {
        bmutex_lock(i_logwin->mutex);
        line = stm_read_line(i_logwin->stream);
        bmutex_unlock(i_logwin->mutex);

        if (line != NULL)
        {
            if (i_logwin->add_timestamp)
            {
                Date date;

                btime_to_date(btime_now(), &date);
                textview_color(i_logwin->text_log, color_gray(128));
                textview_printf(i_logwin->text_log, "[%04d-%02d-%02d_%02d:%02d:%02d] ",
                    date.year, date.month, date.mday, date.hour, date.minute, date.second);
            }

            uint32_t i, j = 0;
            uint32_t len = str_len_c(line);
            for (i = 0; i < len; ++i)
            {
                if (line[j] == '\a')
                {
                    line[j] = '\0';
                    textview_writef(i_logwin->text_log, line);
                    j++;
                    textview_color(i_logwin->text_log, colors[line[j] - '0']);
                    j++;
                    line += j;
                    j = 0;
                }
                else if (i == len - 1)
                {
                    textview_writef(i_logwin->text_log, line);
                }
                else
                    j++;
            }

            textview_color(i_logwin->text_log, colors[0]);
            textview_writef(i_logwin->text_log, "\n");

            //textview_select(i_logwin->text_log, -1, -1);
            textview_scroll_caret(i_logwin->text_log);
        }

    } while (line != NULL);
}

/*---------------------------------------------------------------------------*/

static void i_OnSetTimestamp(LogWin* log_win, Event* e)
{
    Button* button = event_sender(e, Button);

    if (button_get_state(button) == ekGUI_ON)
        log_win->add_timestamp = TRUE;
    else
        log_win->add_timestamp = FALSE;

    unref(log_win);
    unref(e);
}

static void i_OnClearLog(LogWin* log_win, Event* e)
{
    textview_clear(log_win->text_log);

    unref(log_win);
    unref(e);
}

static void i_OnCopyLog(LogWin* log_win, Event* e)
{
    textview_select(log_win->text_log, 0, INT32_MAX);
    textview_copy(log_win->text_log);
    textview_select(log_win->text_log, 0, 0);

    unref(log_win);
    unref(e);
}

static void i_OnSaveLog(LogWin* log_win, Event* e)
{
    Date date;

    btime_to_date(btime_now(), &date);

    String* msg = str_printf("%04d%02d%02d-%02d%02d%02d_nappble.log",
        date.year, date.month, date.mday, date.hour, date.minute, date.second);

    File* file = bfile_create(tc(msg), NULL);
    if (file != NULL)
    {
        const char_t* text = textview_get_text(log_win->text_log);
        bfile_write(file, (byte_t*)text, str_len_c(text), NULL, NULL);
        bfile_close(&file);
    }
    else
        stm_printf(log_stream, "> Cannot open log file.\n");

    str_destroy(&msg);

    unref(log_win);
    unref(e);
}

static void i_OnClose(LogWin* log, Event* e)
{
    window_hide(log->window);

    log->is_open = FALSE;

    unref(e);
}

/*---------------------------------------------------------------------------*/

static Panel* i_panel(LogWin* log_win)
{
    Panel* panel = panel_create();

    Layout* layout = layout_create(1, 2);
    Layout* layout_buttons = layout_create(7, 1);

    Button* button_timestamp = button_check();
    button_text(button_timestamp, "Timestamps");
    button_OnClick(button_timestamp, listener(log_win, i_OnSetTimestamp, LogWin));

    Button* button_clear_log = button_push();
    button_text(button_clear_log, "Clear Log");
    button_OnClick(button_clear_log, listener(log_win, i_OnClearLog, LogWin));

    Button* button_copy_log = button_push();
    button_text(button_copy_log, "Copy Log");
    button_OnClick(button_copy_log, listener(log_win, i_OnCopyLog, LogWin));

    Button* button_save_log = button_push();
    button_text(button_save_log, "Save Log");
    button_OnClick(button_save_log, listener(log_win, i_OnSaveLog, LogWin));

    layout_button(layout_buttons, button_timestamp, 0, 0);
    layout_button(layout_buttons, button_clear_log, 2, 0);
    layout_button(layout_buttons, button_copy_log, 4, 0);
    layout_button(layout_buttons, button_save_log, 6, 0);
    layout_margin4(layout_buttons, 0, 0, 10, 0);
    layout_layout(layout, layout_buttons, 0, 0);

    log_win->text_log = textview_create();
    textview_family(log_win->text_log, "Source Code Pro");
    textview_fsize(log_win->text_log, 11);
    layout_textview(layout, log_win->text_log, 0, 1);
    textview_size(log_win->text_log, s2df(1200, 400));

    layout_hsize(layout, 0, 680);
    layout_hsize(layout, 1, 680);
    layout_vsize(layout, 1, 400);
    layout_margin(layout, 5);

    layout_vexpand(layout, 1);

    panel_layout(panel, layout);

    return panel;
}

/*---------------------------------------------------------------------------*/

LogWin* logwin_create(void)
{
    LogWin* log = heap_new0(LogWin);

    i_logwin = log;

    log->stream = stm_memory(LOG_STREAM_SIZE);
    log_stream = log->stream;

    log->mutex = bmutex_create();
    log_mutex = log->mutex;

    Panel* panel = i_panel(log);

    log->window = window_create(ekWINDOW_STD | ekWINDOW_RESIZE);
    window_panel(log->window, panel);
    window_title(log->window, "BLE Scout - Log Output");
    window_OnClose(log->window, listener(log, i_OnClose, LogWin));

    log->add_timestamp = FALSE;
    log->is_open = FALSE;

    textview_color(i_logwin->text_log, colors[0]);

    return log;
}

void logwin_destroy(LogWin** log_win)
{
    bmutex_close(&(*log_win)->mutex);
    stm_close(&(*log_win)->stream);
    window_destroy(&(*log_win)->window);
    heap_delete(log_win, LogWin);
}

/*---------------------------------------------------------------------------*/

void logwin_show(LogWin* log_win, const V2Df pos)
{
    window_origin(log_win->window, pos);
    window_show(log_win->window);

    log_win->is_open = TRUE;
}

void logwin_hide(LogWin* log_win)
{
    window_hide(log_win->window);

    log_win->is_open = FALSE;
}

bool_t logwin_is_open(void)
{
    return i_logwin->is_open;
}
