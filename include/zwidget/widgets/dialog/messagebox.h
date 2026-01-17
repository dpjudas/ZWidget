#pragma once

#include "dialog.h"

class PushButton;
class TextLabel;

/* Generic MessageBox class */
class MessageBox : public Dialog
{
public:
    explicit MessageBox(Widget* owner, const std::string& windowTitle, const std::string& message, DialogButton buttons);
    static DialogButton Information(Widget* owner, const std::string& message, const std::string& windowTitle = "Information", DialogButton buttons = DialogButton::Ok);
    static DialogButton Question(Widget* owner, const std::string& message, const std::string& windowTitle = "Question", DialogButton buttons = DialogButton::Yes | DialogButton::No);
    static DialogButton Warning(Widget* owner, const std::string& message, const std::string& windowTitle = "Warning", DialogButton buttons = DialogButton::Ok);
    static DialogButton Error(Widget* owner, const std::string& message, const std::string& windowTitle = "Error", DialogButton buttons = DialogButton::Ok);

private:
    TextLabel* m_MessageLabel;
    std::vector<PushButton*> m_DialogButtons;
};