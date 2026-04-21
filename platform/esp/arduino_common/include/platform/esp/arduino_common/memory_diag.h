#pragma once

namespace platform::esp::arduino_common::memory_diag
{

void logHeapSnapshot(const char* stage);
void logTaskSnapshot(const char* stage);

} // namespace platform::esp::arduino_common::memory_diag
