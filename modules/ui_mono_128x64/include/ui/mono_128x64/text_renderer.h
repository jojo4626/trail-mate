#pragma once

#include "ui/mono_128x64/font/mono_font.h"

#include <cstddef>

namespace ui::mono_128x64
{

class MonoDisplay;

class TextRenderer
{
  public:
    explicit TextRenderer(const MonoFont& font);

    int lineHeight() const;
    int measureTextWidth(const char* utf8) const;
    size_t clipTextToWidth(const char* utf8, int max_width) const;
    int ellipsisWidth() const;
    void drawText(MonoDisplay& display, int x, int y, const char* utf8, bool inverse = false) const;

  private:
    const MonoFont& font_;
};

} // namespace ui::mono_128x64
