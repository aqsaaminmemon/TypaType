#pragma once
#include <QColor>
namespace typatype {
struct ThemeColors {
    QColor background,card,text,secondary,border,soft,primary,correct,error,errorSurface;
};
class ThemeManager {
public:
    void apply(bool dark);
    const ThemeColors& colors() const {return colors_;}
private:
    ThemeColors colors_;
};
}
