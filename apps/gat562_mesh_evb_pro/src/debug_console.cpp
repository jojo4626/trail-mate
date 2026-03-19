#include "apps/gat562_mesh_evb_pro/debug_console.h"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

namespace apps::gat562_mesh_evb_pro::debug_console
{
namespace
{

constexpr unsigned long kBaudRate = 115200UL;

void writeToUsbSerial(const char* text)
{
    if (!text)
    {
        return;
    }
    Serial.print(text);
}

void writeToJlinkSerial(const char* text)
{
    if (!text)
    {
        return;
    }
    Serial2.print(text);
}

} // namespace

void begin()
{
    Serial.begin(kBaudRate);
    Serial2.begin(kBaudRate);
    delay(80);
}

void print(const char* text)
{
    writeToUsbSerial(text);
    writeToJlinkSerial(text);
}

void println()
{
    Serial.println();
    Serial2.println();
}

void println(const char* text)
{
    writeToUsbSerial(text);
    writeToJlinkSerial(text);
    println();
}

void printf(const char* format, ...)
{
    if (!format)
    {
        return;
    }

    char buffer[192] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    print(buffer);
}

} // namespace apps::gat562_mesh_evb_pro::debug_console
