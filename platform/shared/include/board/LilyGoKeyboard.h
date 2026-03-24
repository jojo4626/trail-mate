/**
 * @file      LilyGoKeyboard.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-01-04
 *
 */

#pragma once

#include <Arduino.h>

#if defined(ARDUINO_T_LORA_PAGER)
#include <Adafruit_TCA8418.h>

#define KB_NONE -1
#define KB_PRESSED 1
#define KB_RELEASED 0

typedef struct LilyGoKeyboardConfigure
{
    uint8_t kb_rows;
    uint8_t kb_cols;
    const char* current_keymap;
    const char* current_symbol_map;
    uint8_t symbol_key_value;
    uint8_t alt_key_value;
    uint8_t caps_key_value;
    uint8_t caps_b_key_value;
    uint8_t char_b_value;
    uint8_t backspace_value;
    bool has_symbol_key;
} LilyGoKeyboardConfigure_t;

class LilyGoKeyboard : public Adafruit_TCA8418
{
  public:
    using KeyboardReadCallback = void (*)(int state, char& c);
    using GpioEventCallback = void (*)(bool pressed, uint8_t gpio_idx);
    using BacklightCallback = void (*)(uint8_t level);
    using KeyboardRawCallback = void (*)(bool pressed, uint8_t raw);

    LilyGoKeyboard();
    ~LilyGoKeyboard();

    void setPins(int backlight);
    bool begin(const LilyGoKeyboardConfigure_t& config, TwoWire& w, uint8_t irq, uint8_t sda = SDA, uint8_t scl = SCL);
    void end();
    int getKey(char* c);
    void setBrightness(uint8_t level);
    uint8_t getBrightness();
    void setCallback(KeyboardReadCallback cb);
    void setGpioEventCallback(GpioEventCallback cb);
    void setBacklightChangeCallback(BacklightCallback cb);
    void setRawCallback(KeyboardRawCallback cb);
    void setRepeat(bool enable);

  private:
    int update(char* c);
    void printDebugInfo(bool pressed, uint8_t k, char keyVal);
    char handleSpaceAndNullChar(char keyVal, char& lastKeyVal, bool& pressed);
    char getKeyChar(uint8_t k);
    bool handleBrightnessAdjustment(uint8_t k, bool pressed);
    int handleSpecialKeys(uint8_t k, bool pressed, char* c);

    char lastKeyVal = '\n';
    int _backlight = -1;
    uint8_t _brightness;
    uint8_t _irq;
    bool symbol_key_pressed = false;
    bool cap_key_pressed = false;
    bool alt_key_pressed = false;
    bool alt_combo_used = false;
    bool repeat_function = true;
    bool lastState = false;
    KeyboardReadCallback cb = NULL;
    GpioEventCallback gpio_cb = NULL;
    BacklightCallback bl_cb = NULL;
    KeyboardRawCallback raw_cb = NULL;
    uint32_t lastPressedTime = 0;
    const LilyGoKeyboardConfigure_t* _config;
};
#endif
