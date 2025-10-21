#pragma once

#include "zwidget/core/layout.h"

class VBoxLayout final : public Layout
{
public:
    VBoxLayout(Widget* parent = nullptr);

    void OnGeometryChanged() override;

    double GetPreferredWidth() override;
    double GetPreferredHeight() override;

    void SetGapHeight(double newGapHeight);

private:
    double m_gapHeight = 4.0;
};
