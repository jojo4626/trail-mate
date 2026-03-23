Import("env")
import os
import subprocess
import sys

print("[pio] pre: configuring build environment")

# Add include path for LVGL configuration file
# This ensures LVGL library can find lv_conf.h during compilation
project_dir = env.get("PROJECT_DIR")
ui_dir = os.path.join(project_dir, "platform", "esp", "arduino_common", "include")

# Add to both CPPFLAGS (for C/C++ compilation) and CCFLAGS (for all compilation)
env.Append(CPPFLAGS=["-I" + ui_dir])
env.Append(CCFLAGS=["-I" + ui_dir])

# Ensure LV_CONF_INCLUDE_SIMPLE is defined for library compilation
env.Append(CPPDEFINES=["LV_CONF_INCLUDE_SIMPLE"])

print(f"[pio] pre: Added LVGL config include path: {ui_dir}")

# On nordicnrf52/Windows builds, `${platformio.packages_dir}` inside `build_flags`
# can be rewritten into an invalid builder-local include path. Inject the
# TinyUSB framework paths here using the resolved absolute package location so
# framework libraries like `Wire` can include `<Adafruit_TinyUSB.h>`.
platform = env.PioPlatform()
framework_dir = platform.get_package_dir("framework-arduinoadafruitnrf52")
if framework_dir:
    framework_include_candidates = [
        os.path.join(framework_dir, "libraries", "Adafruit_TinyUSB_Arduino", "src"),
        os.path.join(framework_dir, "libraries", "Adafruit_TinyUSB_Arduino", "src", "arduino"),
        os.path.join(framework_dir, "libraries", "SPI"),
        os.path.join(framework_dir, "libraries", "Wire"),
        os.path.join(framework_dir, "libraries", "Bluefruit52Lib", "src"),
        os.path.join(framework_dir, "libraries", "Bluefruit52Lib", "src", "services"),
        os.path.join(framework_dir, "libraries", "Adafruit_nRFCrypto", "src"),
        os.path.join(framework_dir, "libraries", "Adafruit_LittleFS", "src"),
        os.path.join(framework_dir, "libraries", "InternalFileSytem", "src"),
    ]
    existing_cpppath = set(env.get("CPPPATH", []))
    include_paths = [path for path in framework_include_candidates if os.path.isdir(path)]
    missing_paths = [path for path in include_paths if path not in existing_cpppath]
    if missing_paths:
        env.Append(CPPPATH=missing_paths)
        print(f"[pio] pre: Added nRF52 framework include paths: {', '.join(missing_paths)}")

# Generate protobuf files if .proto files exist
proto_dir = os.path.join(project_dir, "lib", "meshtastic_protobufs")
generate_script = os.path.join(project_dir, "scripts", "generate_protobuf.py")

if os.path.exists(generate_script) and os.path.exists(proto_dir):
    print("[pio] pre: Generating protobuf files...")
    try:
        result = subprocess.run([sys.executable, generate_script], 
                              cwd=project_dir, capture_output=True, text=True)
        if result.returncode == 0:
            print("[pio] pre: Protobuf generation successful")
        else:
            print(f"[pio] pre: Protobuf generation warning: {result.stderr}")
    except Exception as e:
        print(f"[pio] pre: Protobuf generation error: {e}")

# Add generated protobuf include path
generated_dir = os.path.join(project_dir, "src", "chat", "infra", "meshtastic", "generated")
if os.path.exists(generated_dir):
    env.Append(CPPFLAGS=["-I" + generated_dir])
    env.Append(CCFLAGS=["-I" + generated_dir])
    print(f"[pio] pre: Added protobuf generated include path: {generated_dir}")

# Example hook: add a build flag to verify hook execution
env.Append(CPPDEFINES=["PIO_PRE_HOOK=1"])
