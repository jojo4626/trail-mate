#ifndef TRAIL_MATE_NRF_MONO_FUSION_PIXEL_8_ENABLED
#define TRAIL_MATE_NRF_MONO_FUSION_PIXEL_8_ENABLED 1
#endif

#if TRAIL_MATE_NRF_MONO_FUSION_PIXEL_8_ENABLED

#include "ui/fonts/fusion_pixel_8_font.h"

namespace ui::mono_128x64
{

extern const MonoFont kFusionPixel8Font;

} // namespace ui::mono_128x64

namespace platform::nrf52::ui::fonts
{

const ::ui::mono_128x64::MonoFont& fusion_pixel_8_font()
{
    // Adafruit-like access point: callers receive a const font object and the
    // renderer owns glyph lookup, measuring, clipping, and blitting.
    return ::ui::mono_128x64::kFusionPixel8Font;
}

} // namespace platform::nrf52::ui::fonts

#endif
