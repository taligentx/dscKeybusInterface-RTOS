/*
    DSC Keybus Interface-RTOS

    https://github.com/taligentx/dscKeybusInterface-RTOS

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef dscKeybusInterface_h
#define dscKeybusInterface_h

#include <FreeRTOS.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <stdio.h>
#include <task.h>
#include <semphr.h>
#include <string.h>
#include "dscSettings.h"


// Data setup - partitions, zones, and buffer size can be overridden by setting in dscSettings.h
#ifndef dscPartitions
#define dscPartitions 8   // Maximum number of partitions - requires 19 bytes of memory per partition
#endif
#ifndef dscZones
#define dscZones 8        // Maximum number of zone groups, 8 zones per group - requires 6 bytes of memory per zone group
#endif
#ifndef dscBufferSize
#define dscBufferSize 50  // Number of commands to buffer if the sketch is busy - requires dscReadSize + 2 bytes of memory per command
#endif
#define dscReadSize 16    // Maximum bytes of a Keybus command

// Arduino syntax compatibility wrappers
#define HIGH 1
#define LOW 0
typedef unsigned char byte;
int digitalRead(uint8_t pin) __attribute__ ((weak, alias("gpio_read")));
void digitalWrite(uint8_t pin, uint8_t val) __attribute__ ((weak, alias("gpio_write")));
#define millis()  (xTaskGetTickCount() * portTICK_PERIOD_MS)
#define micros() sdk_system_get_time()
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

// esp8266 NodeMCU/Wemos development board pins to GPIO mapping
#define D0 16
#define D1  5
#define D2  4
#define D3  0
#define D4  2
#define D5 14
#define D6 12
#define D7 13
#define D8 15

// Exit delay target states
#define DSC_EXIT_STAY 1
#define DSC_EXIT_AWAY 2
#define DSC_EXIT_NO_ENTRY_DELAY 3

// Task handling
TaskHandle_t dscPanelLoopHandle;
SemaphoreHandle_t dscDataAvailable;
bool dscPanelDataAvailable;

// dscKeybusInterface library public
void dscBegin();
void dscStop();                                // Disables the clock hardware interrupt and data timer interrupt
void dscResetStatus();                         // Resets the state of all status components as changed for sketches to get the current status

// Write
void dscWriteKey(int receivedKey);             // Writes a single key
void dscWriteKeys(const char * receivedKeys);  // Writes multiple keys from a char array
byte dscWritePartition;                        // Set to a partition number for virtual keypad

// Prints output
void dscPrintPanelBinary(bool printSpaces);    // Includes spaces between bytes by default
void dscPrintPanelCommand();                   // Prints the panel command as hex
void dscPrintPanelMessage();                   // Prints the decoded panel message
void dscPrintModuleBinary(bool printSpaces);   // Includes spaces between bytes by default
void dscPrintModuleMessage();                  // Prints the decoded keypad or module message

// Settings
bool dscProcessRedundantData;      // Controls if repeated periodic commands are processed and displayed (default: false)
bool dscProcessModuleData;         // Controls if keypad and module data is processed and displayed (default: false)

// Panel time
bool dscTimestampChanged;          // True after the panel sends a timestamped message
byte dscHour, dscMinute, dscDay, dscMonth;
int dscYear;

// Sets panel time, the dscYear can be sent as either 2 or 4 digits
void dscSetTime(unsigned int dscYear, byte dscMonth, byte dscDay, byte dscHour, byte dscMinute, const char* dscAccessCode);

// Status tracking
bool dscStatusChanged;                      // True after any status change
bool dscPauseStatus;                        // Prevent status from showing as changed, set in sketch to control when to update status
bool dscKeybusConnected, dscKeybusChanged;  // True if data is detected on the Keybus
byte dscAccessCode[dscPartitions];
bool dscAccessCodeChanged[dscPartitions];
bool dscAccessCodePrompt;                   // True if the panel is requesting an access code
bool dscTrouble, dscTroubleChanged;
bool dscPowerTrouble, dscPowerChanged;
bool dscBatteryTrouble, dscBatteryChanged;
bool dscKeypadFireAlarm, dscKeypadAuxAlarm, dscKeypadPanicAlarm;
bool dscReady[dscPartitions], dscReadyChanged[dscPartitions];
bool dscArmed[dscPartitions], dscArmedAway[dscPartitions], dscArmedStay[dscPartitions];
bool dscNoEntryDelay[dscPartitions], dscArmedChanged[dscPartitions];
bool dscAlarm[dscPartitions], dscAlarmChanged[dscPartitions];
bool dscExitDelay[dscPartitions], dscExitDelayChanged[dscPartitions];
byte dscExitState[dscPartitions], dscExitStateChanged[dscPartitions];
bool dscEntryDelay[dscPartitions], dscEntryDelayChanged[dscPartitions];
bool dscFire[dscPartitions], dscFireChanged[dscPartitions];
bool dscOpenZonesStatusChanged;
byte dscOpenZones[dscZones], dscOpenZonesChanged[dscZones];    // Zone status is stored in an array using 1 bit per zone, up to 64 zones
bool dscAlarmZonesStatusChanged;
byte dscAlarmZones[dscZones], dscAlarmZonesChanged[dscZones];  // Zone alarm status is stored in an array using 1 bit per zone, up to 64 zones

// dscPanelData[] and dscModuleData[] store panel and keypad data in an array: command [0], stop bit by itself [1],
// followed by the remaining data.  These can be accessed directly in the program to get data that is not already
// tracked in the library.  See dscKeybusPrintData-RTOS.c for the currently known DSC commands and data.
//
// dscPanelData[] example:
//   Byte 0     Byte 2   Byte 3   Byte 4   Byte 5
//   00000101 0 10000001 00000001 10010001 11000111 [0x05] Status lights: Ready Backlight | Partition ready
//            ^ Byte 1 (stop bit)
byte dscPanelData[dscReadSize];
volatile byte dscModuleData[dscReadSize];

// dscStatus[] and dscLights[] store the current status message and LED state for each partition.  These can be accessed
// directly in the sketch to get data that is not already tracked in the library.  See dscPrintPanelMessages() and
// dscPrintPanelLights() in dscKeybusPrintData-RTOS.c to see how this data translates to the status message and LED status.
byte dscStatus[dscPartitions];
byte dscLights[dscPartitions];

// Process panel and keypad/module data
void dscPanelLoop();
bool dscHandleModule();  // Returns true if data is available

volatile bool dscBufferOverflow;

// dscKeybusInterface library private
void dscProcessPanelStatus();
void dscProcessPanelStatus0(byte partition, byte dscPanelByte);
void dscProcessPanelStatus2(byte partition, byte dscPanelByte);
void dscProcessPanelStatus4(byte partition, byte dscPanelByte);
void dscProcessPanel_0x27();
void dscProcessPanel_0x2D();
void dscProcessPanel_0x34();
void dscProcessPanel_0x3E();
void dscProcessPanel_0xA5();
void dscProcessPanel_0xE6();
void dscProcessPanel_0xE6_0x09();
void dscProcessPanel_0xE6_0x0B();
void dscProcessPanel_0xE6_0x0D();
void dscProcessPanel_0xE6_0x0F();
void dscProcessPanel_0xEB();

void dscPrintPanelLights(byte dscPanelByte);
void dscPrintPanelMessages(byte dscPanelByte);
void dscPrintPanelBitNumbers(byte dscPanelByte, byte startNumber);
void dscPrintPanelStatus0(byte dscPanelByte);
void dscPrintPanelStatus1(byte dscPanelByte);
void dscPrintPanelStatus2(byte dscPanelByte);
void dscPrintPanelStatus3(byte dscPanelByte);
void dscPrintPanelStatus4(byte dscPanelByte);
void dscPrintPanelStatus14(byte dscPanelByte);
void dscPrintPanel_0x05();
void dscPrintPanel_0x0A();
void dscPrintPanel_0x11();
void dscPrintPanel_0x16();
void dscPrintPanel_0x1B();
void dscPrintPanel_0x1C();
void dscPrintPanel_0x27();
void dscPrintPanel_0x28();
void dscPrintPanel_0x2D();
void dscPrintPanel_0x34();
void dscPrintPanel_0x3E();
void dscPrintPanel_0x4C();
void dscPrintPanel_0x58();
void dscPrintPanel_0x5D();
void dscPrintPanel_0x63();
void dscPrintPanel_0x64();
void dscPrintPanel_0x69();
void dscPrintPanel_0x75();
void dscPrintPanel_0x7A();
void dscPrintPanel_0x7F();
void dscPrintPanel_0x82();
void dscPrintPanel_0x87();
void dscPrintPanel_0x8D();
void dscPrintPanel_0x94();
void dscPrintPanel_0xA5();
void dscPrintPanel_0xB1();
void dscPrintPanel_0xBB();
void dscPrintPanel_0xC3();
void dscPrintPanel_0xCE();
void dscPrintPanel_0xD5();
void dscPrintPanel_0xE6();
void dscPrintPanel_0xE6_0x03();
void dscPrintPanel_0xE6_0x09();
void dscPrintPanel_0xE6_0x0B();
void dscPrintPanel_0xE6_0x0D();
void dscPrintPanel_0xE6_0x0F();
void dscPrintPanel_0xE6_0x17();
void dscPrintPanel_0xE6_0x18();
void dscPrintPanel_0xE6_0x19();
void dscPrintPanel_0xE6_0x1A();
void dscPrintPanel_0xE6_0x1D();
void dscPrintPanel_0xE6_0x20();
void dscPrintPanel_0xE6_0x2B();
void dscPrintPanel_0xE6_0x2C();
void dscPrintPanel_0xE6_0x41();
void dscPrintPanel_0xEB();

void dscPrintModule_0x77();
void dscPrintModule_0xBB();
void dscPrintModule_0xDD();
void dscPrintModule_Panel_0x11();
void dscPrintModule_Panel_0xD5();
void dscPrintModule_Notification();
void dscPrintModule_Keys();

bool dscValidCRC();
void dscSetWriteKey(int receivedKey);
bool dscRedundantPanelData(byte dscPreviousCmd[], volatile byte dscCurrentCmd[], byte checkedBytes);

const char* dscPanelKeysArray;
volatile bool dscPanelKeyPending, dscPanelKeysPending;
bool dscWriteArm[dscPartitions];
bool dscPreviousTrouble;
bool dscPreviousKeybus;
byte dscPreviousAccessCode[dscPartitions];
byte dscPreviousLights[dscPartitions], dscPreviousStatus[dscPartitions];
bool dscPreviousReady[dscPartitions];
bool dscPreviousExitDelay[dscPartitions], dscPreviousEntryDelay[dscPartitions];
byte dscPreviousExitState[dscPartitions];
bool dscPreviousArmed[dscPartitions], dscPreviousArmedStay[dscPartitions];
bool dscPreviousAlarm[dscPartitions];
bool dscPreviousFire[dscPartitions];
byte dscPreviousOpenZones[dscZones], dscPreviousAlarmZones[dscZones];

byte dscWriteByte, dscWriteBit;
bool dscVirtualKeypad;
byte dscPanelKey;
byte dscPanelBitCount, dscPanelByteCount;
volatile bool dscWriteAlarm, dscWriteAsterisk, dscWroteAsterisk;
volatile bool dscModuleDataCaptured;
volatile unsigned long dscClockHighTime, dscKeybusTime;
volatile byte dscPanelBufferLength;
volatile byte dscPanelBuffer[dscBufferSize][dscReadSize];
volatile byte dscPanelBufferBitCount[dscBufferSize], dscPanelBufferByteCount[dscBufferSize];
volatile byte dscModuleBitCount, dscModuleByteCount;
volatile byte dscCurrentCmd, dscStatusCmd;
volatile byte dscIsrPanelData[dscReadSize], dscIsrPanelBitTotal, dscIsrPanelBitCount, dscIsrPanelByteCount;
volatile byte dscIsrModuleData[dscReadSize], dscIsrModuleBitTotal, dscIsrModuleBitCount, dscIsrModuleByteCount;

#endif  // dscKeybusInterface_h
