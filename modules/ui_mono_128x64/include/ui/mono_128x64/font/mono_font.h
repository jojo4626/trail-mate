#pragma once

#include <cstddef>
#include <cstdint>

namespace ui::mono_128x64
{

struct MonoGlyph
{
    constexpr MonoGlyph(uint32_t bitmap_offset_in = 0,
                        uint8_t width_in = 0,
                        uint8_t height_in = 0,
                        int8_t x_offset_in = 0,
                        int8_t y_offset_in = 0,
                        uint8_t advance_in = 0)
        : bitmap_offset(bitmap_offset_in),
          width(width_in),
          height(height_in),
          x_offset(x_offset_in),
          y_offset(y_offset_in),
          advance(advance_in)
    {
    }

    uint32_t bitmap_offset;
    uint8_t width;
    uint8_t height;
    int8_t x_offset;
    int8_t y_offset;
    uint8_t advance;
};

struct MonoFont
{
    enum Format : uint8_t
    {
        Legacy = 0,
        Compact16 = 1,
    };

    static constexpr MonoFont makeLegacy(const uint8_t* bitmap_in,
                                         const MonoGlyph* glyphs_in,
                                         const uint32_t* codepoints_in,
                                         uint16_t glyph_count_in,
                                         uint8_t line_height_in,
                                         int8_t baseline_in,
                                         uint8_t default_advance_in,
                                         uint16_t fallback_glyph_index_in)
    {
        return MonoFont(bitmap_in,
                        glyphs_in,
                        nullptr,
                        codepoints_in,
                        nullptr,
                        glyph_count_in,
                        line_height_in,
                        baseline_in,
                        default_advance_in,
                        fallback_glyph_index_in,
                        0,
                        0,
                        0,
                        Legacy);
    }

    static constexpr MonoFont makeCompact16(const uint8_t* bitmap_in,
                                            const uint16_t* codepoints16_in,
                                            const uint8_t* advances_in,
                                            uint16_t glyph_count_in,
                                            uint8_t line_height_in,
                                            int8_t baseline_in,
                                            uint8_t default_advance_in,
                                            uint16_t fallback_glyph_index_in,
                                            uint8_t glyph_width_in,
                                            uint8_t glyph_height_in,
                                            uint8_t fixed_advance_in)
    {
        return MonoFont(bitmap_in,
                        nullptr,
                        codepoints16_in,
                        nullptr,
                        advances_in,
                        glyph_count_in,
                        line_height_in,
                        baseline_in,
                        default_advance_in,
                        fallback_glyph_index_in,
                        glyph_width_in,
                        glyph_height_in,
                        fixed_advance_in,
                        Compact16);
    }

    const uint8_t* bitmap;
    const MonoGlyph* glyphs;
    const uint16_t* codepoints16;
    const uint32_t* codepoints32;
    const uint8_t* advances;
    uint16_t glyph_count;
    uint8_t line_height;
    int8_t baseline;
    uint8_t default_advance;
    uint16_t fallback_glyph_index;
    uint8_t glyph_width;
    uint8_t glyph_height;
    uint8_t fixed_advance;
    uint8_t format;

  private:
    constexpr MonoFont(const uint8_t* bitmap_in,
                       const MonoGlyph* glyphs_in,
                       const uint16_t* codepoints16_in,
                       const uint32_t* codepoints32_in,
                       const uint8_t* advances_in,
                       uint16_t glyph_count_in,
                       uint8_t line_height_in,
                       int8_t baseline_in,
                       uint8_t default_advance_in,
                       uint16_t fallback_glyph_index_in,
                       uint8_t glyph_width_in,
                       uint8_t glyph_height_in,
                       uint8_t fixed_advance_in,
                       uint8_t format_in)
        : bitmap(bitmap_in),
          glyphs(glyphs_in),
          codepoints16(codepoints16_in),
          codepoints32(codepoints32_in),
          advances(advances_in),
          glyph_count(glyph_count_in),
          line_height(line_height_in),
          baseline(baseline_in),
          default_advance(default_advance_in),
          fallback_glyph_index(fallback_glyph_index_in),
          glyph_width(glyph_width_in),
          glyph_height(glyph_height_in),
          fixed_advance(fixed_advance_in),
          format(format_in)
    {
    }
};

// Emergency fallback only. Production NRF builds are expected to inject a
// platform font through HostCallbacks::ui_font instead of relying on this stub.
const MonoFont& builtin_ui_font();

} // namespace ui::mono_128x64
