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

 #include "dscKeybusInterface-RTOS.h"

#ifndef dscWritePin
#define dscWritePin 255
#endif


bool IRAM dscValidCRC() {
  byte byteCount = (dscPanelBitCount - 1) / 8;
  int dataSum = 0;
  for (byte dscPanelByte = 0; dscPanelByte < byteCount; dscPanelByte++) {
    if (dscPanelByte != 1) dataSum += dscPanelData[dscPanelByte];
  }
  if (dataSum % 256 == dscPanelData[byteCount]) return true;
  else return false;
}


bool IRAM dscRedundantPanelData(byte dscPreviousCmd[], volatile byte dscCurrentCmd[], byte checkedBytes) {
  bool redundantData = true;
  for (byte i = 0; i < checkedBytes; i++) {
    if (dscPreviousCmd[i] != dscCurrentCmd[i]) {
      redundantData = false;
      break;
    }
  }
  if (redundantData) return true;
  else {
    for (byte i = 0; i < dscReadSize; i++) dscPreviousCmd[i] = dscCurrentCmd[i];
    return false;
  }
}


// Called as an interrupt when the DSC clock changes to write data for virtual keypad and setup timers to read
// data after an interval.
void IRAM dscClockInterrupt(uint8_t dscIsrPin) {

  // Sets up a timer that will call dscDataInterrupt() in 250us to read the data line.
  // Data sent from the panel and keypads/modules has latency after a clock change (observed up to 160us for keypad data).
  timer_set_interrupts(FRC1, true);
  timer_set_run(FRC1, true);

  static unsigned long dscPreviousClockHighTime;
  if (digitalRead(dscClockPin) == HIGH) {
    if (dscVirtualKeypad) digitalWrite(dscWritePin, LOW);  // Restores the data line after a virtual keypad write
    dscPreviousClockHighTime = micros();
  }

  else {
    dscClockHighTime = micros() - dscPreviousClockHighTime;  // Tracks the clock high time to find the reset between commands

    // Virtual keypad
    if (dscVirtualKeypad) {

      static bool writeStart = false;
      static bool writeRepeat = false;
      static bool writeCmd = false;

      if (dscWritePartition <= 4 && dscStatusCmd == 0x05) writeCmd = true;
      else if (dscWritePartition > 4 && dscStatusCmd == 0x1B) writeCmd = true;
      else writeCmd = false;

      // Writes a F/A/P alarm key and repeats the key on the next immediate command from the panel (0x1C verification)
      if ((dscWriteAlarm && dscPanelKeyPending) || writeRepeat) {

        // Writes the first bit by shifting the alarm key data right 7 bits and checking bit 0
        if (dscIsrPanelBitTotal == 1) {
          if (!((dscPanelKey >> 7) & 0x01)) {
            digitalWrite(dscWritePin, HIGH);
          }
          writeStart = true;  // Resolves a timing issue where some writes do not begin at the correct bit
        }

        // Writes the remaining alarm key data
        else if (writeStart && dscIsrPanelBitTotal > 1 && dscIsrPanelBitTotal <= 8) {
          if (!((dscPanelKey >> (8 - dscIsrPanelBitTotal)) & 0x01)) digitalWrite(dscWritePin, HIGH);

          // Resets counters when the write is complete
          if (dscIsrPanelBitTotal == 8) {
            dscPanelKeyPending = false;
            writeStart = false;
            dscWriteAlarm = false;

            // Sets up a repeated write for alarm keys
            if (!writeRepeat) writeRepeat = true;
            else writeRepeat = false;
          }
        }
      }

      // Writes a regular key unless waiting for a response to the '*' key or the panel is sending a query command
      else if (dscPanelKeyPending && !dscWroteAsterisk && dscIsrPanelByteCount == dscWriteByte && writeCmd) {

        // Writes the first bit by shifting the key data right 7 bits and checking bit 0
        if (dscIsrPanelBitTotal == dscWriteBit) {
          if (!((dscPanelKey >> 7) & 0x01)) digitalWrite(dscWritePin, HIGH);
          writeStart = true;  // Resolves a timing issue where some writes do not begin at the correct bit
        }

        // Writes the remaining alarm key data
        else if (writeStart && dscIsrPanelBitTotal > dscWriteBit && dscIsrPanelBitTotal <= dscWriteBit + 7) {
          if (!((dscPanelKey >> (7 - dscIsrPanelBitCount)) & 0x01)) digitalWrite(dscWritePin, HIGH);

          // Resets counters when the write is complete
          if (dscIsrPanelBitTotal == dscWriteBit + 7) {
            if (dscWriteAsterisk) dscWroteAsterisk = true;  // Delays writing after pressing '*' until the panel is ready
            else dscPanelKeyPending = false;
            writeStart = false;
          }
        }
      }
    }
  }
}


