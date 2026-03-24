#ifndef TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED
#define TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED 0
#endif

#if TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED

#include "ui/fonts/fusion_pixel_10_font.h"

namespace ui::mono_128x64
{

extern const MonoFont kFusionPixel10Font;

} // namespace ui::mono_128x64

namespace platform::nrf52::ui::fonts
{

const ::ui::mono_128x64::MonoFont& fusion_pixel_10_font()
{
    return ::ui::mono_128x64::kFusionPixel10Font;
}

} // namespace platform::nrf52::ui::fonts

#endif
