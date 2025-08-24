
#include "core/theme.h"
#include "core/widget.h"
#include "core/canvas.h"

void WidgetStyle::SetBool(const std::string& state, const std::string& propertyName, bool value)
{
	StyleProperties[state][propertyName] = value;
}

void WidgetStyle::SetInt(const std::string& state, const std::string& propertyName, int value)
{
	StyleProperties[state][propertyName] = value;
}

void WidgetStyle::SetDouble(const std::string& state, const std::string& propertyName, double value)
{
	StyleProperties[state][propertyName] = value;
}

void WidgetStyle::SetString(const std::string& state, const std::string& propertyName, const std::string& value)
{
	StyleProperties[state][propertyName] = value;
}

void WidgetStyle::SetColor(const std::string& state, const std::string& propertyName, const Colorf& value)
{
	StyleProperties[state][propertyName] = value;
}

const WidgetStyle::PropertyVariant* WidgetStyle::FindProperty(const std::string& state, const std::string& propertyName) const
{
	const WidgetStyle* style = this;
	do
	{
		// Look for property in the specific state
		auto stateIt = style->StyleProperties.find(state);
		if (stateIt != style->StyleProperties.end())
		{
			auto it = stateIt->second.find(propertyName);
			if (it != stateIt->second.end())
				return &it->second;
		}

		// Fall back to the widget main style
		if (state != std::string())
		{
			stateIt = style->StyleProperties.find(std::string());
			if (stateIt != style->StyleProperties.end())
			{
				auto it = stateIt->second.find(propertyName);
				if (it != stateIt->second.end())
					return &it->second;
			}
		}

		style = style->ParentStyle;
	} while (style);
	return nullptr;
}

bool WidgetStyle::GetBool(const std::string& state, const std::string& propertyName) const
{
	const PropertyVariant* prop = FindProperty(state, propertyName);
	return prop ? std::get<bool>(*prop) : false;
}

int WidgetStyle::GetInt(const std::string& state, const std::string& propertyName) const
{
	const PropertyVariant* prop = FindProperty(state, propertyName);
	return prop ? std::get<int>(*prop) : 0;
}

double WidgetStyle::GetDouble(const std::string& state, const std::string& propertyName) const
{
	const PropertyVariant* prop = FindProperty(state, propertyName);
	return prop ? std::get<double>(*prop) : 0.0;
}

std::string WidgetStyle::GetString(const std::string& state, const std::string& propertyName) const
{
	const PropertyVariant* prop = FindProperty(state, propertyName);
	return prop ? std::get<std::string>(*prop) : std::string();
}

Colorf WidgetStyle::GetColor(const std::string& state, const std::string& propertyName) const
{
	const PropertyVariant* prop = FindProperty(state, propertyName);
	return prop ? std::get<Colorf>(*prop) : Colorf::transparent();
}

/////////////////////////////////////////////////////////////////////////////

void BasicWidgetStyle::Paint(Widget* widget, Canvas* canvas, Size size)
{
	Colorf bgcolor = widget->GetStyleColor("background-color");
	if (bgcolor.a > 0.0f)
		canvas->fillRect(Rect::xywh(0.0, 0.0, size.width, size.height), bgcolor);

	Colorf borderleft = widget->GetStyleColor("border-left-color");
	Colorf bordertop = widget->GetStyleColor("border-top-color");
	Colorf borderright = widget->GetStyleColor("border-right-color");
	Colorf borderbottom = widget->GetStyleColor("border-bottom-color");

	double borderwidth = widget->GridFitSize(1.0);

	if (bordertop.a > 0.0f)
		canvas->fillRect(Rect::xywh(0.0, 0.0, size.width, borderwidth), bordertop);
	if (borderbottom.a > 0.0f)
		canvas->fillRect(Rect::xywh(0.0, size.height - borderwidth, size.width, borderwidth), borderbottom);
	if (borderleft.a > 0.0f)
		canvas->fillRect(Rect::xywh(0.0, 0.0, borderwidth, size.height), borderleft);
	if (borderright.a > 0.0f)
		canvas->fillRect(Rect::xywh(size.width - borderwidth, 0.0, borderwidth, size.height), borderright);
}

/////////////////////////////////////////////////////////////////////////////

static std::unique_ptr<WidgetTheme> CurrentTheme;

WidgetStyle* WidgetTheme::RegisterStyle(std::unique_ptr<WidgetStyle> widgetStyle, const std::string& widgetClass)
{
	auto& style = Styles[widgetClass];
	style = std::move(widgetStyle);
	return style.get();
}

WidgetStyle* WidgetTheme::GetStyle(const std::string& widgetClass)
{
	auto it = Styles.find(widgetClass);
	return it != Styles.end() ? it->second.get() : nullptr;
}