// Timer interrupt called by dscClockInterrupt() after 250us to read the data line
// Data sent from the panel and keypads/modules has latency after a clock change (observed up to 160us for keypad data).
void IRAM dscDataInterrupt(void *arg) {

  // Stops the timer
  timer_set_run(FRC1, false);

  static bool skipData = false;

  // Panel sends data while the clock is high
  if (digitalRead(dscClockPin) == HIGH) {

    // Stops processing Keybus data at the dscReadSize limit
    if (dscIsrPanelByteCount >= dscReadSize) skipData = true;

    else {
      if (dscIsrPanelBitCount < 8) {

        // Data is captured in each byte by shifting left by 1 bit and writing to bit 0
        dscIsrPanelData[dscIsrPanelByteCount] <<= 1;
        if (digitalRead(dscReadPin) == HIGH) {
          dscIsrPanelData[dscIsrPanelByteCount] |= 1;
        }
      }

      if (dscIsrPanelBitTotal == 8) {

        // Tests for a status command, used in dscClockInterrupt() to ensure keys are only written during a status command
        switch (dscIsrPanelData[0]) {
          case 0x05:
          case 0x0A: dscStatusCmd = 0x05; break;
          case 0x1B: dscStatusCmd = 0x1B; break;
          default: dscStatusCmd = 0; break;
        }

        // Stores the stop bit by itself in byte 1 - this aligns the Keybus bytes with dscPanelData[] bytes
        dscIsrPanelBitCount = 0;
        dscIsrPanelByteCount++;
      }

      // Increments the bit counter if the byte is incomplete
      else if (dscIsrPanelBitCount < 7) {
        dscIsrPanelBitCount++;
      }

      // Byte is complete, set the counters for the next byte
      else {
        dscIsrPanelBitCount = 0;
        dscIsrPanelByteCount++;
      }

      dscIsrPanelBitTotal++;
    }
  }

  // Keypads and modules send data while the clock is low
  else {
    static bool moduleDataDetected = false;

    // Keypad and module data is not buffered and skipped if the panel data buffer is filling
    if (dscProcessModuleData && dscIsrModuleByteCount < dscReadSize && dscPanelBufferLength <= 1) {

      // Data is captured in each byte by shifting left by 1 bit and writing to bit 0
      if (dscIsrModuleBitCount < 8) {
        dscIsrModuleData[dscIsrModuleByteCount] <<= 1;
        if (digitalRead(dscReadPin) == HIGH) {
          dscIsrModuleData[dscIsrModuleByteCount] |= 1;
        }
        else moduleDataDetected = true;  // Keypads and modules send data by pulling the data line low
      }

      // Stores the stop bit by itself in byte 1 - this aligns the Keybus bytes with dscModuleData[] bytes
      if (dscIsrModuleBitTotal == 7) {
        dscIsrModuleData[1] = 1;  // Sets the stop bit manually to 1 in byte 1
        dscIsrModuleBitCount = 0;
        dscIsrModuleByteCount += 2;
      }

      // Increments the bit counter if the byte is incomplete
      else if (dscIsrModuleBitCount < 7) {
        dscIsrModuleBitCount++;
      }

      // Byte is complete, set the counters for the next byte
      else {
        dscIsrModuleBitCount = 0;
        dscIsrModuleByteCount++;
      }

      dscIsrModuleBitTotal++;
    }

    // Saves data and resets counters after the clock cycle is complete (high for at least 1ms)
    if (dscClockHighTime > 1000) {
      dscKeybusTime = millis();

      // Skips incomplete and redundant data from status commands - these are sent constantly on the keybus at a high
      // rate, so they are always skipped.  Checking is required in the ISR to prevent flooding the buffer.
      if (dscIsrPanelBitTotal < 8) skipData = true;
      else switch (dscIsrPanelData[0]) {
        static byte dscPreviousCmd05[dscReadSize];
        static byte dscPreviousCmd1B[dscReadSize];
        case 0x05:  // Status: partitions 1-4
          if (dscRedundantPanelData(dscPreviousCmd05, dscIsrPanelData, dscIsrPanelByteCount)) skipData = true;
          break;

        case 0x1B:  // Status: partitions 5-8
          if (dscRedundantPanelData(dscPreviousCmd1B, dscIsrPanelData, dscIsrPanelByteCount)) skipData = true;
          break;
      }

      // Stores new panel data in the panel buffer
      dscCurrentCmd = dscIsrPanelData[0];
      if (dscPanelBufferLength == dscBufferSize) dscBufferOverflow = true;
      else if (!skipData && dscPanelBufferLength < dscBufferSize) {
        for (byte i = 0; i < dscReadSize; i++) dscPanelBuffer[dscPanelBufferLength][i] = dscIsrPanelData[i];
        dscPanelBufferBitCount[dscPanelBufferLength] = dscIsrPanelBitTotal;
        dscPanelBufferByteCount[dscPanelBufferLength] = dscIsrPanelByteCount;
        dscPanelBufferLength++;
      }

      // Resets the panel capture data and counters
      for (byte i = 0; i < dscReadSize; i++) dscIsrPanelData[i] = 0;
      dscIsrPanelBitTotal = 0;
      dscIsrPanelBitCount = 0;
      dscIsrPanelByteCount = 0;
      skipData = false;

      if (dscProcessModuleData) {

        // Stores new keypad and module data - this data is not buffered
        if (moduleDataDetected) {
          moduleDataDetected = false;
          dscModuleDataCaptured = true;  // Sets a flag for dscHandleModule()
          for (byte i = 0; i < dscReadSize; i++) dscModuleData[i] = dscIsrModuleData[i];
          dscModuleBitCount = dscIsrModuleBitTotal;
          dscModuleByteCount = dscIsrModuleByteCount;
        }

        // Resets the keypad and module capture data and counters
        for (byte i = 0; i < dscReadSize; i++) dscIsrModuleData[i] = 0;
        dscIsrModuleBitTotal = 0;
        dscIsrModuleBitCount = 0;
        dscIsrModuleByteCount = 0;
      }

      // Notifies the dscPanelLoop task when new data is available
      if (dscPanelBufferLength > 0) {
        BaseType_t xHigherPriorityTaskWoken;
        vTaskNotifyGiveFromISR(dscPanelLoopHandle, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) portYIELD();
      }
      else if (dscModuleDataCaptured) xSemaphoreGive(dscDataAvailable);
    }
  }
}


