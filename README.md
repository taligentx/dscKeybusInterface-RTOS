# DSC Keybus Interface-RTOS
This library directly interfaces esp8266 microcontrollers to [DSC PowerSeries](http://www.dsc.com/dsc-security-products/g/PowerSeries/4) security systems for integration with home automation, notifications on alarm events, and usage as a virtual keypad.  This enables existing DSC security system installations to retain the features and reliability of a hardwired system while integrating with modern devices and software for under $5USD in components.

This repository is an [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) port of the [Arduino dscKeybusInterface library](https://github.com/taligentx/dscKeybusInterface), primarily to support direct native integration with [Apple HomeKit & Siri](https://www.apple.com/ios/home/) as a standalone accessory using [esp-homekit](https://github.com/maximkulkin/esp-homekit) (without using an intermediary like [Homebridge](https://github.com/nfarina/homebridge), [HAP-NodeJS](https://github.com/KhaosT/HAP-NodeJS), etc.)

* [Apple Home & Siri](https://www.apple.com/ios/home/):  
  ![HomeKit](https://user-images.githubusercontent.com/12835671/39588413-5a99099a-4ec1-11e8-9a2e-e332fa2d6379.jpg)

## Why?
**I Had**: _A DSC security system not being monitored by a third-party service._  
**I Wanted**: _Notification if the alarm triggered._

I was interested in finding a solution that directly accessed the pair of data lines that DSC uses for their proprietary Keybus protocol to send data between the panel, keypads, and other modules.  Tapping into the data lines is an ideal task for a microcontroller and also presented an opportunity to work with the [Arduino](https://www.arduino.cc) and [FreeRTOS](https://www.freertos.org) (via [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos)) platforms.

While there has been excellent [discussion about the DSC Keybus protocol](https://www.avrfreaks.net/forum/dsc-keybus-protocol) and a few existing projects, there were major issues that remained unsolved:
* Error-prone Keybus data capture.
* Limited data decoding - there was good progress for armed/disarmed states and partial zone status for a single partition, but otherwise most of the data was undecoded (notably missing the alarm triggered state).
* Read-only - unable to control the Keybus to act as a virtual keypad.
* No implementations to do useful work with the data.

Poking around with a logic analyzer and oscilloscope revealed that the errors capturing the Keybus data were timing issues.  Updating the existing projects to fix this turned out to be more troublesome than starting from scratch, so this project was born.  After resolving the data errors, it was possible to reverse engineer the protocol by capturing the Keybus binary data as the security system handled various events.  At this point, this interface resolves all of the earlier issues (and goes beyond my initial goal of simply seeing if the alarm is triggered).

## Features
* Monitor the alarm state of all partitions:
  - Alarm triggered, armed/disarmed, entry/exit delay, fire triggered, keypad panic keys
* Monitor zones status:
  - Zones open/closed, zones in alarm
* Monitor system status:
  - Ready, trouble, AC power, battery
* Panel time - retrieve current panel date/time and set a new date/time
* Virtual keypad:
  - Send keys to the panel for any partition
* Direct Keybus interface:
  - Does not require the [DSC IT-100 serial interface](https://www.dsc.com/alarm-security-products/IT-100%20-%20PowerSeries%20Integration%20Module/22).
* Supported security systems:
  - [DSC PowerSeries](https://www.dsc.com/?n=enduser&o=identify)
  - Verified panels: PC585, PC1555MX, PC1565, PC5005, PC5015, PC1616, PC1808, PC1832, PC1864.
  - All PowerSeries series are supported, please [post an issue](https://github.com/taligentx/dscKeybusInterface/issues) if you have a different panel (PC5020, etc) and have tested the interface to update this list.
  - Rebranded DSC PowerSeries (such as some ADT systems) should also work with this interface.
* Supported microcontrollers:
    - esp8266:
      * Development boards: NodeMCU v2 or v3, Wemos D1 Mini, etc.
      * Includes [Arduino framework support](https://github.com/esp8266/Arduino) and integrated WiFi for ~$3USD shipped.
* Designed for reliable data decoding and performance:
  - Pin change and timer interrupts for accurate data capture timing
  - Data buffering: helps prevent lost Keybus data if the program is busy
  - Extensive data decoding: the majority of Keybus data as seen in the [DSC IT-100 Data Interface developer's guide](https://cms.dsc.com/download.php?t=1&id=16238) has been reverse engineered and documented in [`src/dscKeybusPrintData-RTOS.c`](https://github.com/taligentx/dscKeybusInterface-RTOS/blob/master/src/dscKeybusPrintData-RTOS.c).
* Unsupported security systems:
  - DSC Classic series ([PC1500, etc](https://www.dsc.com/?n=enduser&o=identify)) use a different data protocol, though support is possible.
  - DSC Neo series use a higher speed encrypted data protocol (Corbus) that is not possible to support.
  - Honeywell, Ademco, and other brands (that are not rebranded DSC systems) use different protocols and are not supported.
* Possible features (PRs welcome!):
  - Virtual zone expander: Add new zones to the DSC panel emulated by the microcontroller based on GPIO pin states or software-based states.  Requires decoding the DSC PC5108 zone expander data.
  - Installer code unlocking: Requires brute force checking all possible codes and a workaround if keypad lockout is enabled (possibly automatically power-cycling the panel with a relay to skip the lockout time).
  - [DSC IT-100](https://cms.dsc.com/download.php?t=1&id=16238) emulation
  - DSC Classic series support: This protocol is [already decoded](https://github.com/dougkpowers/pc1550-interface), use with this library would require major changes.

## Release notes
* 0.1 - Initial release

## Examples
The included examples demonstrate how to use the library and can be used as-is or adapted to integrate with other software.  Post an issue/pull request if you've developed (and would like to share) a program/integration that others can use.

* **HomeKit**: Integrates with Apple HomeKit, including the iOS Home app and Siri.  This uses [esp-homekit](https://github.com/maximkulkin/esp-homekit) to directly integrate with [Apple HomeKit & Siri](https://www.apple.com/ios/home/) as a standalone accessory (without using an intermediary like [Homebridge](https://github.com/nfarina/homebridge), [HAP-NodeJS](https://github.com/KhaosT/HAP-NodeJS), etc.).  This demonstrates using the armed and alarm states for the HomeKit securitySystem object, zone states for the contactSensor and motionSensor objects, and fire alarm states for the smokeSensor object.

* **Status**: Processes and prints the security system status to a serial interface, including reading from serial for the virtual keypad.  This demonstrates how to determine if the security system status has changed, what has changed, and how to take action based on those changes.  Post an issue/pull request if you have a use for additional system states - for now, only a subset of all decoded commands are being tracked for status to limit memory usage:
  * Partitions ready
  * Partitions armed away/stay/disarmed
  * Partitions in alarm
  * Partitions exit delay in progress
  * Partitions entry delay in progress
  * Partitions fire alarm
  * Zones open/closed
  * Zones in alarm
  * Get/set panel date and time
  * Keypad fire/auxiliary/panic alarm
  * Panel AC power
  * Panel battery
  * Panel trouble
  * Keybus connected

* **KeybusReader**: Decodes and prints data from the Keybus to a serial interface, including reading from serial for the virtual keypad.  This can be used to help decode the Keybus protocol and is also handy as a troubleshooting tool to verify that data is displayed without errors.

  See [`src/dscKeybusPrintData-RTOS.c`](https://github.com/taligentx/dscKeybusInterface-RTOS/blob/master/src/dscKeybusPrintData-RTOS.c) for all currently known Keybus protocol commands and messages.  Issues and pull requests with additions/corrections are welcome!

## Installation - Ubuntu 18.04+
This example installs all components to the `esp` directory in your home directory (`~/esp/`).

* Create and change to a directory to contain all components:

  ```
  $ cd ~/
  ~$ mkdir esp
  ~$ cd esp
  ```
* Install [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk):

  1. Install required packages:    
      ```
      ~/esp$ sudo apt install make unrar-free autoconf automake libtool gcc g++ gperf \
      flex bison texinfo gawk ncurses-dev libexpat-dev python-dev python python-serial \
      sed git unzip bash help2man wget bzip2 libtool-bin
      ```

  2. Clone the esp-open-sdk repository: 

      ```
      ~/esp$ git clone --recursive https://github.com/pfalcon/esp-open-sdk.git
      ```

  3. Build the esp-open-sdk toolchain (this can take 10-15 minutes):
       ```
       ~/esp$ cd esp-open-sdk
       ~/esp/esp-open-sdk$ make toolchain esptool libhal STANDALONE=n
       ```

* Install [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos):
    ```
    ~/esp/esp-open-sdk$ cd ~/esp
    ~/esp$ git clone --recursive https://github.com/Superhouse/esp-open-rtos.git
    ```

* Add paths to esp-open-sdk and esp-open-rtos:
  ```
  ~/esp$ nano ~/.bashrc

  Add the following (with your actual username), then save and close:
  PATH=$PATH:/home/myusername/esp/esp-open-sdk/xtensa-lx106-elf/bin
  export SDK_PATH=/home/myusername/esp/esp-open-rtos

  ~/esp$ source ~/.bashrc
  ```

* Install dscKeybusInterface-RTOS:

  1. Clone the dscKeybusInterface-RTOS repository:

      ```
      ~/esp$ git clone https://github.com/taligentx/dscKeybusInterface-RTOS.git
      ```

  2. Navigate to one of the examples and edit the Makefile to set the esp8266 serial port, baud rate, and flash size:
      ```
      ~/esp$ cd dscKeybusInterface-RTOS/examples/esp8266/KeybusReader
      KeybusReader$ nano Makefile
      ```

  3. Build the example and flash the esp8266 - check the example for specific usage documentation:

      ```
      KeybusReader$ make flash
      ```

## Installation - macOS 10.11+
This example installs all components to an `esp` disk image - this is necessary if macOS is installed with the default case-insensitive file system:

* Create a disk image to contain all components:

    ```
    $ hdiutil create ~/Documents/esp.dmg -volname "esp" -size 5g -fs "Case-sensitive HFS+"
    $ hdiutil mount ~/Documents/esp.dmg
    ```

* Install [Homebrew](https://brew.sh):

  ```
  $ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  ```

* Install [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk):

  1. Install required packages:
  
      ```
      $ brew install binutils coreutils automake wget gawk libtool help2man gperf gnu-sed grep
      ```

  2. Add a path to the Homebrew installation of gnu-sed:

      ```
      $ nano ~/.bash_profile

      Add the following, then save and close:
      export PATH="/usr/local/opt/gnu-sed/libexec/gnubin:$PATH"

      $ source ~/.bash_profile
      ```

  3. Clone the esp-open-sdk repository:

      ```
      $ cd /Volumes/esp
      esp$ git clone --recursive https://github.com/pfalcon/esp-open-sdk.git
      ```

  4. Build the esp-open-sdk toolchain (this can take 10-15 minutes):

      ```
      esp$ cd esp-open-sdk
      esp-open-sdk$ make toolchain esptool libhal STANDALONE=n
      ```

* Install [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos):
    ```
    esp-open-sdk$ cd /Volumes/esp
    esp$ git clone --recursive https://github.com/Superhouse/esp-open-rtos.git
    ```

* Add paths to esp-open-sdk and esp-open-rtos:
    ```
    esp$ nano ~/.bash_profile

    Add the following, then save and close:
    PATH=$PATH:/Volumes/esp/esp-open-sdk/xtensa-lx106-elf/bin
    export SDK_PATH=/Volumes/esp/esp-open-rtos

    esp$ source ~/.bash_profile
    ```

* Install dscKeybusInterface-RTOS:

  1. Clone the dscKeybusInterface-RTOS repository:

      ```
      esp$ git clone https://github.com/taligentx/dscKeybusInterface-RTOS.git
      ```

  2. Navigate to one of the examples and edit the Makefile to set the esp8266 serial port, baud rate, and flash size::
      ```
      esp$ cd dscKeybusInterface-RTOS/examples/esp8266/KeybusReader
      KeybusReader$ nano Makefile
      ```

  3. Build the example and flash the esp8266 - check the example for specific usage documentation:

      ```
      KeybusReader$ make flash
      ```

## Wiring

```
DSC Aux(+) --- 5v voltage regulator --- esp8266 development board 5v pin (NodeMCU, Wemos)

DSC Aux(-) --- esp8266 Ground

                                   +--- dscClockPin (esp8266: D1, D2, D8)
DSC Yellow --- 15k ohm resistor ---|
                                   +--- 10k ohm resistor --- Ground

                                   +--- dscReadPin (esp8266: D1, D2, D8)
DSC Green ---- 15k ohm resistor ---|
                                   +--- 10k ohm resistor --- Ground 

Virtual keypad (optional):
DSC Green ---- NPN collector --\
                                |-- NPN base --- 1k ohm resistor --- dscWritePin (esp8266: D1, D2, D8)
      Ground --- NPN emitter --/
```

* The DSC Keybus operates at ~12.6v, a pair of resistors per data line will bring this down to an appropriate voltage for each microcontroller.
    * esp8266: connect the DSC lines to GPIO pins that are normally low to avoid putting spurious data on the Keybus: D1 (GPIO5), D2 (GPIO4) and D8 (GPIO15).
* Virtual keypad uses an NPN transistor and a resistor to write to the Keybus.  Most small signal NPN transistors should be suitable, for example:
  * 2N3904
  * BC547, BC548, BC549
  * That random NPN at the bottom of your parts bin (my choice)
* Power:
  * esp8266 development boards should use a 5v voltage regulator:
    - LM2596-based step-down buck converter modules are reasonably efficient and commonly available for under $1USD shipped (eBay, Aliexpress, etc).
    - MP2307-based step-down buck converter modules (aka Mini360) are also available but some versions run hot with an efficiency nearly as poor as linear regulators.
    - Linear voltage regulators (LM7805, etc) will work but are inefficient and run hot - these may need a heatsink.
* Connections should be soldered, breadboards can cause issues.

## Virtual keypad
This allows a program to send keys to the DSC panel to emulate the physical DSC keypads and enables full control of the panel from the program or other software.

Keys are sent to partition 1 by default and can be changed to a different partition.  The following keys can be sent to the panel - see the examples for usage:

* Keypad: `0-9 * #`
* Arm stay (requires access code if quick arm is disabled): `s`
* Arm away (requires access code if quick arm is disabled): `w`
* Arm with no entry delay (requires access code): `n`
* Fire alarm: `f`
* Auxiliary alarm: `a`
* Panic alarm: `p`
* Door chime enable/disable: `c`
* Fire reset: `r`
* Quick exit: `x`
* Change partition: `/` + `partition number` or set `dscWritePartition` to the partition number.  Examples:
  * Switch to partition 2 and send keys: `/2` + `1234`
  * Switch back to partition 1: `/1`
  * Set directly in program: `dscWritePartition = 8;`
* Command output 1: `[`
* Command output 2: `]`
* Command output 3: `{`
* Command output 4: `}`

## DSC Configuration
Panel options affecting this interface, configured by `*8 + installer code` - see the DSC installation manual for your panel for configuration steps:
* PC1555MX/5015 section 370, PC1616/PC1832/PC1864 section 377:
  - Swinger shutdown: By default, the panel will limit the number of alarm commands sent in a single armed cycle to 3 - for example, a zone alarm being triggered multiple times will stop reporting after 3 alerts.  This is to avoid sending alerts repeatedly to a third-party monitoring service, and also affects this interface.  As I do not use a monitoring service, I disable swinger shutdown by setting this to `000`.

  - AC power failure reporting delay: The default delay is 30 minutes and can be set to 000 to immediately report a power failure.  

## Notes
* Memory usage can be adjusted based on the number of partitions, zones, and data buffer size specified in [`src/dscKeybusInterface-RTOS.h`](https://github.com/taligentx/dscKeybusInterface-RTOS/blob/master/src/dscKeybusInterface-RTOS.h).  Default settings:
  * esp8266: up to 8 partitions, 64 zones, 50 buffered commands

* PCB layouts are available in [`extras/PCB Layouts`](https://github.com/taligentx/dscKeybusInterface-RTOS/tree/master/extras/PCB%20Layouts) - thanks to [sjlouw](https://github.com/sj-louw) for contributing these designs!

* Support for other platforms depends on adjusting the code to use their platform-specific timers.  In addition to hardware interrupts to capture the DSC clock, this library uses platform-specific timer interrupts to capture the DSC data line in a non-blocking way 250μs after the clock changes.  This is necessary because the clock and data are asynchronous - I've observed keypad data delayed up to 160us after the clock falls.

## Troubleshooting
If you are running into issues:
1. Run the KeybusReader example program and view the serial output to verify that the interface is capturing data successfully without reporting CRC errors.  
    * If data is not showing up or has errors, check the clock and data line wiring, resistors, and all connections.
2. For virtual keypad, run the KeybusReader example program and enter keys through serial and verify that the keys appear in the output and that the panel responds.  
    * If keys are not displayed in the output, verify the transistor pinout, base resistor, and wiring connections.
3. Run the Status example program and view the serial output to verify that the interface displays events from the security system correctly as partitions are armed, zones opened, etc.

## References
[AVR Freaks - DSC Keybus Protocol](https://www.avrfreaks.net/forum/dsc-keybus-protocol): An excellent discussion on how data is sent on the Keybus.

[stagf15/DSC_Panel](https://github.com/stagf15/DSC_Panel): A library that nearly works for the PC1555MX but had timing and data errors.  Writing this library from scratch was primarily a programming exercise, otherwise it should be possible to patch the DSC_Panel library.

[dougkpowers/pc1550-interface](https://github.com/dougkpowers/pc1550-interface): An interface for the DSC Classic series.