void WidgetTheme::SetTheme(std::unique_ptr<WidgetTheme> theme)
{
	CurrentTheme = std::move(theme);
}

WidgetTheme* WidgetTheme::GetTheme()
{
	return CurrentTheme.get();
}

WidgetTheme::WidgetTheme(const struct SimpleTheme &theme)
{
	auto bg0    = theme.bg0;    // Deepest background headers/inputs
	auto bg1    = theme.bg1;    // Main background
	auto base0  = theme.base0;  // Interactive elements
	auto base1  = theme.base1;  // Hover states
	auto base2  = theme.base2;  // Borders
	auto base3  = theme.base3;  // Main text
	auto accent = theme.accent; // Highlight
	auto none   = Colorf::transparent();

	auto widget = RegisterStyle(std::make_unique<BasicWidgetStyle>(), "widget");
	/*auto textlabel =*/ RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "textlabel");
	auto pushbutton = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "pushbutton");
	auto lineedit = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "lineedit");
	auto textedit = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "textedit");
	auto listview = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "listview");
	auto dropdown = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "dropdown");
	auto scrollbar = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "scrollbar");
	auto tabbar = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "tabbar");
	auto tabbar_tab = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "tabbar-tab");
	auto tabbar_spacer = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "tabbar-spacer");
	auto tabwidget_stack = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "tabwidget-stack");
	auto checkbox_label = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "checkbox-label");
	auto menubar = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "menubar");
	auto menubaritem = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "menubaritem");
	auto menu = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "menu");
	auto menuitem = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "menuitem");
	auto toolbar = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "toolbar");
	auto toolbarbutton = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "toolbarbutton");
	auto statusbar = RegisterStyle(std::make_unique<BasicWidgetStyle>(widget), "statusbar");

	widget->SetString("font-family", "NotoSans");
	widget->SetColor("color", base3);
	widget->SetColor("window-background", bg1);
	widget->SetColor("window-border", bg1);
	widget->SetColor("window-caption-color", bg0);
	widget->SetColor("window-caption-text-color", base3);

	pushbutton->SetDouble("noncontent-left", 10.0);
	pushbutton->SetDouble("noncontent-top", 5.0);
	pushbutton->SetDouble("noncontent-right", 10.0);
	pushbutton->SetDouble("noncontent-bottom", 5.0);
	pushbutton->SetColor("background-color", base0);
	pushbutton->SetColor("border-left-color", base2);
	pushbutton->SetColor("border-top-color", base2);
	pushbutton->SetColor("border-right-color", base2);
	pushbutton->SetColor("border-bottom-color", base2);
	pushbutton->SetColor("hover", "background-color", base1);
	pushbutton->SetColor("down", "background-color", base1);

	lineedit->SetDouble("noncontent-left", 5.0);
	lineedit->SetDouble("noncontent-top", 3.0);
	lineedit->SetDouble("noncontent-right", 5.0);
	lineedit->SetDouble("noncontent-bottom", 3.0);
	lineedit->SetColor("background-color", bg0);
	lineedit->SetColor("border-left-color", base2);
	lineedit->SetColor("border-top-color", base2);
	lineedit->SetColor("border-right-color", base2);
	lineedit->SetColor("border-bottom-color", base2);
	lineedit->SetColor("selection-color", accent);
	lineedit->SetColor("no-focus-selection-color", accent);

	textedit->SetDouble("noncontent-left", 8.0);
	textedit->SetDouble("noncontent-top", 8.0);
	textedit->SetDouble("noncontent-right", 8.0);
	textedit->SetDouble("noncontent-bottom", 8.0);
	textedit->SetColor("background-color", bg0);
	textedit->SetColor("border-left-color", base2);
	textedit->SetColor("border-top-color", base2);
	textedit->SetColor("border-right-color", base2);
	textedit->SetColor("border-bottom-color", base2);

	listview->SetDouble("noncontent-left", 10.0);
	listview->SetDouble("noncontent-top", 10.0);
	listview->SetDouble("noncontent-right", 3.0);
	listview->SetDouble("noncontent-bottom", 10.0);
	listview->SetColor("background-color", bg0);
	listview->SetColor("border-left-color", base2);
	listview->SetColor("border-top-color", base2);
	listview->SetColor("border-right-color", base2);
	listview->SetColor("border-bottom-color", base2);
	listview->SetColor("selection-color", accent);

	dropdown->SetDouble("noncontent-left", 5.0);
	dropdown->SetDouble("noncontent-top", 5.0);
	dropdown->SetDouble("noncontent-right", 5.0);
	dropdown->SetDouble("noncontent-bottom", 5.0);
	dropdown->SetColor("background-color", bg0);
	dropdown->SetColor("border-left-color", base2);
	dropdown->SetColor("border-top-color", base2);
	dropdown->SetColor("border-right-color", base2);
	dropdown->SetColor("border-bottom-color", base2);
	dropdown->SetColor("arrow-color", base2);

	scrollbar->SetColor("track-color", bg0);
	scrollbar->SetColor("thumb-color", base0);

	tabbar->SetDouble("spacer-left", 20.0);
	tabbar->SetDouble("spacer-right", 20.0);
	tabbar->SetColor("background-color", bg0);

	tabbar_tab->SetDouble("noncontent-left", 15.0);
	tabbar_tab->SetDouble("noncontent-right", 15.0);
	tabbar_tab->SetDouble("noncontent-top", 1.0);
	tabbar_tab->SetDouble("noncontent-bottom", 1.0);
	tabbar_tab->SetColor("background-color", bg1);
	tabbar_tab->SetColor("border-left-color", base0);
	tabbar_tab->SetColor("border-top-color", base0);
	tabbar_tab->SetColor("border-right-color", base0);
	tabbar_tab->SetColor("border-bottom-color", base2);
	tabbar_tab->SetColor("hover", "background-color", base0);
	tabbar_tab->SetColor("active", "background-color", bg1);
	tabbar_tab->SetColor("active", "border-left-color", base2);
	tabbar_tab->SetColor("active", "border-top-color", base2);
	tabbar_tab->SetColor("active", "border-right-color", base2);
	tabbar_tab->SetColor("active", "border-bottom-color", none);

	tabbar_spacer->SetDouble("noncontent-bottom", 1.0);
	tabbar_spacer->SetColor("border-bottom-color", base2);

	tabwidget_stack->SetDouble("noncontent-left", 20.0);
	tabwidget_stack->SetDouble("noncontent-top", 5.0);
	tabwidget_stack->SetDouble("noncontent-right", 20.0);
	tabwidget_stack->SetDouble("noncontent-bottom", 5.0);

	checkbox_label->SetColor("checked-outer-border-color", base2);
	checkbox_label->SetColor("checked-inner-border-color", bg1);
	checkbox_label->SetColor("checked-color", base3);
	checkbox_label->SetColor("unchecked-outer-border-color", base2);
	checkbox_label->SetColor("unchecked-inner-border-color", bg1);

	menubar->SetColor("background-color", bg0);
	toolbar->SetColor("background-color", bg0);
	statusbar->SetColor("background-color", bg0);

	toolbarbutton->SetColor("hover", "background-color", base1);
	toolbarbutton->SetColor("down", "background-color", base1);

	menubaritem->SetColor("color", base3);
	menubaritem->SetColor("hover", "background-color", base1);
	menubaritem->SetColor("hover", "color", base3);
	menubaritem->SetColor("down", "background-color", base1);
	menubaritem->SetColor("down", "color", base3);

	menu->SetDouble("noncontent-left", 5.0);
	menu->SetDouble("noncontent-top", 5.0);
	menu->SetDouble("noncontent-right", 5.0);
	menu->SetDouble("noncontent-bottom", 5.0);
	menu->SetColor("background-color", bg1);
	menu->SetColor("border-left-color", base2);
	menu->SetColor("border-top-color", base2);
	menu->SetColor("border-right-color", base2);
	menu->SetColor("border-bottom-color", base2);

	menuitem->SetColor("hover", "background-color", base1);
	menuitem->SetColor("down", "background-color", base1);
}

/////////////////////////////////////////////////////////////////////////////

DarkWidgetTheme::DarkWidgetTheme(): WidgetTheme({
	Colorf::fromRgb(0x212121), // Deepest background headers/inputs
	Colorf::fromRgb(0x2A2A2A), // Main background
	Colorf::fromRgb(0x444444), // Interactive elements
	Colorf::fromRgb(0x555555), // Hover states
	Colorf::fromRgb(0x646464), // Borders
	Colorf::fromRgb(0xE2DFDB), // Main text
	Colorf::fromRgb(0xC83C00)  // Highlight
}) {};

/////////////////////////////////////////////////////////////////////////////

LightWidgetTheme::LightWidgetTheme(): WidgetTheme({
	Colorf::fromRgb(0xFAFAFA), // Deepest background headers/inputs
	Colorf::fromRgb(0xF0F0F0), // Main background
	Colorf::fromRgb(0xC8C8C8), // Interactive elements
	Colorf::fromRgb(0xB9B9B9), // Hover states
	Colorf::fromRgb(0xA0A0A0), // Borders
	Colorf::fromRgb(0x191919), // Main text
	Colorf::fromRgb(0xD2D2FF)  // Highlight
}) {};