void dscBegin() {

  // Settings
  if (dscWritePin == 255) dscVirtualKeypad = false;
  else dscVirtualKeypad = true;
  dscPanelKeyPending = false;
  dscWritePartition = 1;
  dscPauseStatus = false;

  // GPIO setup
  gpio_enable(dscClockPin, GPIO_INPUT);
  gpio_enable(dscReadPin, GPIO_INPUT);
  gpio_enable(dscWritePin, GPIO_OUTPUT);

  // Task setup
  dscDataAvailable = xSemaphoreCreateBinary();
  xTaskCreate(dscPanelLoop, "dscPanelLoop", 384, NULL, 1, NULL);

  printf("\ndscKeybusInterface is online.\n\n");
}


void IRAM dscPanelLoop() {

  // Timer interrupt setup
  timer_set_interrupts(FRC1, false);
  timer_set_run(FRC1, false);
  _xt_isr_attach(INUM_TIMER_FRC1, dscDataInterrupt, NULL);
  timer_set_timeout(FRC1, 250);
  timer_set_reload(FRC1, true);

  // Clock pin interrupt setup
  gpio_set_interrupt(dscClockPin, GPIO_INTTYPE_EDGE_ANY, dscClockInterrupt);

  // Sets this task to be notified by dscDataInterrupt() when new data is available
  dscPanelLoopHandle = xTaskGetCurrentTaskHandle();

  while(1) {

    // Waits until notification from dscDataInterrupt() that new data is available
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


    // Checks if Keybus data is detected and sets a status flag if data is not detected for 3s
    taskENTER_CRITICAL();
    if (millis() - dscKeybusTime > 3000) dscKeybusConnected = false;  // dataTime is set in dscDataInterrupt() when the clock resets
    else dscKeybusConnected = true;
    taskEXIT_CRITICAL();

    if (dscPreviousKeybus != dscKeybusConnected) {
      dscPreviousKeybus = dscKeybusConnected;
      dscKeybusChanged = true;
      if (!dscPauseStatus) dscStatusChanged = true;
      if (!dscKeybusConnected) continue;
    }

    // Copies data from the buffer to dscPanelData[]
    static byte dscPanelBufferIndex = 1;
    byte dataIndex = dscPanelBufferIndex - 1;
    for (byte i = 0; i < dscReadSize; i++) dscPanelData[i] = dscPanelBuffer[dataIndex][i];
    dscPanelBitCount = dscPanelBufferBitCount[dataIndex];
    dscPanelByteCount = dscPanelBufferByteCount[dataIndex];
    dscPanelBufferIndex++;

    // Resets counters when the buffer is cleared
    taskENTER_CRITICAL();
    if (dscPanelBufferIndex > dscPanelBufferLength) {
      dscPanelBufferIndex = 1;
      dscPanelBufferLength = 0;
    }
    taskEXIT_CRITICAL();

    // Waits at startup for the 0x05 status command or a command with valid CRC data to eliminate spurious data.
    static bool firstClockCycle = true;
    if (firstClockCycle) {
      if ((dscValidCRC() || dscPanelData[0] == 0x05) && dscPanelData[0] != 0) firstClockCycle = false;
      else continue;
    }

    // Skips redundant data sent constantly while in installer programming
    static byte dscPreviousCmd0A[dscReadSize];
    static byte dscPreviousCmdE6_20[dscReadSize];
    switch (dscPanelData[0]) {
      case 0x0A:  // Status in programming
        if (dscRedundantPanelData(dscPreviousCmd0A, dscPanelData, dscReadSize)) continue;
        break;

      case 0xE6:
        if (dscPanelData[2] == 0x20 && dscRedundantPanelData(dscPreviousCmdE6_20, dscPanelData, dscReadSize)) continue;  // Status in programming, zone lights 33-64
        break;
    }
    if (dscPartitions > 4) {
      static byte dscPreviousCmdE6_03[dscReadSize];
      if (dscPanelData[0] == 0xE6 && dscPanelData[2] == 0x03 && dscRedundantPanelData(dscPreviousCmdE6_03, dscPanelData, 8)) continue;  // Status in alarm/programming, partitions 5-8
    }

    // Skips redundant data from periodic commands sent at regular intervals, by default this data is processed
    if (!dscProcessRedundantData) {
      static byte dscPreviousCmd11[dscReadSize];
      static byte dscPreviousCmd16[dscReadSize];
      static byte dscPreviousCmd27[dscReadSize];
      static byte dscPreviousCmd2D[dscReadSize];
      static byte dscPreviousCmd34[dscReadSize];
      static byte dscPreviousCmd3E[dscReadSize];
      static byte dscPreviousCmd5D[dscReadSize];
      static byte dscPreviousCmd63[dscReadSize];
      static byte dscPreviousCmdB1[dscReadSize];
      static byte dscPreviousCmdC3[dscReadSize];
      switch (dscPanelData[0]) {
        case 0x11:  // Keypad slot query
          if (dscRedundantPanelData(dscPreviousCmd11, dscPanelData, dscReadSize)) continue;
          break;

        case 0x16:  // Zone wiring
          if (dscRedundantPanelData(dscPreviousCmd16, dscPanelData, dscReadSize)) continue;
          break;

        case 0x27:  // Status with zone 1-8 info
          if (dscRedundantPanelData(dscPreviousCmd27, dscPanelData, dscReadSize)) continue;
          break;

        case 0x2D:  // Status with zone 9-16 info
          if (dscRedundantPanelData(dscPreviousCmd2D, dscPanelData, dscReadSize)) continue;
          break;

        case 0x34:  // Status with zone 17-24 info
          if (dscRedundantPanelData(dscPreviousCmd34, dscPanelData, dscReadSize)) continue;
          break;

        case 0x3E:  // Status with zone 25-32 info
          if (dscRedundantPanelData(dscPreviousCmd3E, dscPanelData, dscReadSize)) continue;
          break;

        case 0x5D:  // Flash panel lights: status and zones 1-32
          if (dscRedundantPanelData(dscPreviousCmd5D, dscPanelData, dscReadSize)) continue;
          break;

        case 0x63:  // Flash panel lights: status and zones 33-64
          if (dscRedundantPanelData(dscPreviousCmd63, dscPanelData, dscReadSize)) continue;
          break;

        case 0xB1:  // Enabled zones 1-32
          if (dscRedundantPanelData(dscPreviousCmdB1, dscPanelData, dscReadSize)) continue;
          break;

        case 0xC3:  // Unknown command
          if (dscRedundantPanelData(dscPreviousCmdC3, dscPanelData, dscReadSize)) continue;
          break;
      }
    }

    // Processes valid panel data
    switch (dscPanelData[0]) {
      case 0x05:
      case 0x1B: dscProcessPanelStatus(); break;
      case 0x27: dscProcessPanel_0x27(); break;
      case 0x2D: dscProcessPanel_0x2D(); break;
      case 0x34: dscProcessPanel_0x34(); break;
      case 0x3E: dscProcessPanel_0x3E(); break;
      case 0xA5: dscProcessPanel_0xA5(); break;
      case 0xE6: if (dscPartitions > 2) dscProcessPanel_0xE6(); break;
      case 0xEB: if (dscPartitions > 2) dscProcessPanel_0xEB(); break;
    }

    dscPanelDataAvailable = true;
    xSemaphoreGive(dscDataAvailable);
  }
}


