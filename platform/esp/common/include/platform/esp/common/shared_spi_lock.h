#pragma once

#include "freertos/FreeRTOS.h"

namespace platform::esp::common
{

// The shared SPI lock arbitrates ownership of the board-level SPI bus on
// devices where display, SD, radio, NFC, or other peripherals physically
// share one controller. It is not display-specific.
bool shared_spi_lock(TickType_t xTicksToWait = portMAX_DELAY);
void shared_spi_unlock();

class SharedSpiLockGuard
{
  public:
    explicit SharedSpiLockGuard(TickType_t wait_ticks = portMAX_DELAY)
        : locked_(shared_spi_lock(wait_ticks))
    {
    }

    ~SharedSpiLockGuard()
    {
        if (locked_)
        {
            shared_spi_unlock();
        }
    }

    bool locked() const { return locked_; }

  private:
    bool locked_ = false;
};

} // namespace platform::esp::common

// Legacy compatibility alias. New code should use shared_spi_lock /
// shared_spi_unlock or SharedSpiLockGuard so call sites describe the bus they
// are arbitrating rather than implying the lock belongs only to display code.
inline bool display_spi_lock(TickType_t xTicksToWait = portMAX_DELAY)
{
    return ::platform::esp::common::shared_spi_lock(xTicksToWait);
}

inline void display_spi_unlock()
{
    ::platform::esp::common::shared_spi_unlock();
}
