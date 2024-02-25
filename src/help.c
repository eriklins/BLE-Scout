/* Help window */

#include <nappgui.h>
#include "help.h"
#include "help_rtf.h"


struct _help_t
{
	Window* window;
    Stream* stream;
    TextView* text;

    bool_t is_open;
};

static Help* i_help = NULL;

/*---------------------------------------------------------------------------*/

static void i_OnClose(Help* help, Event* e)
{
    window_hide(help->window);

    help->is_open = FALSE;

    unref(e);
}

static Panel* i_panel(Help* help)
{
    Panel* panel = panel_create();

    Layout* layout = layout_create(1, 1);

    help->text = textview_create();
    layout_textview(layout, help->text, 0, 0);

    layout_hsize(layout, 0, 600);
    layout_vsize(layout, 0, 600);
    layout_margin(layout, 5);

    panel_layout(panel, layout);

    return panel;

    unref(help);
}

Help* help_create(void)
{
	Help* help = heap_new0(Help);

	i_help = help;

    Panel* panel = i_panel(help);

    help->window = window_create(ekWINDOW_STD | ekWINDOW_RESIZE);
    window_panel(help->window, panel);
    window_title(help->window, "BLE Scout - Help");
    window_OnClose(help->window, listener(help, i_OnClose, Help));
    
    help->stream = stm_from_block((byte_t*)help_txt, str_len_c(help_txt));
    textview_rtf(help->text, help->stream);

    return help;
}

void help_destroy(Help** help)
{
	heap_delete(help, Help);
}

/*---------------------------------------------------------------------------*/

void help_show(Help* help, const V2Df pos)
{
    window_origin(help->window, pos);
    window_show(help->window);

    help->is_open = TRUE;
}

void help_hide(Help* help)
{
    window_hide(help->window);

    help->is_open = FALSE;
}

bool_t help_is_open(void)
{
    return i_help->is_open;
}
