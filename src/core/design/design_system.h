#ifndef DESIGN_SYSTEM_H
#define DESIGN_SYSTEM_H

#include <QObject>

#include "color_tokens.h"
#include "spacing_tokens.h"
#include "responsive_tokens.h"
#include "font_tokens.h"

class DesignSystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ColorTokens* Colors READ colors CONSTANT)
    Q_PROPERTY(SpacingTokens* Spacing READ spacing CONSTANT)
    Q_PROPERTY(ResponsiveTokens* Responsive READ responsive CONSTANT)
    Q_PROPERTY(FontTokens* Fonts READ fonts CONSTANT)

public:
    explicit DesignSystem(QObject* parent = nullptr);

    ColorTokens* colors() const { return m_colors; }
    SpacingTokens* spacing() const { return m_spacing; }
    ResponsiveTokens* responsive() const { return m_responsive; }
    FontTokens* fonts() const { return m_fonts; }

    Q_INVOKABLE void save();
    Q_INVOKABLE void resetAll();

private:
    ColorTokens* m_colors;
    SpacingTokens* m_spacing;
    ResponsiveTokens* m_responsive;
    FontTokens* m_fonts;
};

#endif
