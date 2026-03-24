/**
 * @file rtc_utils.h
 * @brief RTC helper functions (lightweight, no Wire include)
 */

#pragma once

#include <stdint.h>

bool board_adjust_rtc_by_offset_minutes(int offset_minutes);
