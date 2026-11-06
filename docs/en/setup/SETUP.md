# Setup

These instructions target Ubuntu/Debian-based Linux. Prepare a Raspberry Pi Pico or Pico W (RP2040) and a data-capable USB cable.

## Hardware preparation

For initial boot and USB CDC console access, **a Pico or Pico W and a data-capable USB cable** are sufficient. External circuits, including the LED matrix, are optional.

Before using GPIO or other peripherals, check the required components and wiring:

- [Hardware overview](../reference/hardware/HARDWARE.md): supported boards and safe starting assumptions
- [Pin assignments and 40-pin diagram](../reference/hardware/PINS.md): GPIO roles and physical header numbers
- [Wiring guide](../reference/hardware/WIRING.md): LED matrix, switches, PWM, I²C, UART, and ADC
- [Device guide](../reference/devices/DEVICES.md): operations on GPIO, ADC, PWM, and framebuffer nodes

**Before wiring:** GPIO uses 3.3 V logic. LEDs need current-limiting resistors and suitable driver circuitry. Disconnect USB power while wiring, and verify pin roles and polarity before powering the board. Treat `kernel/pins.h` as the authoritative pin configuration.

## 1. Install build tools

```sh
sudo apt update
sudo apt install git cmake ninja-build python3 unzip picocom \
    gcc-arm-none-eabi libnewlib-arm-none-eabi
sudo apt install clang lld llvm
```

ARM GCC builds the RP2040 firmware. Clang, LLD, and LLVM are used mainly for host-side PIC checks of RAM applications.

## 2. Get Pico SDK 2.1.1

```sh
mkdir -p ~/.pico-sdk/sdk
cd ~/.pico-sdk/sdk

git clone --branch 2.1.1 \
    https://github.com/raspberrypi/pico-sdk.git 2.1.1

cd 2.1.1
git submodule update --init --recursive
```

If you already have this checkout, update its submodules rather than cloning a second copy.

## 3. Extract PicoNIX

Run the following in the directory where you downloaded the archive. Replace the quoted placeholders with the actual archive name and extracted folder name before running the commands.

```sh
unzip "PicoNIX-<version>.zip"
cd "<extracted-directory>"
chmod +x ./tools/build_sdk.sh
```

If you are using Git, change to the root of the PicoNIX repository instead.

## 4. Build with MinSizeRel

Select the board with `-DPICO_BOARD=pico` or `-DPICO_BOARD=pico_w`. The following examples target the regular Pico.

To use the ARM GCC toolchain installed by the distribution packages:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

**If you have separately installed ARM GCC 14.2 at `~/.pico-sdk/toolchain/14_2_Rel1`**, select it explicitly as follows. The directory is *not* created by the SDK Git clone or the `apt install` commands above.

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

**For Pico W**, replace `-DPICO_BOARD=pico` with `-DPICO_BOARD=pico_w` in either command. For example:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Hardware testing has confirmed that **the current Pico W firmware also boots and runs on a regular Pico**. However, the regular Pico has no Wi-Fi hardware, so networking is unavailable. This observed compatibility is not a guarantee for future versions; use the board-specific options above for supported builds.

Use `MinSizeRel` for the firmware (kernel normally optimized with `-Os`). All user applications are compiled separately with `-Oz` and `-fPIC` using the ARM compiler selected by CMake. Clang/LLD are used for the independent host-side PIC regression tests. The build script also runs host-side audits and PIC checks. If switching toolchains, delete the corresponding `build-pico/` or `build-pico_w/` directory first to avoid stale CMake configuration.

Outputs are `build-pico/piconix.uf2` and `build-pico/piconix.elf` for Pico, or `build-pico_w/piconix.uf2` and `build-pico_w/piconix.elf` for Pico W.

### Kernel function tracing (ON by default at build time)

The standard build includes selected kernel function instrumentation (**ON by default**), but **event capture is OFF at boot**. `trace run hello` shows nested function calls; `trace run -s hello` records syscalls alone. Explicitly passing ON is optional:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DPICONIX_KERNEL_FUNCTION_TRACE=ON
```

Use `-DPICO_BOARD=pico_w` for Pico W. After building and flashing, try `trace run hello`. For a smaller build without function instrumentation, rebuild with `-DPICONIX_KERNEL_FUNCTION_TRACE=OFF` explicitly. **CMake may retain a previous ON setting in its cache; omitting the option does not necessarily turn it off.**

The 51 selected functions retain entry/exit instrumentation; other functions in instrumented source files use `NO_TRACE`. Instrumentation can still increase Flash usage, stack usage and execution time for those selected functions. See the [trace guide](../guides/TRACE.md) for details and limitations.

## 5. Flash the Pico or Pico W

1. Hold BOOTSEL while connecting the Pico or Pico W over USB.
2. Copy `build-pico/piconix.uf2` (Pico) or `build-pico_w/piconix.uf2` (Pico W) onto the `RPI-RP2` drive.
3. After the Pico restarts, locate its USB CDC device on Linux.

```sh
ls -l /dev/ttyACM*
picocom -b 115200 /dev/ttyACM0
```

`/dev/ttyACM0` is only an example; use the actual device name. On USB CDC, `115200` is a terminal setting, not the physical UART baud rate.

## 6. Smoke test

```text
piconix# help
piconix# ps
piconix# tty
piconix# cat /proc/uptime
piconix# blink &
piconix# ps
```

Use Ctrl+C to interrupt a foreground program. When using `kill`, obtain the PID from `ps` rather than assuming a fixed number.

## 7. Troubleshooting

| Symptom | Check |
|---|---|
| SDK not found | `PICO_SDK_PATH` and SDK submodules |
| ARM GCC not found | `arm-none-eabi-gcc --version` and the `PICO_TOOLCHAIN_PATH` directory |
| Clang/LLD not found | `clang --version` and `ld.lld --version` |
| RAM/link failure | `MinSizeRel`, stale build cache, and the 24 KiB application limit |
| No `/dev/ttyACM0` | `sudo dmesg -w` for USB enumeration, cable and firmware upload |
| Cannot open serial port | Device name, permissions, and other programs using the port |
| GPIO25 does not work | Confirm the Raspberry Pi Pico board variant |

For a separate UART1 shell, run `getty /dev/ttyS1 sh &`. See [UART and terminals](../reference/devices/UART.md) for serial configuration and Ctrl+C/Ctrl+D behavior, and [Internals](../reference/INTERNAL.md) for RAM layout.

## Pico W network access

For Pico W builds, Wi-Fi settings, Telnet/socat access and disconnect instructions, see [Pico W Wi-Fi, Telnet and raw TCP](../reference/network/NETWORK.md).


## Next steps

After connecting, follow the [Learning roadmap](../ROADMAP.md) to explore files, pipes, and tasks. Keep commands for the Linux build PC separate from commands typed at `piconix#`.

---

**Related links:** [日本語](../../ja/setup/SETUP.md) | [Usage guides](../guides/README.md)
