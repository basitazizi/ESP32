# Waveshare ESP32-S3-Touch-LCD-1.85C-BOX V2 display test

Minimal PlatformIO / ESP-IDF firmware for the 360 x 360 ST77916 round LCD.
It displays red, green, and blue vertical bands with a white center cross,
sets the backlight to 80%, and logs a heartbeat every five seconds.

## Build

Run from this directory in PowerShell:

```powershell
& 'C:\Users\abdul\.platformio\penv\Scripts\platformio.exe' run
```

The project pins Espressif32 7.1.3, which uses ESP-IDF 6.1.0.
`sdkconfig.defaults` configures 16 MB flash (DIO, 80 MHz), octal PSRAM
(80 MHz), and the native USB Serial/JTAG console. The DevKitC board profile
is a base for the ESP32-S3 toolchain; the Waveshare memory configuration
overrides its defaults. The minimal factory application partition is 1 MB;
the remaining flash is unused at this milestone.

There are no external libraries. `src/idf_component.yml` only declares the
compatible ESP-IDF version. `dependencies.lock` confirms that IDF is the sole
managed dependency. Additional features should use ESP-IDF Component Manager
instead of adding a second copy through PlatformIO `lib_deps`.

## Hardware initialization

1. Create I2C0 with **SDA GPIO11 / SCL GPIO10**, matching the official V2
   `components/auido_borad/boards/include/ESP32_S3_AUDIO_Board.h` and
   `bsp_i2c_init()`.
2. Probe TCA9554 at `0x20`, add it at 400 kHz, and configure outputs as in
   Waveshare's `EXIO_Init()`. Reset the LCD through **EXIO2**: low for 10 ms,
   then high for 50 ms. Preserve the other output bits during reset.
3. Initialize SPI2 QSPI with **SCK 40, D0 46, D1 45, D2 42, D3 41, CS 21**.
   Keep Waveshare's 3 MHz panel-ID read and selection of its default or
   alternate initialization table. Release the temporary read IO before
   creating the 80 MHz display IO. Command width is 32 bits, parameter width
   is 8 bits, and pixels are RGB565.
4. Configure backlight PWM on **GPIO5** at 5 kHz / 13 bits. Keep it off while
   initializing and clearing the panel; turn it on after the clear completes.
5. Draw using one 11,520-byte internal DMA buffer (360 x 16 x 2). A binary
   semaphore signaled by `on_color_trans_done` prevents reuse before DMA
   completes, including the last transfer. RGB565 is sent MSB first.

All required hardware initialization and transfer errors propagate to
`ESP_ERROR_CHECK` in `app_main()`. A failed transaction does not produce a
successful initialization message. The LCD helper API is used by one task.

The official low-level ST77916 driver and both initialization tables are
retained. Its only functional modification is returning the result of
`tx_color()` from `draw_bitmap()`, so queue errors reach the caller.

## Flashing and verification

**Flashing requires the user's approval. It replaces the running Waveshare demo.**
No firmware has been uploaded as part of this repair. The firmware image and
partition table have been checked locally; physical operation is still pending.

Check the port again immediately before uploading:

```powershell
& 'C:\Users\abdul\.platformio\penv\Scripts\platformio.exe' device list
```

The last detected board was **COM5**, Espressif USB VID/PID **303A:1001**,
serial **28:84:85:55:2C:34**. The Bluetooth COM3/COM4 ports are unrelated.

After approval, upload the reviewed build and monitor it:

```powershell
& 'C:\Users\abdul\.platformio\penv\Scripts\platformio.exe' run --target upload --upload-port COM5
& 'C:\Users\abdul\.platformio\penv\Scripts\platformio.exe' device monitor --port COM5 --baud 115200
```

The upload writes the bootloader at `0x0`, partition table at `0x8000`, and
application at `0x10000`. It does not request a full-chip erase or eFuse changes.
The factory application partition spans `0x10000` through `0x10FFFF`.

Expected serial output includes:

```text
APP: Waveshare V2 display test; ESP-IDF ...
APP: PSRAM: 8388608 bytes
BOARD: V2 I2C ready: SDA=11 SCL=10
TCA9554: Expander ready at 0x20; EXIO2 controls LCD reset
ST77916: Panel ID: ...
APP: LCD initialized: 360x360 RGB565; backlight 80%
APP: LCD test screen ready: red/green/blue bands with a white center cross
APP: Display test alive; free internal heap: ... bytes
```

Verify the screen visually and check that the heartbeat continues without
resets or error messages. Serial transfer completion alone cannot prove the
panel is displaying the intended colors.

## Preserved sources

Inactive Waveshare audio, battery, touch, LVGL driver, and LVGL configuration
files are in `reference/waveshare/`. They are outside all component discovery
and application include paths, and will need integration work before reuse.
In particular, the display/expander APIs now return errors and accept an
explicit I2C bus; the old LVGL/touch callers must be adapted when reintroduced.

Original project files and the old generated build/dependency trees were
saved under `.local-backup/before-display-fix/` (ignored by Git).
The new build starts without those caches. VS Code include metadata was
regenerated through PlatformIO; do not edit its generated paths by hand.

Official sources:

- [Waveshare board documentation and V2 pin differences](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.85C)
- [Waveshare source repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.85C)

The working downloaded ESP-IDF example was also used for direct comparisons.
LVGL, touch, audio, Wi-Fi, and custom UI are later milestones.
