/**
 * @file ui_boot.h
 * @brief Boot splash screen helpers.
 */

#ifndef UI_BOOT_H
#define UI_BOOT_H

#include "lvgl.h"

namespace ui::boot
{

// Show the boot splash overlay (background + logo fade-in).
void show();

// Update the single-line boot log shown in the lower-left corner.
void set_log_line(const char* text);

// Signal that background loading is complete.
// Splash will hide after the minimum display time has elapsed.
void mark_ready();

} // namespace ui::boot

#endif // UI_BOOT_H
