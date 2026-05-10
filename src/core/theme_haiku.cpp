#include "zwidget/core/theme.h"
#include <InterfaceDefs.h>

#ifdef __HAIKU__

static Colorf FromHaiku(color_which which)
{
    rgb_color c = ui_color(which);
    return Colorf(c.red / 255.0f, c.green / 255.0f, c.blue / 255.0f, c.alpha / 255.0f);
}

HaikuWidgetTheme::HaikuWidgetTheme() : SimpleTheme({
	FromHaiku(B_PANEL_BACKGROUND_COLOR),   // background
	FromHaiku(B_PANEL_TEXT_COLOR),         //
	FromHaiku(B_DOCUMENT_BACKGROUND_COLOR), // headers / inputs
	FromHaiku(B_DOCUMENT_TEXT_COLOR),       //
	FromHaiku(B_CONTROL_BACKGROUND_COLOR), // interactive elements
	FromHaiku(B_CONTROL_TEXT_COLOR),       //
	FromHaiku(B_CONTROL_HIGHLIGHT_COLOR),  // hover / highlight
	FromHaiku(B_CONTROL_TEXT_COLOR),       // (Haiku doesn't have a specific hover text color usually)
	FromHaiku(B_KEYBOARD_NAVIGATION_COLOR), // click (using keyboard nav color as a placeholder for active/active-hover)
	FromHaiku(B_CONTROL_TEXT_COLOR),       //
	FromHaiku(B_CONTROL_BORDER_COLOR),     // around elements
	FromHaiku(B_CONTROL_BORDER_COLOR)      // between elements
	})
{
}

#endif
