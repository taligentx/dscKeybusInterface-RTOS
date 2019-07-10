/*
 *  DSC-RTOS HomeKit 1.0 (esp8266)
 *
 *  Processes the security system status and allows for control using Apple HomeKit, including the iOS Home app and
 *  Siri.  This uses esp-open-rtos and esp-homekit to enable the esp8266 to directly integrate with HomeKit as a
 *  standalone accessory and demonstrates using the partition 1 armed and alarm states for the HomeKit securitySystem
 *  object, zone states for the contactSensor and motionSensor objects, and fire alarm states for the smokeSensor
 *  object.  Additional partitions and zones can be setup by cloning the example code.
 *
 *  Usage (macOS/Linux):
 *    1. Set the security system access code to permit disarming through HomeKit.
 *
 *    2. Set the HomeKit setup code to a unique value for pairing.
 *
 *    3. Configure partitions, zones, and fire sensors as needed (see below).
 *
 *    4. Edit dscSettings.h to configure GPIO pins and WiFi settings.
 *
 *    5. Edit Makefile to set the esp8266 serial port, baud rate, and flash size.
 *
 *    6. Build the example and flash the esp8266:
 *         $ make flash
 *       (If a previous flash causes issues, erase before flashing: make erase_flash)
 *
 *    7. Open the serial interface to view status (close with CTRL-a, k):
 *         $ screen /dev/tty.wchusbserial410 115200
 *
 *    8. Open the iOS Home app, select (+) to add an accessory, select "Don't have a Code or Can't Scan?"
 *
 *    9. Select the "Security System" accessory and pair with the setup code - this can take up to 30 seconds.
 *
 *
 *  Partitions, zones, and fire sensors are currently configured individually - making these dynamically generated
 *  would be ideal (esp-homekit includes an example of dynamic services).
 *
 *  Adding a new zone (see each section for examples):
 *    1. Add a characteristic for the zone: homekit_characteristic_t serviceZoneX
 *    2. In accessories[], add a service for the zone and set the characteristic: HOMEKIT_SERVICE(CONTACT_SENSOR, ...
 *    3. In dscLoop(), add a status check for the zone - clone zone 1 and update the characteristics: if (bitRead(dscOpenZonesChanged...
 *
 *  Adding a new partition (see each section for examples):
 *    1. Add a characterisic for the partition current state: homekit_characteristic_t servicePartitionXCurrentState
 *    2. Add a function definition for the partition target state setter: void setPartitionXTargetState
 *    3. Add a characteristic for the partition target state: homekit_characteristic_t servicePartitionXTargetState
 *    4. In accessories[], add a service for the partition: HOMEKIT_SERVICE(SECURITY_SYSTEM, ...
 *    5. In dscLoop(), add the partition status checks - clone the partition 1 status checks and update the characteristics
 *    6. Add the partition target state setter - clone setPartition1TargetState() and update the characteristics: void setPartitionXTargetState()
 *
 *  Adding a new partition fire sensor (see each section for examples):
 *    1. Add a characteristic for the fire sensor: homekit_characteristic_t servicePartitionXFire
 *    2. In accessories[], add a service for the fire sensor and set the characteristic: HOMEKIT_SERVICE(SMOKE_SENSOR, ...
 *    3. In dscLoop(), add a status check for the fire sensor - clone the partition 1 sensor and update the characteristics: if (dscFireChanged...
 *
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
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

// Settings
char accessCode[] = "1234";  // An access code is required to disarm/night arm and may be required to arm based on panel configuration.
char homekitSetupCode[] = "111-11-111";  // HomeKit pairing code

enum alarmState {STAY_ARM, AWAY_ARM, NIGHT_ARM, DISARMED, ALARM_TRIGGERED};
char exitState;


// HomeKit zones characteristic - add a characteristic for each zone and set the appropriate characteristic: CONTACT_SENSOR_STATE, MOTION_DETECTED
homekit_characteristic_t serviceZone1 = HOMEKIT_CHARACTERISTIC_(CONTACT_SENSOR_STATE, 0, NULL);
homekit_characteristic_t serviceZone2 = HOMEKIT_CHARACTERISTIC_(MOTION_DETECTED, 0, NULL);
// homekit_characteristic_t serviceZone3 = HOMEKIT_CHARACTERISTIC_(CONTACT_SENSOR_STATE, 0, NULL);
// ...


// HomeKit partitions current state characteristic - add a characteristic for each partition
homekit_characteristic_t servicePartition1CurrentState = HOMEKIT_CHARACTERISTIC_(SECURITY_SYSTEM_CURRENT_STATE, DISARMED,);
// homekit_characteristic_t servicePartition2CurrentState = HOMEKIT_CHARACTERISTIC_(SECURITY_SYSTEM_CURRENT_STATE, DISARMED,);
// ...


// HomeKit partitions setPartitionXTargetState function definition - add a definition for each partition
void setPartition1TargetState(homekit_value_t value);
// void setPartition2TargetState(homekit_value_t value);
// ...


// HomeKit partitions target state characteristic - add a characteristic for each partition
homekit_characteristic_t servicePartition1TargetState = HOMEKIT_CHARACTERISTIC_(SECURITY_SYSTEM_TARGET_STATE, DISARMED, .setter=setPartition1TargetState,);
// homekit_characteristic_t servicePartition2TargetState = HOMEKIT_CHARACTERISTIC_(SECURITY_SYSTEM_TARGET_STATE, DISARMED, .setter=setPartition2TargetState,);
// ...


// HomeKit partitions fire characteristic - add a characteristic for each partition
homekit_characteristic_t servicePartition1Fire = HOMEKIT_CHARACTERISTIC_(SMOKE_DETECTED, 0, NULL);
// homekit_characteristic_t servicePartition2Fire = HOMEKIT_CHARACTERISTIC_(SMOKE_DETECTED, 0, NULL);
// ...


// HomeKit accessory identification during pairing
void dscIdentify(homekit_value_t _value) {
    printf("DSC Keybus Interface identifying\n");
}


// HomeKit accessory and services definition - add services to match the DSC configuration
homekit_accessory_t *accessories[] = {
  HOMEKIT_ACCESSORY(
    .id=1,
    .category=homekit_accessory_category_security_system,
    .services=(homekit_service_t*[]) {
      HOMEKIT_SERVICE(
        ACCESSORY_INFORMATION,
        .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Security System"),
          HOMEKIT_CHARACTERISTIC(MANUFACTURER, "DSC"),
          HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "8675309"),
          HOMEKIT_CHARACTERISTIC(MODEL, "PC1864"),
          HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
          HOMEKIT_CHARACTERISTIC(IDENTIFY, dscIdentify),
          NULL
        },
      ),

      // Partition 1 service definition - clone to add additional partitions
      HOMEKIT_SERVICE(
        SECURITY_SYSTEM,
        .primary=true,
        .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Security System 1"),
          &servicePartition1CurrentState,  // Partition 1 current state characteristic
          &servicePartition1TargetState,   // Partition 1 target state characteristic
          NULL
        },
      ),

      // Zone 1 service definition - clone to add additional zones
      HOMEKIT_SERVICE(
        CONTACT_SENSOR,
        .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Zone 1"),
          &serviceZone1,  // Zone 1 service characteristic
          NULL
        },
      ),

      // Zone 2 service definition
      HOMEKIT_SERVICE(
        MOTION_SENSOR,
        .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Zone 2"),
          &serviceZone2,  // Zone 2 service characteristic
          NULL
        },
      ),

      // Partition 1 fire definition - clone to add additional partition fire sensors
      HOMEKIT_SERVICE(
        SMOKE_SENSOR,
        .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Fire 1"),
          &servicePartition1Fire,  // Partition 1 fire service characteristic
          NULL
        },
      ),

      NULL
    },
  ),
  NULL
};


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

      // Sends the access code when needed by the panel for arming
      if (dscAccessCodePrompt) {
        dscAccessCodePrompt = false;
        dscWriteKeys(accessCode);
      }

      // Sets partition 1 status
      byte partition = 0;

      // Publishes armed/disarmed status
      if (dscArmedChanged[partition]) {
        dscArmedChanged[partition] = false;  // Resets the partition armed status flag

        if (dscArmed[partition]) {
          exitState = 0;

          // Night armed away
          if (dscArmedAway[partition] && dscNoEntryDelay[partition]) {
            servicePartition1TargetState.value = HOMEKIT_UINT8(NIGHT_ARM);
            homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
            servicePartition1CurrentState.value = HOMEKIT_UINT8(NIGHT_ARM);
            homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
          }

          // Armed away
          else if (dscArmedAway[partition]) {
            servicePartition1TargetState.value = HOMEKIT_UINT8(AWAY_ARM);
            homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
            servicePartition1CurrentState.value = HOMEKIT_UINT8(AWAY_ARM);
            homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
          }

          // Night armed stay
          else if (dscArmedStay[partition] && dscNoEntryDelay[partition]) {
            servicePartition1TargetState.value = HOMEKIT_UINT8(NIGHT_ARM);
            homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
            servicePartition1CurrentState.value = HOMEKIT_UINT8(NIGHT_ARM);
            homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
          }

          // Armed stay
          else if (dscArmedStay[partition]) {
            servicePartition1TargetState.value = HOMEKIT_UINT8(STAY_ARM);
            homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
            servicePartition1CurrentState.value = HOMEKIT_UINT8(STAY_ARM);
            homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
          }
        }

        // Disarmed
        else {
          servicePartition1TargetState.value = HOMEKIT_UINT8(DISARMED);
          homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
          servicePartition1CurrentState.value = HOMEKIT_UINT8(DISARMED);
          homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
        }
      }

      // Checks exit delay status
      if (dscExitDelayChanged[partition]) {
        dscExitDelayChanged[partition] = false;  // Resets the exit delay status flag

        // Exit delay in progress
        if (dscExitDelay[partition]) {

          // Sets the arming target state if the panel is armed externally
          if (exitState == 0 || dscExitStateChanged[partition]) {
            dscExitStateChanged[partition] = 0;
            switch (dscExitState[partition]) {
              case DSC_EXIT_STAY: {
                exitState = 'S';
                servicePartition1TargetState.value = HOMEKIT_UINT8(STAY_ARM);
                break;
              }
              case DSC_EXIT_AWAY: {
                exitState = 'A';
                servicePartition1TargetState.value = HOMEKIT_UINT8(AWAY_ARM);
                break;
              }
              case DSC_EXIT_NO_ENTRY_DELAY: {
                exitState = 'N';
                servicePartition1TargetState.value = HOMEKIT_UINT8(NIGHT_ARM);
                break;
              }
            }
            homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
          }
        }

        // Disarmed during exit delay
        else if (!dscArmed[partition]) {
          exitState = 0;
          servicePartition1TargetState.value = HOMEKIT_UINT8(DISARMED);
          homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
          servicePartition1CurrentState.value = HOMEKIT_UINT8(DISARMED);
          homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
        }
      }

      // Publishes alarm triggered status
      if (dscAlarmChanged[partition]) {
        dscAlarmChanged[partition] = false;  // Resets the partition alarm status flag
        if (dscAlarm[partition]) {
          servicePartition1CurrentState.value = HOMEKIT_UINT8(ALARM_TRIGGERED);
          homekit_characteristic_notify(&servicePartition1CurrentState, servicePartition1CurrentState.value);
        }
      }

      // Publishes fire alarm status
      if (dscFireChanged[partition]) {
        dscFireChanged[partition] = false;  // Resets the fire status flag
        servicePartition1Fire.value = HOMEKIT_UINT8(dscFire[partition]);
        homekit_characteristic_notify(&servicePartition1Fire, servicePartition1Fire.value);
      }

      // Checks for open zones
      // Zone status is stored in the openZones[] and openZonesChanged[] arrays using 1 bit per zone, up to 64 zones
      //   openZones[0] and openZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
      //   openZones[1] and openZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
      //   ...
      //   openZones[7] and openZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
      if (dscOpenZonesStatusChanged) {
        dscOpenZonesStatusChanged = false;  // Resets the open zones status flag

        // Sets zone 1 status - clone to add additional zones
        if (bitRead(dscOpenZonesChanged[0], 0)) {  // Checks open zone 1 status flag
          serviceZone1.value = HOMEKIT_UINT8(bitRead(dscOpenZones[0], 0));   // Gets zone 1 status
          homekit_characteristic_notify(&serviceZone1, serviceZone1.value);  // Updates HomeKit with zone 1 status
        }

        // Sets zone 2 status
        if (bitRead(dscOpenZonesChanged[0], 1)) {  // Checks open zone 2 status flag
          serviceZone2.value = HOMEKIT_BOOL(bitRead(dscOpenZones[0], 1));    // Gets zone 2 status
          homekit_characteristic_notify(&serviceZone2, serviceZone2.value);  // Updates HomeKit with zone 2 status
        }

        // Resets the zones changed status flag
        for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
          for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
            if (bitRead(dscOpenZonesChanged[zoneGroup], zoneBit)) {  // Checks an individual open zone status flag
              bitWrite(dscOpenZonesChanged[zoneGroup], zoneBit, 0);  // Resets the individual open zone status flag
            }
          }
        }
      }
    }
  }
}


// Sets the partition 1 target state - clone to add new partitions, set "byte partition" to (partition number -1)
void setPartition1TargetState(homekit_value_t value) {
  byte partition = 0;
  servicePartition1TargetState.value = value;

  // Resets the HomeKit target state if attempting to change the armed mode while armed or not ready
  if (value.int_value != DISARMED && !dscReady[partition]) {
    dscArmedChanged[partition] = true;
    dscStatusChanged = true;
    xSemaphoreGive(dscDataAvailable);
    return;
  }

  // Resets the HomeKit target state if attempting to change the arming mode during the exit delay
  if (value.int_value != DISARMED && dscExitDelay[partition] && exitState != 0) {
    if (exitState == 'S') {
      servicePartition1TargetState.value = HOMEKIT_UINT8(STAY_ARM);
      homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    }
    else if (exitState == 'A') {
      servicePartition1TargetState.value = HOMEKIT_UINT8(AWAY_ARM);
      homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    }
    else if (exitState == 'N') {
      servicePartition1TargetState.value = HOMEKIT_UINT8(NIGHT_ARM);
      homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    }
  }

  // Stay arm
  if (value.int_value == STAY_ARM && !dscArmed[partition] && !dscExitDelay[partition]) {
    dscWritePartition = partition + 1;    // Sets writes to the partition number
    dscWriteKey('s');  // Keypad stay arm
    servicePartition1TargetState.value = HOMEKIT_UINT8(STAY_ARM);
    homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    exitState = 'S';
    return;
  }

  // Away arm
  if (value.int_value == AWAY_ARM && !dscArmed[partition] && !dscExitDelay[partition]) {
    dscWritePartition = partition + 1;    // Sets writes to the partition number
    dscWriteKey('w');  // Keypad away arm
    servicePartition1TargetState.value = HOMEKIT_UINT8(AWAY_ARM);
    homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    exitState = 'A';
    return;
  }

  // Night arm
  if (value.int_value == NIGHT_ARM && !dscArmed[partition] && !dscExitDelay[partition]) {
    dscWritePartition = partition + 1;    // Sets writes to the partition number
    dscWriteKey('n');  // Keypad arm with no entry delay
    servicePartition1TargetState.value = HOMEKIT_UINT8(NIGHT_ARM);
    homekit_characteristic_notify(&servicePartition1TargetState, servicePartition1TargetState.value);
    exitState = 'N';
    return;
  }

  // Disarm
  if (value.int_value == DISARMED && (dscArmed[partition] || dscExitDelay[partition])) {
    dscWritePartition = partition + 1;    // Sets writes to the partition number
    dscWriteKeys(accessCode);
    return;
  }
}


homekit_server_config_t config = {
    .accessories = accessories,
    .password = homekitSetupCode
};


void wifiLoop(void *pvParameters) {
  uint8_t wifi_alive = 0;
  uint8_t status = 0;
  uint8_t retries = 30;
  struct sdk_station_config wifi_config = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD, };
  bool homekitInitialized = false;

  printf("Connecting to WiFi...\n");
  sdk_wifi_set_opmode (STATION_MODE);
  sdk_wifi_station_set_config(&wifi_config);
  sdk_wifi_station_connect();

  while (1) {
    wifi_alive = 0;

    while ((status != STATION_GOT_IP) && (retries)) {
      status = sdk_wifi_station_get_connect_status();
      if (status == STATION_WRONG_PASSWORD) {
        printf("WiFi: wrong password\n");
        break;
      }
      else if (status == STATION_NO_AP_FOUND) {
        printf("WiFi: AP not found\n");
        break;
      }
      else if (status == STATION_CONNECT_FAIL) {
        printf("WiFi: connection failed\n");
        break;
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      --retries;
    }

    while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP) {
      if (wifi_alive == 0) {
        printf("WiFi connected\n");
        wifi_alive = 1;
        if (!homekitInitialized) {
          homekitInitialized = true;
          homekit_server_init(&config);
        }
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    wifi_alive = 0;
    printf("WiFi disconnected\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


void user_init(void) {

  // Serial setup
  uart_set_baud(0, 115200);
  vTaskDelay(1);

  // dscKeybusInterface-RTOS setup
  dscProcessRedundantData = true;  // Controls if repeated periodic commands are processed
  dscProcessModuleData = false;    // Controls if keypad and module data is processed and displayed
  dscBegin();

  // Task setup
  xTaskCreate(&wifiLoop, "wifiLoop", 384, NULL, 1, NULL);
  xTaskCreate(dscLoop, "dscLoop", 256, NULL, 1, NULL);
}
