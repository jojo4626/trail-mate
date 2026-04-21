#include "platform/esp/arduino_common/memory_diag.h"

#include <Arduino.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "platform/esp/arduino_common/app_tasks.h"
#include "platform/esp/arduino_common/gps/gps_service.h"

namespace platform::esp::arduino_common::memory_diag
{
namespace
{

const char* safe_text(const char* value)
{
    return (value && value[0] != '\0') ? value : "<unnamed>";
}

unsigned long stack_high_water_bytes(TaskHandle_t handle)
{
    if (!handle)
    {
        return 0;
    }
    return static_cast<unsigned long>(uxTaskGetStackHighWaterMark(handle)) * sizeof(StackType_t);
}

void log_named_task(const char* label, TaskHandle_t handle)
{
    Serial.printf("[Mem][Task] name=%s active=%u stack_hw=%lu\n",
                  safe_text(label),
                  handle ? 1U : 0U,
                  stack_high_water_bytes(handle));
}

} // namespace

void logHeapSnapshot(const char* stage)
{
    Serial.printf("[Mem][Heap] stage=%s ram_free=%u ram_largest=%u psram_free=%u psram_largest=%u tasks=%u\n",
                  safe_text(stage),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                  static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)),
                  static_cast<unsigned>(uxTaskGetNumberOfTasks()));
}

void logTaskSnapshot(const char* stage)
{
    Serial.printf("[Mem][Tasks] stage=%s count=%u\n",
                  safe_text(stage),
                  static_cast<unsigned>(uxTaskGetNumberOfTasks()));
    log_named_task("gps_collect", gps::GpsService::getInstance().getTaskHandle());
    log_named_task("motion_mgr", gps::GpsService::getInstance().getMotionTaskHandle());
    log_named_task("radio_task", app::AppTasks::getRadioTaskHandle());
    log_named_task("mesh_task", app::AppTasks::getMeshTaskHandle());
}

} // namespace platform::esp::arduino_common::memory_diag
