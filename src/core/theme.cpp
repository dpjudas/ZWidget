
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

/////////////////////////////////////////////////////////////////////////////

DarkWidgetTheme::DarkWidgetTheme()
{
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

	// raw colors
	auto rcTransparent = Colorf::transparent();
	auto rcBlack = Colorf::fromRgba8(0, 0, 0);
	auto rcDarkestGrey = Colorf::fromRgba8(33, 33, 33);
	auto rcDarkererGrey = Colorf::fromRgba8(38, 38, 38);
	auto rcDarkerGrey = Colorf::fromRgba8(45, 45, 45);
	auto rcDarkGrey = Colorf::fromRgba8(51, 51, 51);
	auto rcMidderesterGrey = Colorf::fromRgba8(58, 58, 58);
	auto rcMidGrey = Colorf::fromRgba8(68, 68, 68);
	auto rcMiddererGrey = Colorf::fromRgba8(78, 78, 78);
	auto rcMidderestGrey = Colorf::fromRgba8(88, 88, 88);
	auto rcMiddestGrey = Colorf::fromRgba8(99, 99, 99);
	auto rcMidderGrey = Colorf::fromRgba8(100, 100, 100);
	auto rcLightGrey = Colorf::fromRgba8(226, 223, 219);

	// colors
	auto cTransparent = rcTransparent;
	auto cTextColor = rcLightGrey;
	auto cHeaderTextcolor = rcLightGrey;
	auto cTextHoverColor = rcBlack;
	auto cWindowBackground = rcDarkGrey;
	auto cWindowBorder = rcDarkGrey;
	auto cHeaderBackground = rcDarkestGrey;
	auto cInputBackground = rcDarkererGrey;
	auto cListBackground = rcDarkererGrey;
	auto cMenuBackground = rcDarkererGrey;
	auto cButtonBackground = rcMidGrey;
	auto cHoverBackground = rcMiddererGrey;
	auto cDownBackground = rcMidderestGrey;
	auto cInputBorder = rcMidderGrey;
	auto cTextSelectionBackground = rcMidderGrey;
	auto cTextSelectionNoFocusBackground = rcMidGrey;
	auto cListSelectionBackground = rcMidderGrey;
	auto cScrollTrackColor = rcDarkestGrey;
	auto cScrollThumbColor = rcMidderesterGrey;
	auto cTabBackground = rcDarkererGrey;
	auto cTabActiveBackground = rcDarkGrey;
	auto cTabHoverBackground = rcDarkerGrey;
	auto cTabInactiveBorder = rcMidGrey;
	auto cTabActiveBorder = rcMidderGrey;
	auto cCheckboxCheckColor = rcLightGrey;
	auto cCheckboxCheckedBorder = rcMidderGrey;
	auto cCheckboxUncheckedBorder = rcMiddestGrey;
	auto cCheckboxInnerBorder = rcDarkGrey;

	widget->SetString("font-family", "NotoSans");
	widget->SetColor("color", cTextColor);
	widget->SetColor("window-background", cWindowBackground);
	widget->SetColor("window-border", cWindowBorder);
	widget->SetColor("window-caption-color", cHeaderBackground);
	widget->SetColor("window-caption-text-color", cHeaderTextcolor);

	pushbutton->SetDouble("noncontent-left", 10.0);
	pushbutton->SetDouble("noncontent-top", 5.0);
	pushbutton->SetDouble("noncontent-right", 10.0);
	pushbutton->SetDouble("noncontent-bottom", 5.0);
	pushbutton->SetColor("background-color", cButtonBackground);
	pushbutton->SetColor("border-left-color", cInputBorder);
	pushbutton->SetColor("border-top-color", cInputBorder);
	pushbutton->SetColor("border-right-color", cInputBorder);
	pushbutton->SetColor("border-bottom-color", cInputBorder);
	pushbutton->SetColor("hover", "background-color", cHoverBackground);
	pushbutton->SetColor("down", "background-color", cDownBackground);

	lineedit->SetDouble("noncontent-left", 5.0);
	lineedit->SetDouble("noncontent-top", 3.0);
	lineedit->SetDouble("noncontent-right", 5.0);
	lineedit->SetDouble("noncontent-bottom", 3.0);
	lineedit->SetColor("background-color", cInputBackground);
	lineedit->SetColor("border-left-color", cInputBorder);
	lineedit->SetColor("border-top-color", cInputBorder);
	lineedit->SetColor("border-right-color", cInputBorder);
	lineedit->SetColor("border-bottom-color", cInputBorder);
	lineedit->SetColor("selection-color", cTextSelectionBackground);
	lineedit->SetColor("no-focus-selection-color", cTextSelectionNoFocusBackground);

	textedit->SetDouble("noncontent-left", 8.0);
	textedit->SetDouble("noncontent-top", 8.0);
	textedit->SetDouble("noncontent-right", 8.0);
	textedit->SetDouble("noncontent-bottom", 8.0);
	textedit->SetColor("background-color", cInputBackground);
	textedit->SetColor("border-left-color", cInputBorder);
	textedit->SetColor("border-top-color", cInputBorder);
	textedit->SetColor("border-right-color", cInputBorder);
	textedit->SetColor("border-bottom-color", cInputBorder);

	listview->SetDouble("noncontent-left", 10.0);
	listview->SetDouble("noncontent-top", 10.0);
	listview->SetDouble("noncontent-right", 3.0);
	listview->SetDouble("noncontent-bottom", 10.0);
	listview->SetColor("background-color", cListBackground);
	listview->SetColor("border-left-color", cInputBorder);
	listview->SetColor("border-top-color", cInputBorder);
	listview->SetColor("border-right-color", cInputBorder);
	listview->SetColor("border-bottom-color", cInputBorder);
	listview->SetColor("selection-color", cListSelectionBackground);

	dropdown->SetDouble("noncontent-left", 5.0);
	dropdown->SetDouble("noncontent-top", 5.0);
	dropdown->SetDouble("noncontent-right", 5.0);
	dropdown->SetDouble("noncontent-bottom", 5.0);
	dropdown->SetColor("background-color", cListBackground);
	dropdown->SetColor("border-left-color", cInputBorder);
	dropdown->SetColor("border-top-color", cInputBorder);
	dropdown->SetColor("border-right-color", cInputBorder);
	dropdown->SetColor("border-bottom-color", cInputBorder);
	dropdown->SetColor("selection-color", cListSelectionBackground);
	dropdown->SetColor("arrow-color", cInputBorder);

	scrollbar->SetColor("track-color", cScrollTrackColor);
	scrollbar->SetColor("thumb-color", cScrollThumbColor);

	tabbar->SetDouble("spacer-left", 20.0);
	tabbar->SetDouble("spacer-right", 20.0);
	tabbar->SetColor("background-color", cHeaderBackground);

	tabbar_tab->SetDouble("noncontent-left", 15.0);
	tabbar_tab->SetDouble("noncontent-right", 15.0);
	tabbar_tab->SetDouble("noncontent-top", 1.0);
	tabbar_tab->SetDouble("noncontent-bottom", 1.0);
	tabbar_tab->SetColor("background-color", cTabBackground);
	tabbar_tab->SetColor("border-left-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-top-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-right-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-bottom-color", cTabActiveBorder);
	tabbar_tab->SetColor("hover", "background-color", cTabHoverBackground);
	tabbar_tab->SetColor("active", "background-color", cTabActiveBackground);
	tabbar_tab->SetColor("active", "border-left-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-top-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-right-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-bottom-color", cTransparent);

	tabbar_spacer->SetDouble("noncontent-bottom", 1.0);
	tabbar_spacer->SetColor("border-bottom-color", cTabActiveBorder);

	tabwidget_stack->SetDouble("noncontent-left", 20.0);
	tabwidget_stack->SetDouble("noncontent-top", 5.0);
	tabwidget_stack->SetDouble("noncontent-right", 20.0);
	tabwidget_stack->SetDouble("noncontent-bottom", 5.0);

	checkbox_label->SetColor("checked-outer-border-color", cCheckboxCheckedBorder);
	checkbox_label->SetColor("checked-inner-border-color", cCheckboxInnerBorder);
	checkbox_label->SetColor("checked-color", cCheckboxCheckColor);
	checkbox_label->SetColor("unchecked-outer-border-color", cCheckboxUncheckedBorder);
	checkbox_label->SetColor("unchecked-inner-border-color", cCheckboxInnerBorder);

	menubar->SetColor("background-color", cHeaderBackground);
	toolbar->SetColor("background-color", cHeaderBackground);
	statusbar->SetColor("background-color", cHeaderBackground);

	toolbarbutton->SetColor("hover", "background-color", cHoverBackground);
	toolbarbutton->SetColor("down", "background-color", cDownBackground);

	menubaritem->SetColor("color", cHeaderTextcolor);
	menubaritem->SetColor("hover", "background-color", cHoverBackground);
	menubaritem->SetColor("hover", "color", cTextHoverColor);
	menubaritem->SetColor("down", "background-color", cDownBackground);
	menubaritem->SetColor("down", "color", cTextHoverColor);

	menu->SetDouble("noncontent-left", 5.0);
	menu->SetDouble("noncontent-top", 5.0);
	menu->SetDouble("noncontent-right", 5.0);
	menu->SetDouble("noncontent-bottom", 5.0);
	menu->SetColor("background-color", cMenuBackground);
	menu->SetColor("border-left-color", cInputBorder);
	menu->SetColor("border-top-color", cInputBorder);
	menu->SetColor("border-right-color", cInputBorder);
	menu->SetColor("border-bottom-color", cInputBorder);

	menuitem->SetColor("hover", "background-color", cHoverBackground);
	menuitem->SetColor("down", "background-color", cDownBackground);
}