bool dscHandleModule() {
  if (!dscModuleDataCaptured) return false;
  dscModuleDataCaptured = false;

  if (dscModuleBitCount < 8) return false;

  // Skips periodic keypad slot query responses
  if (!dscProcessRedundantData) {
    bool redundantData = true;
    byte checkedBytes = dscReadSize;
    static byte dscPreviousSlotData[dscReadSize];
    for (byte i = 0; i < checkedBytes; i++) {
      if (dscPreviousSlotData[i] != dscModuleData[i]) {
        redundantData = false;
        break;
      }
    }
    if (redundantData) return false;
    else {
      for (byte i = 0; i < dscReadSize; i++) dscPreviousSlotData[i] = dscModuleData[i];
      return true;
    }
  }

  return true;
}


// Sets up writes for a single key
void dscWriteKey(int receivedKey) {
  while(dscPanelKeyPending || dscPanelKeysPending) vTaskDelay(1);
  dscSetWriteKey(receivedKey);
}


// Sets up writes for multiple keys sent as a char array
void dscWriteKeys(const char *receivedKeys) {
  while(dscPanelKeyPending || dscPanelKeysPending) vTaskDelay(1);
  dscPanelKeysArray = receivedKeys;
  if (dscPanelKeysArray[0] != '\0') dscPanelKeysPending = true;
  while (dscPanelKeysPending) {
    static byte writeCounter = 0;
    if (!dscPanelKeyPending && writeCounter < strlen(dscPanelKeysArray)) {
      if (dscPanelKeysArray[writeCounter] != '\0') {
        dscSetWriteKey(dscPanelKeysArray[writeCounter]);
        writeCounter++;
        if (dscPanelKeysArray[writeCounter] == '\0') {
          dscPanelKeysPending = false;
          writeCounter = 0;
        }
      }
    }
    else vTaskDelay(1);
  }
}


