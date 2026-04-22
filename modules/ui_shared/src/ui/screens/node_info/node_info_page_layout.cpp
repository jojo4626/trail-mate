/**
 * @file node_info_page_layout.cpp
 * @brief Node info page layout helpers
 */

#include "ui/screens/node_info/node_info_page_layout.h"

#include "ui/page/page_profile.h"
#include "ui/widgets/top_bar.h"

namespace node_info
{
namespace ui
{
namespace layout
{

namespace
{

lv_coord_t resolve_parent_width(lv_obj_t* parent)
{
    if (parent)
    {
        const lv_coord_t width = lv_obj_get_width(parent);
        if (width > 0)
        {
            return width;
        }
    }

    const lv_coord_t width = lv_display_get_physical_horizontal_resolution(nullptr);
    return width > 0 ? width : 320;
}

lv_coord_t resolve_parent_height(lv_obj_t* parent)
{
    if (parent)
    {
        const lv_coord_t height = lv_obj_get_height(parent);
        if (height > 0)
        {
            return height;
        }
    }

    const lv_coord_t height = lv_display_get_physical_vertical_resolution(nullptr);
    return height > 0 ? height : 240;
}

lv_coord_t top_bar_height()
{
    const auto& profile = ::ui::page_profile::current();
    return profile.top_bar_height > 0 ? profile.top_bar_height
                                      : static_cast<lv_coord_t>(::ui::widgets::kTopBarHeight);
}

void make_plain(lv_obj_t* obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

} // namespace

lv_obj_t* create_root(lv_obj_t* parent)
{
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, resolve_parent_width(parent), resolve_parent_height(parent));
    lv_obj_set_pos(root, 0, 0);
    make_plain(root);
    return root;
}

lv_obj_t* create_header(lv_obj_t* root)
{
    const lv_coord_t root_w = resolve_parent_width(root);
    lv_obj_t* header = lv_obj_create(root);
    lv_obj_set_size(header, root_w, top_bar_height());
    lv_obj_set_pos(header, 0, 0);
    make_plain(header);
    return header;
}

lv_obj_t* create_content(lv_obj_t* root)
{
    const lv_coord_t root_w = resolve_parent_width(root);
    const lv_coord_t root_h = resolve_parent_height(root);
    const lv_coord_t header_h = top_bar_height();
    lv_obj_t* content = lv_obj_create(root);
    lv_obj_set_size(content, root_w, root_h - header_h);
    lv_obj_set_pos(content, 0, header_h);
    make_plain(content);
    return content;
}

} // namespace layout
} // namespace ui
} // namespace node_info
