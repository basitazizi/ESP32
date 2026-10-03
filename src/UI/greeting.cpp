#include "greeting.h"
#include "ST77916.h"
#include "esp_heap_caps.h"
#include <cstring>

namespace {
constexpr int kScale = 6;
constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;
constexpr int kLineHeight = kGlyphHeight * kScale;
constexpr int kLineGap = 18;
constexpr int kMaxLineWidth = (6 * (kGlyphWidth + 1) - 1) * kScale;

// Original 5x7 glyphs, one row per byte, most significant used bit on the left.
struct Glyph { char character; uint8_t rows[kGlyphHeight]; };
constexpr Glyph kGlyphs[] = {
    {'H', {17, 17, 17, 31, 17, 17, 17}},
    {'e', {0, 0, 14, 17, 31, 16, 14}},
    {'l', {12, 4, 4, 4, 4, 4, 14}},
    {'o', {0, 0, 14, 17, 17, 17, 14}},
    {'B', {30, 17, 17, 30, 17, 17, 30}},
    {'a', {0, 0, 14, 1, 15, 17, 15}},
    {'s', {0, 0, 15, 16, 14, 1, 30}},
    {'i', {4, 0, 12, 4, 4, 4, 14}},
    {'t', {4, 4, 31, 4, 4, 4, 3}},
    {'!', {4, 4, 4, 4, 4, 0, 4}},
};

esp_err_t draw_line(const char *text, int y, uint16_t *pixels)
{
    const int length = static_cast<int>(std::strlen(text));
    const int width = (length * (kGlyphWidth + 1) - 1) * kScale;
    std::memset(pixels, 0, width * kLineHeight * sizeof(*pixels));
    for (int index = 0; index < length; ++index) {
        for (const auto &glyph : kGlyphs) {
            if (glyph.character != text[index]) continue;
            for (int row = 0; row < kGlyphHeight; ++row) {
                for (int col = 0; col < kGlyphWidth; ++col) {
                    if (!(glyph.rows[row] & (1 << (kGlyphWidth - 1 - col)))) continue;
                    for (int dy = 0; dy < kScale; ++dy) {
                        for (int dx = 0; dx < kScale; ++dx) {
                            const int x = (index * (kGlyphWidth + 1) + col) * kScale + dx;
                            pixels[(row * kScale + dy) * width + x] = 0xFFFF;
                        }
                    }
                }
            }
            break;
        }
    }
    return LCD_DrawBitmap((EXAMPLE_LCD_WIDTH - width) / 2, y, width, kLineHeight, pixels);
}
} // namespace

esp_err_t UI_ShowGreeting()
{
    auto *pixels = static_cast<uint16_t *>(heap_caps_malloc(
        kMaxLineWidth * kLineHeight * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!pixels) return ESP_ERR_NO_MEM;

    const int top = (EXAMPLE_LCD_HEIGHT - (2 * kLineHeight + kLineGap)) / 2;
    esp_err_t result = LCD_FillColor(0x0000);
    if (result == ESP_OK) result = draw_line("Hello", top, pixels);
    if (result == ESP_OK) result = draw_line("Basit!", top + kLineHeight + kLineGap, pixels);
    heap_caps_free(pixels);
    return result;
}
