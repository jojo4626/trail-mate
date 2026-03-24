#pragma once

#include "ui/mono_128x64/font/mono_font.h"

namespace platform::nrf52::ui::fonts
{

// NRF mono OLED uses a single 8px Fusion Pixel asset for both ASCII and CJK.
// Keeping this font under platform/nrf52 makes the platform boundary explicit:
// ESP/LVGL fonts stay on the ESP side, and NRF never picks them up by accident.
const ::ui::mono_128x64::MonoFont& fusion_pixel_8_font();

} // namespace platform::nrf52::ui::fonts
