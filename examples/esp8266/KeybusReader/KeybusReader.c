/*
 *  DSC-RTOS Keybus Reader 1.0 (esp8266)
 *
 *  Decodes and prints data from the Keybus to a serial interface, including reading from serial for the virtual
 *  keypad.  This is primarily to help decode the Keybus protocol - see the Status example to put the interface
 *  to productive use.
 *
 *  Usage (macOS/Linux):
 *    1. Edit dscSettings.h to configure the GPIO pins
 *
 *    2. Edit Makefile to set the esp8266 serial port, baud rate, and flash size.
 *
 *    3. Build the example and flash the esp8266:
 *         $ make flash
 *       (If a previous flash causes issues, erase before flashing: make erase_flash)
 *
 *    4. Open the serial interface (close with CTRL-a, k):
 *         $ screen /dev/tty.wchusbserial410 115200
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

    // If the Keybus data buffer is exceeded, the program is too busy to process all Keybus commands.  Call
    // dscLoop() more often, or increase dscBufferSize in the library: src/dscKeybusInterface-RTOS.h
    if (dscBufferOverflow) {
      printf("Keybus buffer overflow\n");
      dscBufferOverflow = false;
    }

    if (dscPanelDataAvailable) {
      dscPanelDataAvailable = false;

      // Prints panel data
      printf("%8.2f: ", millis() / 1000.0);  // Prints a timestamp
      dscPrintPanelBinary(true);             // Optionally prints without spaces: printPanelBinary(false);
      printf(" [");
      dscPrintPanelCommand();                // Prints the panel command as hex
      printf("] ");
      dscPrintPanelMessage();                // Prints the decoded message
      printf("\n");
    }

    // Prints keypad and module data
    if (dscHandleModule()) {
      printf("%8.2f: ", millis() / 1000.0);  // Prints a timestamp
      dscPrintModuleBinary(true);            // Optionally prints without spaces: printKeybusBinary(false);
      printf(" ");
      dscPrintModuleMessage();               // Prints the decoded message
      printf("\n");
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