// Specifies the key value to be written by dscClockInterrupt() and selects the write partition.  This includes a 500ms
// delay after alarm keys to resolve errors when additional keys are sent immediately after alarm keys.
void dscSetWriteKey(int receivedKey) {
  static unsigned long previousTime;
  static bool setPartition;

  // Sets the write partition if set by virtual keypad key '/'
  if (setPartition) {
    setPartition = false;
    if (receivedKey >= '1' && receivedKey <= '8') {
      dscWritePartition = receivedKey - 48;
    }
    return;
  }

  // Sets the binary to write for virtual keypad keys
  if (!dscPanelKeyPending && millis() - previousTime > 500) {
    bool validKey = true;
    switch (receivedKey) {
      case '/': setPartition = true; validKey = false; break;
      case '0': dscPanelKey = 0x00; break;
      case '1': dscPanelKey = 0x05; break;
      case '2': dscPanelKey = 0x0A; break;
      case '3': dscPanelKey = 0x0F; break;
      case '4': dscPanelKey = 0x11; break;
      case '5': dscPanelKey = 0x16; break;
      case '6': dscPanelKey = 0x1B; break;
      case '7': dscPanelKey = 0x1C; break;
      case '8': dscPanelKey = 0x22; break;
      case '9': dscPanelKey = 0x27; break;
      case '*': dscPanelKey = 0x28; dscWriteAsterisk = true; break;
      case '#': dscPanelKey = 0x2D; break;
      case 'F':
      case 'f': dscPanelKey = 0x77; dscWriteAlarm = true; break;  // Keypad fire alarm
      case 's':
      case 'S': dscPanelKey = 0xAF; dscWriteArm[dscWritePartition - 1] = true; break;  // Arm stay
      case 'w':
      case 'W': dscPanelKey = 0xB1; dscWriteArm[dscWritePartition - 1] = true; break;  // Arm away
      case 'n':
      case 'N': dscPanelKey = 0xB6; dscWriteArm[dscWritePartition - 1] = true; break;  // Arm with no entry delay (night arm)
      case 'A':
      case 'a': dscPanelKey = 0xBB; dscWriteAlarm = true; break;  // Keypad auxiliary alarm
      case 'c':
      case 'C': dscPanelKey = 0xBB; break;                        // Door chime
      case 'r':
      case 'R': dscPanelKey = 0xDA; break;                        // Reset
      case 'P':
      case 'p': dscPanelKey = 0xDD; dscWriteAlarm = true; break;  // Keypad panic alarm
      case 'x':
      case 'X': dscPanelKey = 0xE1; break;                        // Exit
      case '[': dscPanelKey = 0xD5; break;                        // Command output 1
      case ']': dscPanelKey = 0xDA; break;                        // Command output 2
      case '{': dscPanelKey = 0x70; break;                        // Command output 3
      case '}': dscPanelKey = 0xEC; break;                        // Command output 4
      default: {
        validKey = false;
        break;
      }
    }

    // Sets the writing position in dscClockInterrupt() for the currently set partition
    if (dscPartitions < dscWritePartition) dscWritePartition = 1;
    switch (dscWritePartition) {
      case 1:
      case 5: {
        dscWriteByte = 2;
        dscWriteBit = 9;
        break;
      }
      case 2:
      case 6: {
        dscWriteByte = 3;
        dscWriteBit = 17;
        break;
      }
      case 3:
      case 7: {
        dscWriteByte = 8;
        dscWriteBit = 57;
        break;
      }
      case 4:
      case 8: {
        dscWriteByte = 9;
        dscWriteBit = 65;
        break;
      }
      default: {
        dscWriteByte = 2;
        dscWriteBit = 9;
        break;
      }
    }

    if (dscWriteAlarm) previousTime = millis();  // Sets a marker to time writes after keypad alarm keys
    if (validKey) dscPanelKeyPending = true;     // Sets a flag indicating that a write is pending, cleared by dscClockInterrupt()
  }
}
