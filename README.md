# STM32 Development for Xilinx FPGAs

This code is a modificaction of the fantastic STM32 code created by Victor Trucco from https://gitlab.com/victor.trucco/Multicore/-/tree/master/System/STM32

To know about usage, look at [docs/USAGE.md](./docs/USAGE.md)

## To know before start

- Set the "FIRMWARE_TYPE", "LOG_LEVEL" and change "version" on `pins.h` file

- Atention: You have to uncomment line 62 ( #define DISABLE_LOGGING ) of `pins.h` file after debugging, to reduce binary size 
  But to avoid compilation errors, you have to update the file `ArduinoLog.h` from "C:\Users\Your_User\Documents\Arduino\libraries\ArduinoLog" 
  following the changes of this commit --> https://github.com/thijse/Arduino-Log/pull/25/commits/a7ffa91a4036826d6217288c79bba15af3b0d5b2

- The main file is `SPI.ino`. The execution starts on methods:
  - void setup( void )
  - void loop ( void )

## Prepare for development

### Arduino IDE

Download the latest Arduino IDE: https://www.arduino.cc/

### Install STM32 Board definitions

Go to `Files -> Preferences -> Additional Boards Manager URLs` and add the URL:
- http://dan.drown.org/stm32duino/package_STM32duino_index.json

Save the preferences.

- Go to `Tools -> Board -> Boards Manager`
- Filter `Type` for `Contributed`
- Search for `STM32F1xx/GD32F1xx boards`

Select the board version (the current working version is `2021.3.18`) and install.

#### Board configuration

Go to `Tools -> Board` and select the board `Generic STM32F103C Series`

```
Board: Generic STM32F103C Series
Variant: STM32F103C8 (20k RAM. 64 Flash)
Upload Method: Serial
CPU Speed (Mhz): 72Mhz (Normal)
Optimize: Smallest (Default)
```

###### ST-Link

If you are using **ST-Link** to upload the binary:
- Keep the upload method as `Serial`
- Select `Sketch -> Export compiled Binary`
- At command line, use **st-flash**:

```
st-flash erase && st-flash write SPI.ino.generic_stm32f103c.bin 0x8000000 && st-flash reset
```

### Libraries

Go to `Tools -> Manage Libraries`

### SDFat

- Search for `SDFat by Bill Greiman`
- Select the version 2.0.6 (last working version)

### ArduinoLog

- Search for `ArduinoLog by Thijs Elenbaas`
- Select the version 1.0.3 (current working)
