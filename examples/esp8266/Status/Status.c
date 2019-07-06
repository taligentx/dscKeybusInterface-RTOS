/*
 *  DSC-RTOS Status 1.0 (esp8266)
 *
 *  Processes and prints the security system status to a serial interface, including reading from serial for the
 *  virtual keypad.  This demonstrates how to determine if the security system status has changed, what has
 *  changed, and how to take action based on those changes.
 *
 *  Usage (macOS/Linux):
 *    1. Edit dscSettings.h to configure pins
 *
 *    2. Set the `ESPPORT` environment variable to the esp8266 USB-serial interface as listed in `/dev/` - this
 *       will need to be set again if the system is rebooted or if the `tty` device name changes:
 *         $ export ESPPORT=/dev/tty.wchusbserial410  # Typical style for CH240 USB-serial controllers
 *         $ export ESPPORT=/dev/tty.SLAB_USBtoUART   # Typical style for CP2102 USB-serial controllers
 *
 *    3. Build and flash the esp8266:
 *         $ make flash
 *       (If a previous flash causes issues, erase before flashing: make erase_flash)
 *
 *    4. Open the serial interface (close with CTRL-a, k):
 *         $ screen /dev/tty.wchusbserial410 115200
 *
 *  Release notes:
 *    1.0 - Initial release
 *
 *  Wiring:
 *      DSC Aux(+) --- 5v voltage regulator --- esp8266 development board 5v pin (NodeMCU, Wemos)
 *
 *      DSC Aux(-) --- esp8266 Ground
 *
 *                                         +--- dscClockPin (esp8266: D1, D2, D8)
 *      DSC Yellow --- 15k ohm resistor ---|
 *                                         +--- 10k ohm resistor --- Ground
 *
 *                                         +--- dscReadPin (esp8266: D1, D2, D8)
 *      DSC Green ---- 15k ohm resistor ---|
 *                                         +--- 10k ohm resistor --- Ground
 *
 *  Virtual keypad (optional):
 *      DSC Green ---- NPN collector --\
 *                                      |-- NPN base --- 1k ohm resistor --- dscWritePin (esp8266: D1, D2, D8)
 *            Ground --- NPN emitter --/
 *
 *  Virtual keypad uses an NPN transistor to pull the data line low - most small signal NPN transistors should
 *  be suitable, for example:
 *   -- 2N3904
 *   -- BC547, BC548, BC549
 *
 *  Issues and (especially) pull requests are welcome:
 *  https://github.com/taligentx/dscKeybusInterface-RTOS
 *
 *  This example code is in the public domain.
 */

#include <dscKeybusInterface-RTOS.h>


