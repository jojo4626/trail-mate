#include "platform/esp/common/shared_spi_lock.h"

namespace platform::esp::common
{

bool shared_spi_lock(TickType_t xTicksToWait)
{
    (void)xTicksToWait;
    return true;
}

void shared_spi_unlock() {}

} // namespace platform::esp::common