/////////////////////////////////////////////////////////////////////////////

LightWidgetTheme::LightWidgetTheme()
{
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

	// raw colors
	auto rcTransparent = Colorf::transparent();
	auto rcBlack = Colorf::fromRgba8(0, 0, 0);
	auto rcDarkGrey = Colorf::fromRgba8(70, 70, 70);
	auto rcDarkerGrey = Colorf::fromRgba8(50, 50, 50);
	auto rcMidGrey = Colorf::fromRgba8(100, 100, 100);
	auto rcMidderGrey = Colorf::fromRgba8(190, 190, 190);
	auto rcMiddererGrey = Colorf::fromRgba8(155, 155, 155);
	auto rcMiddestGrey = Colorf::fromRgba8(180, 180, 180);
	auto rcMiddesterGrey = Colorf::fromRgba8(156, 156, 156);
	auto rcLightGrey = Colorf::fromRgba8(226, 223, 219);
	auto rcLighterGrey = Colorf::fromRgba8(240, 240, 240);
	auto rcLightererGrey = Colorf::fromRgba8(230, 230, 230);
	auto rcLightestGrey = Colorf::fromRgba8(210, 210, 210);
	auto rcLightesterGrey = Colorf::fromRgba8(200, 200, 200);
	auto rcLightestererGrey = Colorf::fromRgba8(210, 210, 255);
	auto rcLightesterestGrey = Colorf::fromRgba8(240, 240, 255);
	auto rcLightesteresterGrey = Colorf::fromRgba8(210, 210, 220);
	auto rcWhite = Colorf::fromRgba8(255, 255, 255);
	auto rcWhite2 = Colorf::fromRgba8(220, 220, 220);

	// colors
	auto cTransparent = rcTransparent;
	auto cTextColor = rcBlack;
	auto cHeaderTextcolor = rcLightGrey;
	auto cTextHoverColor = rcBlack;
	auto cWindowBackground = rcLighterGrey;
	auto cWindowBorder = rcMidGrey;
	auto cHeaderBackground = rcDarkGrey;
	auto cInputBackground = rcWhite;
	auto cListBackground = rcLightererGrey;
	auto cMenuBackground = rcWhite;
	auto cButtonBackground = rcLightestGrey;
	auto cHoverBackground = rcLightesterGrey;
	auto cDownBackground = rcMidderGrey;
	auto cInputBorder = rcMiddererGrey;
	auto cTextSelectionBackground = rcLightestererGrey;
	auto cTextSelectionNoFocusBackground = rcLightesterestGrey;
	auto cListSelectionBackground = rcLightesterGrey;
	auto cScrollTrackColor = rcLightesteresterGrey;
	auto cScrollThumbColor = rcMiddestGrey;
	auto cTabBackground = rcWhite2;
	auto cTabActiveBackground = rcLighterGrey;
	auto cTabHoverBackground = rcLightestGrey;
	auto cTabInactiveBorder = rcLightesterGrey;
	auto cTabActiveBorder = rcMiddererGrey;
	auto cCheckboxCheckColor = rcDarkerGrey;
	auto cCheckboxCheckedBorder = rcMiddererGrey;
	auto cCheckboxUncheckedBorder = rcMiddesterGrey;
	auto cCheckboxInnerBorder = rcLightesterGrey;

	widget->SetString("font-family", "NotoSans");
	widget->SetColor("color", cTextColor);
	widget->SetColor("window-background", cWindowBackground);
	widget->SetColor("window-border", cWindowBorder);
	widget->SetColor("window-caption-color", cHeaderBackground);
	widget->SetColor("window-caption-text-color", cHeaderTextcolor);

	pushbutton->SetDouble("noncontent-left", 10.0);
	pushbutton->SetDouble("noncontent-top", 5.0);
	pushbutton->SetDouble("noncontent-right", 10.0);
	pushbutton->SetDouble("noncontent-bottom", 5.0);
	pushbutton->SetColor("background-color", cButtonBackground);
	pushbutton->SetColor("border-left-color", cInputBorder);
	pushbutton->SetColor("border-top-color", cInputBorder);
	pushbutton->SetColor("border-right-color", cInputBorder);
	pushbutton->SetColor("border-bottom-color", cInputBorder);
	pushbutton->SetColor("hover", "background-color", cHoverBackground);
	pushbutton->SetColor("down", "background-color", cDownBackground);

	lineedit->SetDouble("noncontent-left", 5.0);
	lineedit->SetDouble("noncontent-top", 3.0);
	lineedit->SetDouble("noncontent-right", 5.0);
	lineedit->SetDouble("noncontent-bottom", 3.0);
	lineedit->SetColor("background-color", cInputBackground);
	lineedit->SetColor("border-left-color", cInputBorder);
	lineedit->SetColor("border-top-color", cInputBorder);
	lineedit->SetColor("border-right-color", cInputBorder);
	lineedit->SetColor("border-bottom-color", cInputBorder);
	lineedit->SetColor("selection-color", cTextSelectionBackground);
	lineedit->SetColor("no-focus-selection-color", cTextSelectionNoFocusBackground);

	textedit->SetDouble("noncontent-left", 8.0);
	textedit->SetDouble("noncontent-top", 8.0);
	textedit->SetDouble("noncontent-right", 8.0);
	textedit->SetDouble("noncontent-bottom", 8.0);
	textedit->SetColor("background-color", cInputBackground);
	textedit->SetColor("border-left-color", cInputBorder);
	textedit->SetColor("border-top-color", cInputBorder);
	textedit->SetColor("border-right-color", cInputBorder);
	textedit->SetColor("border-bottom-color", cInputBorder);

	listview->SetDouble("noncontent-left", 10.0);
	listview->SetDouble("noncontent-top", 10.0);
	listview->SetDouble("noncontent-right", 3.0);
	listview->SetDouble("noncontent-bottom", 10.0);
	listview->SetColor("background-color", cListBackground);
	listview->SetColor("border-left-color", cInputBorder);
	listview->SetColor("border-top-color", cInputBorder);
	listview->SetColor("border-right-color", cInputBorder);
	listview->SetColor("border-bottom-color", cInputBorder);
	listview->SetColor("selection-color", cListSelectionBackground);

	dropdown->SetDouble("noncontent-left", 5.0);
	dropdown->SetDouble("noncontent-top", 5.0);
	dropdown->SetDouble("noncontent-right", 5.0);
	dropdown->SetDouble("noncontent-bottom", 5.0);
	dropdown->SetColor("background-color", cListBackground);
	dropdown->SetColor("border-left-color", cInputBorder);
	dropdown->SetColor("border-top-color", cInputBorder);
	dropdown->SetColor("border-right-color", cInputBorder);
	dropdown->SetColor("border-bottom-color", cInputBorder);
	dropdown->SetColor("selection-color", cListSelectionBackground);
	dropdown->SetColor("arrow-color", cInputBorder);

	scrollbar->SetColor("track-color", cScrollTrackColor);
	scrollbar->SetColor("thumb-color", cScrollThumbColor);

	tabbar->SetDouble("spacer-left", 20.0);
	tabbar->SetDouble("spacer-right", 20.0);
	tabbar->SetColor("background-color", cHeaderBackground);

	tabbar_tab->SetDouble("noncontent-left", 15.0);
	tabbar_tab->SetDouble("noncontent-right", 15.0);
	tabbar_tab->SetDouble("noncontent-top", 1.0);
	tabbar_tab->SetDouble("noncontent-bottom", 1.0);
	tabbar_tab->SetColor("background-color", cTabBackground);
	tabbar_tab->SetColor("border-left-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-top-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-right-color", cTabInactiveBorder);
	tabbar_tab->SetColor("border-bottom-color", cTabActiveBorder);
	tabbar_tab->SetColor("hover", "background-color", cTabHoverBackground);
	tabbar_tab->SetColor("active", "background-color", cTabActiveBackground);
	tabbar_tab->SetColor("active", "border-left-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-top-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-right-color", cTabActiveBorder);
	tabbar_tab->SetColor("active", "border-bottom-color", cTransparent);

	tabbar_spacer->SetDouble("noncontent-bottom", 1.0);
	tabbar_spacer->SetColor("border-bottom-color", cTabActiveBorder);

	tabwidget_stack->SetDouble("noncontent-left", 20.0);
	tabwidget_stack->SetDouble("noncontent-top", 5.0);
	tabwidget_stack->SetDouble("noncontent-right", 20.0);
	tabwidget_stack->SetDouble("noncontent-bottom", 5.0);

	checkbox_label->SetColor("checked-outer-border-color", cCheckboxCheckedBorder);
	checkbox_label->SetColor("checked-inner-border-color", cCheckboxInnerBorder);
	checkbox_label->SetColor("checked-color", cCheckboxCheckColor);
	checkbox_label->SetColor("unchecked-outer-border-color", cCheckboxUncheckedBorder);
	checkbox_label->SetColor("unchecked-inner-border-color", cCheckboxInnerBorder);

	menubar->SetColor("background-color", cHeaderBackground);
	toolbar->SetColor("background-color", cHeaderBackground);
	statusbar->SetColor("background-color", cHeaderBackground);

	toolbarbutton->SetColor("hover", "background-color", cHoverBackground);
	toolbarbutton->SetColor("down", "background-color", cDownBackground);

	menubaritem->SetColor("color", cHeaderTextcolor);
	menubaritem->SetColor("hover", "background-color", cHoverBackground);
	menubaritem->SetColor("hover", "color", cTextHoverColor);
	menubaritem->SetColor("down", "background-color", cDownBackground);
	menubaritem->SetColor("down", "color", cTextHoverColor);

	menu->SetDouble("noncontent-left", 5.0);
	menu->SetDouble("noncontent-top", 5.0);
	menu->SetDouble("noncontent-right", 5.0);
	menu->SetDouble("noncontent-bottom", 5.0);
	menu->SetColor("background-color", cMenuBackground);
	menu->SetColor("border-left-color", cInputBorder);
	menu->SetColor("border-top-color", cInputBorder);
	menu->SetColor("border-right-color", cInputBorder);
	menu->SetColor("border-bottom-color", cInputBorder);

	menuitem->SetColor("hover", "background-color", cHoverBackground);
	menuitem->SetColor("down", "background-color", cDownBackground);
}