void dscLoop() {
  while(1) {

    // Blocks this task until valid panel data is available
    xSemaphoreTake(dscDataAvailable, portMAX_DELAY);

    if (dscStatusChanged) {      // Checks if the security system status has changed
      dscStatusChanged = false;  // Reset the status tracking flag

      // If the Keybus data buffer is exceeded, the program is too busy to process all Keybus commands.  Call
      // dscLoop() more often, or increase dscBufferSize in the library: src/dscKeybusInterface-RTOS.h
      if (dscBufferOverflow) {
        printf("Keybus buffer overflow\n");
        dscBufferOverflow = false;
      }

      // Checks if the interface is connected to the Keybus
      if (dscKeybusChanged) {
        dscKeybusChanged = false;                 // Resets the Keybus data status flag
        if (dscKeybusConnected) printf("Keybus connected\n");
        else printf("Keybus disconnected\n");
      }

      // Checks status per partition
      for (byte partition = 0; partition < dscPartitions; partition++) {

        // Checks ready status
        if (dscReadyChanged[partition]) {
          dscReadyChanged[partition] = false;  // Resets the partition ready status flag
          if (dscReady[partition]) {
            printf("Partition %d ready\n", partition + 1);
          }
        }

        // Checks armed status
        if (dscArmedChanged[partition]) {
          dscArmedChanged[partition] = false;  // Resets the partition armed status flag
          if (dscArmed[partition]) {
            printf("Partition %d armed", partition + 1);
            if (dscArmedAway[partition]) printf(" away\n");
            if (dscArmedStay[partition]) printf(" stay\n");
          }
          else printf("Partition %d disarmed\n", partition + 1);
        }

        // Checks alarm triggered status
        if (dscAlarmChanged[partition]) {
          dscAlarmChanged[partition] = false;  // Resets the partition alarm status flag
          if (dscAlarm[partition]) {
            printf("Partition %d in alarm\n", partition + 1);
          }
        }

        // Checks exit delay status
        if (dscExitDelayChanged[partition]) {
          dscExitDelayChanged[partition] = false;  // Resets the exit delay status flag
          if (dscExitDelay[partition]) {
            printf("Partition %d exit delay in progress\n", partition + 1);
          }
          else if (!dscArmed[partition]) {  // Checks for disarm during exit delay
            printf("Partition %d disarmed\n", partition + 1);
          }
        }

        // Checks entry delay status
        if (dscEntryDelayChanged[partition]) {
          dscEntryDelayChanged[partition] = false;  // Resets the exit delay status flag
          if (dscEntryDelay[partition]) {
            printf("Partition %d entry delay in progress\n", partition + 1);
          }
        }

        // Checks the access code used to arm or disarm
        if (dscAccessCodeChanged[partition]) {
          dscAccessCodeChanged[partition] = false;  // Resets the access code status flag
          printf("Partition %d", partition + 1);
          switch (dscAccessCode[partition]) {
            case 33: printf(" duress"); break;
            case 34: printf(" duress"); break;
            case 40: printf(" master"); break;
            case 41: printf(" supervisor"); break;
            case 42: printf(" supervisor"); break;
            default: printf(" user"); break;
          }
          printf(" code %d\n", dscAccessCode[partition]);
        }

        // Checks fire alarm status
        if (dscFireChanged[partition]) {
          dscFireChanged[partition] = false;  // Resets the fire status flag
          if (dscFire[partition]) {
            printf("Partition %d fire alarm on\n", partition + 1);
          }
          else {
            printf("Partition %d fire alarm restored\n", partition + 1);
          }
        }
      }

      // Checks for open zones
      // Zone status is stored in the openZones[] and openZonesChanged[] arrays using 1 bit per zone, up to 64 zones
      //   openZones[0] and openZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
      //   openZones[1] and openZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
      //   ...
      //   openZones[7] and openZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
      if (dscOpenZonesStatusChanged) {
        dscOpenZonesStatusChanged = false;                           // Resets the open zones status flag
        for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
          for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
            if (bitRead(dscOpenZonesChanged[zoneGroup], zoneBit)) {  // Checks an individual open zone status flag
              bitWrite(dscOpenZonesChanged[zoneGroup], zoneBit, 0);  // Resets the individual open zone status flag
              if (bitRead(dscOpenZones[zoneGroup], zoneBit)) {       // Zone open
                printf("Zone open: %d\n", zoneBit + 1 + (zoneGroup * 8));  // Determines the zone number
              }
              else {                                                  // Zone closed
                printf("Zone restored: %d\n", zoneBit + 1 + (zoneGroup * 8));  // Determines the zone number
              }
            }
          }
        }
      }

      // Checks for zones in alarm
      // Zone alarm status is stored in the alarmZones[] and alarmZonesChanged[] arrays using 1 bit per zone, up to 64 zones
      //   alarmZones[0] and alarmZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
      //   alarmZones[1] and alarmZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
      //   ...
      //   alarmZones[7] and alarmZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
      if (dscAlarmZonesStatusChanged) {
        dscAlarmZonesStatusChanged = false;                           // Resets the alarm zones status flag
        for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
          for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
            if (bitRead(dscAlarmZonesChanged[zoneGroup], zoneBit)) {  // Checks an individual alarm zone status flag
              bitWrite(dscAlarmZonesChanged[zoneGroup], zoneBit, 0);  // Resets the individual alarm zone status flag
              if (bitRead(dscAlarmZones[zoneGroup], zoneBit)) {       // Zone alarm
                printf("Zone alarm: %d\n", zoneBit + 1 + (zoneGroup * 8));  // Determines the zone number
              }
              else {
                printf("Zone alarm restored: %d\n", zoneBit + 1 + (zoneGroup * 8));  // Determines the zone number
              }
            }
          }
        }
      }

      // Checks for a panel timestamp
      //
      // The panel time can be set using dsc.setTime(year, month, day, hour, minute, "accessCode") - for example:
      //   dsc.setTime(2015, 10, 21, 7, 28, "1234")  # Sets 2015.10.21 07:28 with access code 1234
      //
      if (dscTimestampChanged) {
        dscTimestampChanged = false;
        printf("Timestamp: %d.%02d.%02d %02d:%02d\n", dscYear, dscMonth, dscDay, dscHour, dscMinute);
      }

      // Checks trouble status
      if (dscTroubleChanged) {
        dscTroubleChanged = false;  // Resets the trouble status flag
        if (dscTrouble) printf("Trouble status on\n");
        else printf("Trouble status restored\n");
      }

      // Checks AC power status
      if (dscPowerChanged) {
        dscPowerChanged = false;  // Resets the power trouble status flag
        if (dscPowerTrouble) printf("Panel AC power trouble\n");
        else printf("Panel AC power restored\n");
      }

      // Checks panel battery status
      if (dscBatteryChanged) {
        dscBatteryChanged = false;  // Resets the battery trouble status flag
        if (dscBatteryTrouble) printf("Panel battery trouble\n");
        else printf("Panel battery restored\n");
      }

      // Checks keypad fire alarm triggered
      if (dscKeypadFireAlarm) {
        dscKeypadFireAlarm = false;  // Resets the keypad fire alarm status flag
        printf("Keypad fire alarm\n");
      }

      // Checks keypad auxiliary alarm triggered
      if (dscKeypadAuxAlarm) {
        dscKeypadAuxAlarm = false;  // Resets the keypad auxiliary alarm status flag
        printf("Keypad aux alarm\n");
      }

      // Checks keypad panic alarm triggered
      if (dscKeypadPanicAlarm) {
        dscKeypadPanicAlarm = false;  // Resets the keypad panic alarm status flag
        printf("Keypad panic alarm\n");
      }
    }

  }
}


// Reads from serial input and writes to the Keybus as a virtual keypad
void dscWrite() {
  while (1) {
    int inputKey = getchar();
    dscWriteKey(inputKey);
  }
}


void user_init(void) {

  // Serial setup
  uart_set_baud(0, 115200);
  vTaskDelay(1);

  // Wifi setup
  sdk_wifi_set_opmode(STATION_MODE);

  // dscKeybusInterface-RTOS setup
  dscProcessRedundantData = false;  // Controls if repeated periodic commands are processed and displayed
  dscProcessModuleData = true;      // Controls if keypad and module data is processed and displayed
  dscBegin();

  // Task setup
  xTaskCreate(dscLoop, "dscLoop", 256, NULL, 1, NULL);
  xTaskCreate(dscWrite, "dscWrite", 256, NULL, 0, NULL);
}
