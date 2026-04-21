#include "ui/startup_ui_shell.h"

#include "lvgl.h"
#include "ui/app_runtime.h"
#include "ui/localization.h"
#include "ui/menu/menu_layout.h"
#include "ui/ui_boot.h"

namespace ui::startup_ui_shell
{
namespace
{

bool s_shell_initialized = false;

void present_boot_overlay_now()
{
    lv_timer_handler();
    lv_refr_now(nullptr);
}

bool lock_ui(const Hooks& hooks)
{
    return hooks.lock_ui ? hooks.lock_ui(hooks.lock_timeout_ms) : true;
}

void unlock_ui(const Hooks& hooks)
{
    if (hooks.unlock_ui)
    {
        hooks.unlock_ui();
    }
}

} // namespace

bool prepareBootUi(const Hooks& hooks, bool waking_from_sleep)
{
    if (waking_from_sleep)
    {
        ::ui::i18n::reload_language();
        return true;
    }
    if (!lock_ui(hooks))
    {
        return false;
    }
    ui::boot::show();
    ui::boot::set_log_line("Loading language packs...");
    present_boot_overlay_now();
    unlock_ui(hooks);
    ::ui::i18n::reload_language();
    return true;
}

bool initializeMenuSkeleton(const Hooks& hooks)
{
    if (s_shell_initialized)
    {
        return true;
    }
    if (!lock_ui(hooks))
    {
        return false;
    }

    lv_obj_t* active_screen = lv_screen_active();
    lv_obj_set_style_bg_color(active_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_radius(active_screen, 0, 0);

    ui::menu_layout::InitOptions options{};
    options.apps = hooks.apps;
    ui::menu_layout::init(options);

    s_shell_initialized = true;
    unlock_ui(hooks);
    return true;
}

bool finalizeStartup(const Hooks& hooks, bool waking_from_sleep)
{
    (void)waking_from_sleep;
    if (!lock_ui(hooks))
    {
        return false;
    }
    ui::boot::mark_ready();
    unlock_ui(hooks);
    return true;
}

} // namespace ui::startup_ui_shell
