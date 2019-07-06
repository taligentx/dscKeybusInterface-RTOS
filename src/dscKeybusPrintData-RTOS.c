/*
    DSC Keybus Interface

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

/*
 *  Print messages
 */


void dscPrintPanelMessage() {
  switch (dscPanelData[0]) {
    case 0x05: dscPrintPanel_0x05(); return;  // Panel status: partitions 1-4
    case 0x0A: dscPrintPanel_0x0A(); return;  // Panel status in alarm/programming, partitions 1-4
    case 0x11: dscPrintPanel_0x11(); return;  // Keypad slot query
    case 0x16: dscPrintPanel_0x16(); return;  // Zone wiring
    case 0x1B: dscPrintPanel_0x1B(); return;  // Panel status: partitions 5-8
    case 0x1C: dscPrintPanel_0x1C(); return;  // Verify keypad Fire/Auxiliary/Panic
    case 0x27: dscPrintPanel_0x27(); return;  // Panel status and zones 1-8 status
    case 0x28: dscPrintPanel_0x28(); return;  // Zone expander query
    case 0x2D: dscPrintPanel_0x2D(); return;  // Panel status and zones 9-16 status
    case 0x34: dscPrintPanel_0x34(); return;  // Panel status and zones 17-24 status
    case 0x3E: dscPrintPanel_0x3E(); return;  // Panel status and zones 25-32 status
    case 0x4C: dscPrintPanel_0x4C(); return;  // Unknown Keybus query
    case 0x58: dscPrintPanel_0x58(); return;  // Unknown Keybus query
    case 0x5D: dscPrintPanel_0x5D(); return;  // Flash panel lights: status and zones 1-32, partition 1
    case 0x63: dscPrintPanel_0x63(); return;  // Flash panel lights: status and zones 1-32, partition 2
    case 0x64: dscPrintPanel_0x64(); return;  // Beep - one-time, partition 1
    case 0x69: dscPrintPanel_0x69(); return;  // Beep - one-time, partition 2
    case 0x75: dscPrintPanel_0x75(); return;  // Beep pattern - repeated, partition 1
    case 0x7A: dscPrintPanel_0x7A(); return;  // Beep pattern - repeated, partition 2
    case 0x7F: dscPrintPanel_0x7F(); return;  // Beep - one-time long beep, partition 1
    case 0x82: dscPrintPanel_0x82(); return;  // Beep - one-time long beep, partition 1
    case 0x87: dscPrintPanel_0x87(); return;  // Panel outputs
    case 0x8D: dscPrintPanel_0x8D(); return;  // User code programming key response, codes 17-32
    case 0x94: dscPrintPanel_0x94(); return;  // Unknown - immediate after entering *5 programming
    case 0xA5: dscPrintPanel_0xA5(); return;  // Date, time, system status messages - partitions 1-2
    case 0xB1: dscPrintPanel_0xB1(); return;  // Enabled zones 1-32
    case 0xBB: dscPrintPanel_0xBB(); return;  // Bell
    case 0xC3: dscPrintPanel_0xC3(); return;  // Keypad status
    case 0xCE: dscPrintPanel_0xCE(); return;  // Unknown command
    case 0xD5: dscPrintPanel_0xD5(); return;  // Keypad zone query
    case 0xE6: dscPrintPanel_0xE6(); return;  // Extended status commands: partitions 3-8, zones 33-64
    case 0xEB: dscPrintPanel_0xEB(); return;  // Date, time, system status messages - partitions 1-8
    default: {
      printf("Unrecognized data");
      if (!dscValidCRC()) {
        printf("[No CRC or CRC Error]");
        return;
      }
      else printf("[CRC OK]");
      return;
    }
  }
}


void dscPrintModuleMessage() {
  switch (dscModuleData[0]) {
    case 0x77: dscPrintModule_0x77(); return;  // Keypad fire alarm
    case 0xBB: dscPrintModule_0xBB(); return;  // Keypad auxiliary alarm
    case 0xDD: dscPrintModule_0xDD(); return;  // Keypad panic alarm
  }

  // Keypad and module responses to panel queries
  switch (dscCurrentCmd) {
    case 0x11: dscPrintModule_Panel_0x11(); return;  // Keypad slot query response
    case 0xD5: dscPrintModule_Panel_0xD5(); return;  // Keypad zone query response
  }

  // Keypad and module status update notifications
  if (dscModuleData[4] != 0xFF || dscModuleData[5] != 0xFF) {
    dscPrintModule_Notification();
    return;
  }

  // Keypad keys
  dscPrintModule_Keys();
}


/*
 *  Print panel messages
 */


 // Keypad lights for commands 0x05, 0x0A, 0x1B, 0x27, 0x2D, 0x34, 0x3E, 0x5D
 void dscPrintPanelLights(byte dscPanelByte) {
  if (dscPanelData[dscPanelByte] == 0) printf("none ");
  else {
    if (bitRead(dscPanelData[dscPanelByte],0)) printf("Ready ");
    if (bitRead(dscPanelData[dscPanelByte],1)) printf("Armed ");
    if (bitRead(dscPanelData[dscPanelByte],2)) printf("Memory ");
    if (bitRead(dscPanelData[dscPanelByte],3)) printf("Bypass ");
    if (bitRead(dscPanelData[dscPanelByte],4)) printf("Trouble ");
    if (bitRead(dscPanelData[dscPanelByte],5)) printf("Program ");
    if (bitRead(dscPanelData[dscPanelByte],6)) printf("Fire ");
    if (bitRead(dscPanelData[dscPanelByte],7)) printf("Backlight ");
  }
 }


// Messages for commands 0x05, 0x0A, 0x1B, 0x27, 0x2D, 0x34, 0x3E
void dscPrintPanelMessages(byte dscPanelByte) {
  switch (dscPanelData[dscPanelByte]) {
    case 0x01: printf("Partition ready"); break;
    case 0x02: printf("Stay/away zones open"); break;
    case 0x03: printf("Zones open"); break;
    case 0x04: printf("Armed stay"); break;
    case 0x05: printf("Armed away"); break;
    case 0x07: printf("Failed to arm"); break;
    case 0x08: printf("Exit delay in progress"); break;
    case 0x09: printf("Arming with no entry delay"); break;
    case 0x0B: printf("Quick exit in progress"); break;
    case 0x0C: printf("Entry delay in progress"); break;
    case 0x0D: printf("Opening after alarm"); break;
    case 0x10: printf("Keypad lockout"); break;
    case 0x11: printf("Partition in alarm"); break;
    case 0x14: printf("Auto-arm in progress"); break;
    case 0x15: printf("Arming with bypassed zones"); break;
    case 0x16: printf("Armed with no entry delay"); break;
    case 0x22: printf("Recent closing"); break;
    case 0x33: printf("Command output in progress"); break;
    case 0x3D: printf("Disarmed after alarm in memory"); break;
    case 0x3E: printf("Partition disarmed"); break;
    case 0x40: printf("Keypad blanked"); break;
    case 0x8A: printf("Activate stay/away zones"); break;
    case 0x8B: printf("Quick exit"); break;
    case 0x8E: printf("Invalid option"); break;
    case 0x8F: printf("Invalid access code"); break;
    case 0x9E: printf("Enter * function code"); break;
    case 0x9F: printf("Enter access code"); break;
    case 0xA0: printf("*1: Zone bypass programming"); break;
    case 0xA1: printf("*2: Trouble menu"); break;
    case 0xA2: printf("*3: Alarm memory display"); break;
    case 0xA3: printf("Door chime enabled"); break;
    case 0xA4: printf("Door chime disabled"); break;
    case 0xA5: printf("Enter master code"); break;
    case 0xA6: printf("*5: Access codes"); break;
    case 0xA7: printf("*5: Enter new code"); break;
    case 0xA9: printf("*6: User functions"); break;
    case 0xAA: printf("*6: Time and Date"); break;
    case 0xAB: printf("*6: Auto-arm time"); break;
    case 0xAC: printf("*6: Auto-arm enabled"); break;
    case 0xAD: printf("*6: Auto-arm disabled"); break;
    case 0xAF: printf("*6: System test"); break;
    case 0xB0: printf("*6: Enable DLS"); break;
    case 0xB2: printf("*7: Command output"); break;
    case 0xB7: printf("Enter installer code"); break;
    case 0xB8: printf("*  pressed while armed"); break;
    case 0xB9: printf("*2: Zone tamper menu"); break;
    case 0xBA: printf("*2: Zones with low batteries"); break;
    case 0xC6: printf("*2: Zone fault menu"); break;
    case 0xC8: printf("*2: Service required menu"); break;
    case 0xD0: printf("*2: Handheld keypads with low batteries"); break;
    case 0xD1: printf("*2: Wireless keys with low batteries"); break;
    case 0xE4: printf("*8: Main menu"); break;
    case 0xE5: printf("Keypad slot assignment"); break;
    case 0xE6: printf("*8: Input required: 2 digits"); break;
    case 0xE7: printf("*8: Input required: 3 digits"); break;
    case 0xE8: printf("*8: Input required: 4 digits"); break;
    case 0xEA: printf("*8: Reporting code: 2 digits"); break;
    case 0xEB: printf("*8: Telephone number account code: 4 digits"); break;
    case 0xEC: printf("*8: Input required: 6 digits"); break;
    case 0xED: printf("*8: Input required: 32 digits"); break;
    case 0xEE: printf("*8: Input required: 1 option per zone"); break;
    case 0xF0: printf("Function key 1 programming"); break;
    case 0xF1: printf("Function key 2 programming"); break;
    case 0xF2: printf("Function key 3 programming"); break;
    case 0xF3: printf("Function key 4 programming"); break;
    case 0xF4: printf("Function key 5 programming"); break;
    case 0xF8: printf("Keypad programming"); break;
    default:
      printf("Unrecognized data: 0x%02X", dscPanelData[dscPanelByte]);
      break;
  }
}


// Status messages for commands 0xA5, 0xEB
void dscPrintPanelStatus0(byte dscPanelByte) {
  bool decoded = true;
  switch (dscPanelData[dscPanelByte]) {
    /*
     *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
     *  10100101 0 00011000 01001111 10110000 11101100 01001001 11111111 11110000 [0xA5] 03/29/2018 16:59 | Duress alarm
     *  10100101 0 00011000 01001111 11001110 10111100 01001010 11111111 11011111 [0xA5] 03/30/2018 14:47 | Disarmed after alarm in memory
     *  10100101 0 00011000 01001111 11001010 01000100 01001011 11111111 01100100 [0xA5] 03/30/2018 10:17 | Partition in alarm
     *  10100101 0 00011000 01010000 01001001 10111000 01001100 11111111 01011001 [0xA5] 04/02/2018 09:46 | Zone expander supervisory alarm
     *  10100101 0 00011000 01010000 01001010 00000000 01001101 11111111 10100011 [0xA5] 04/02/2018 10:00 | Zone expander supervisory restored
     *  10100101 0 00011000 01001111 01110010 10011100 01001110 11111111 01100111 [0xA5] 03/27/2018 18:39 | Keypad Fire alarm
     *  10100101 0 00011000 01001111 01110010 10010000 01001111 11111111 01011100 [0xA5] 03/27/2018 18:36 | Keypad Aux alarm
     *  10100101 0 00011000 01001111 01110010 10001000 01010000 11111111 01010101 [0xA5] 03/27/2018 18:34 | Keypad Panic alarm
     *  10100101 0 00010001 01101101 01100000 00000100 01010001 11111111 11010111 [0xA5] 11/11/2011 00:01 | Keypad status check?   // Power-on +124s, keypad sends status update immediately after this
     *  10100101 0 00011000 01001111 01110010 10011100 01010010 11111111 01101011 [0xA5] 03/27/2018 18:39 | Keypad Fire alarm restored
     *  10100101 0 00011000 01001111 01110010 10010000 01010011 11111111 01100000 [0xA5] 03/27/2018 18:36 | Keypad Aux alarm restored
     *  10100101 0 00011000 01001111 01110010 10001000 01010100 11111111 01011001 [0xA5] 03/27/2018 18:34 | Keypad Panic alarm restored
     *  10100101 0 00011000 01001111 11110110 00110100 10011000 11111111 11001101 [0xA5] 03/31/2018 22:13 | Keypad lockout
     *  10100101 0 00011000 01001111 11101011 10100100 10111110 11111111 01011000 [0xA5] 03/31/2018 11:41 | Armed partial: Zones bypassed
     *  10100101 0 00011000 01001111 11101011 00011000 10111111 11111111 11001101 [0xA5] 03/31/2018 11:06 | Armed special: quick-arm/auto-arm/keyswitch/wireless key/DLS
     *  10100101 0 00010001 01101101 01100000 00101000 11100101 11111111 10001111 [0xA5] 11/11/2011 00:10 | Auto-arm cancelled
     *  10100101 0 00011000 01001111 11110111 01000000 11100110 11111111 00101000 [0xA5] 03/31/2018 23:16 | Disarmed special: keyswitch/wireless key/DLS
     *  10100101 0 00011000 01001111 01101111 01011100 11100111 11111111 10111101 [0xA5] 03/27/2018 15:23 | Panel battery trouble
     *  10100101 0 00011000 01001111 10110011 10011000 11101000 11111111 00111110 [0xA5] 03/29/2018 19:38 | AC power failure  // Sent after delay in *8 [370]
     *  10100101 0 00011000 01001111 01110100 01010000 11101001 11111111 10111000 [0xA5] 03/27/2018 20:20 | Bell trouble
     *  10100101 0 00011000 01001111 11000000 10001000 11101100 11111111 00111111 [0xA5] 03/30/2018 00:34 | Telephone line trouble
     *  10100101 0 00011000 01001111 01101111 01110000 11101111 11111111 11011001 [0xA5] 03/27/2018 15:28 | Panel battery restored
     *  10100101 0 00011000 01010000 00100000 01011000 11110000 11111111 01110100 [0xA5] 04/01/2018 00:22 | AC power restored  // Sent after delay in *8 [370]
     *  10100101 0 00011000 01001111 01110100 01011000 11110001 11111111 11001000 [0xA5] 03/27/2018 20:22 | Bell restored
     *  10100101 0 00011000 01001111 11000000 10001000 11110100 11111111 01000111 [0xA5] 03/30/2018 00:34 | Telephone line restored
     *  10100101 0 00011000 01001111 11100001 01011000 11111111 11111111 01000011 [0xA5] 03/31/2018 01:22 | System test
     */
    // 0x09 - 0x28: Zone alarm, zones 1-32
    // 0x29 - 0x48: Zone alarm restored, zones 1-32
    case 0x49: printf("Duress alarm"); break;
    case 0x4A: printf("Disarmed after alarm in memory"); break;
    case 0x4B: printf("Partition in alarm"); break;
    case 0x4C: printf("Zone expander supervisory alarm"); break;
    case 0x4D: printf("Zone expander supervisory restored"); break;
    case 0x4E: printf("Keypad Fire alarm"); break;
    case 0x4F: printf("Keypad Aux alarm"); break;
    case 0x50: printf("Keypad Panic alarm"); break;
    case 0x51: printf("Auxiliary input alarm"); break;
    case 0x52: printf("Keypad Fire alarm restored"); break;
    case 0x53: printf("Keypad Aux alarm restored"); break;
    case 0x54: printf("Keypad Panic alarm restored"); break;
    case 0x55: printf("Auxilary input alarm restored"); break;
    // 0x56 - 0x75: Zone tamper, zones 1-32
    // 0x76 - 0x95: Zone tamper restored, zones 1-32
    case 0x98: printf("Keypad lockout"); break;
    // 0x99 - 0xBD: Armed by access code
    case 0xBE: printf("Armed partial: Zones bypassed"); break;
    case 0xBF: printf("Armed special: quick-arm/auto-arm/keyswitch/wireless key/DLS"); break;
    // 0xC0 - 0xE4: Disarmed by access code
    case 0xE5: printf("Auto-arm cancelled"); break;
    case 0xE6: printf("Disarmed special: keyswitch/wireless key/DLS"); break;
    case 0xE7: printf("Panel battery trouble"); break;
    case 0xE8: printf("Panel AC power failure"); break;
    case 0xE9: printf("Bell trouble"); break;
    case 0xEA: printf("Power on +16s"); break;
    case 0xEC: printf("Telephone line trouble"); break;
    case 0xEF: printf("Panel battery restored"); break;
    case 0xF0: printf("Panel AC power restored"); break;
    case 0xF1: printf("Bell restored"); break;
    case 0xF4: printf("Telephone line restored"); break;
    case 0xFF: printf("System test"); break;
    default: decoded = false;
  }
  if (decoded) return;

  /*
   *  Zone alarm, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 01001111 01001001 11011000 00001001 11111111 00110101 [0xA5] 03/26/2018 09:54 | Zone alarm: 1
   *  10100101 0 00011000 01001111 01001010 00100000 00001110 11111111 10000011 [0xA5] 03/26/2018 10:08 | Zone alarm: 6
   *  10100101 0 00011000 01001111 10010100 11001000 00010000 11111111 01110111 [0xA5] 03/28/2018 20:50 | Zone alarm: 8
   */
  if (dscPanelData[dscPanelByte] >= 0x09 && dscPanelData[dscPanelByte] <= 0x28) {
    printf("Zone alarm: %d", dscPanelData[dscPanelByte] - 0x08);
    return;
  }

  /*
   *  Zone alarm restored, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 01001111 10010100 11001100 00101001 11111111 10010100 [0xA5] 03/28/2018 20:51 | Zone alarm restored: 1
   *  10100101 0 00011000 01001111 10010100 11010100 00101110 11111111 10100001 [0xA5] 03/28/2018 20:53 | Zone alarm restored: 6
   *  10100101 0 00011000 01001111 10010100 11010000 00110000 11111111 10011111 [0xA5] 03/28/2018 20:52 | Zone alarm restored: 8
   */
  if (dscPanelData[dscPanelByte] >= 0x29 && dscPanelData[dscPanelByte] <= 0x48) {
    printf("Zone alarm restored: %d", dscPanelData[dscPanelByte] - 0x28);
    return;
  }

  /*
   *  Zone tamper, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00000001 01000100 00100010 01011100 01010110 11111111 10111101 [0xA5] 01/01/2001 02:23 | Zone tamper: 1
   *  10100101 0 00000001 01000100 00100010 01011100 01010111 11111111 10111101 [0xA5] 01/01/2001 02:23 | Zone tamper: 2
   *  10100101 0 00010001 01101101 01101011 10010000 01011011 11111111 01111000 [0xA5] 11/11/2011 11:36 | Zone tamper: 6
   */
  if (dscPanelData[dscPanelByte] >= 0x56 && dscPanelData[dscPanelByte] <= 0x75) {
    printf("Zone tamper: %d", dscPanelData[6] - 0x55);
    return;
  }

  /*
   *  Zone tamper restored, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00000001 01000100 00100010 01011100 01110110 11111111 11011101 [0xA5] 01/01/2001 02:23 | Zone tamper restored: 1
   *  10100101 0 00000001 01000100 00100010 01011100 01111000 11111111 11011101 [0xA5] 01/01/2001 02:23 | Zone tamper restored: 2
   *  10100101 0 00010001 01101101 01101011 10010000 01111011 11111111 10011000 [0xA5] 11/11/2011 11:36 | Zone tamper restored: 6
   */
  if (dscPanelData[dscPanelByte] >= 0x76 && dscPanelData[dscPanelByte] <= 0x95) {
    printf("Zone tamper restored: %d", dscPanelData[dscPanelByte] - 0x75);
    return;
  }

  /*
   *  Armed by access code
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 01001101 00001000 10010000 10011001 11111111 00111010 [0xA5] 03/08/2018 08:36 | Armed by user code 1
   *  10100101 0 00011000 01001101 00001000 10111100 10111011 11111111 10001000 [0xA5] 03/08/2018 08:47 | Armed by master code 40
   */
  if (dscPanelData[dscPanelByte] >= 0x99 && dscPanelData[dscPanelByte] <= 0xBD) {
    byte dscCode = dscPanelData[dscPanelByte] - 0x98;
    if (dscCode >= 35) dscCode += 5;
    printf("Armed by ");
    switch (dscCode) {
      case 33: printf("duress "); break;
      case 34: printf("duress "); break;
      case 40: printf("master "); break;
      case 41: printf("supervisor "); break;
      case 42: printf("supervisor "); break;
      default: printf("user "); break;
    }
    printf("code %d", dscCode);
    return;
  }

  /*
   *  Disarmed by access code
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 01001101 00001000 11101100 11000000 11111111 10111101 [0xA5] 03/08/2018 08:59 | Disarmed by user code 1
   *  10100101 0 00011000 01001101 00001000 10110100 11100010 11111111 10100111 [0xA5] 03/08/2018 08:45 | Disarmed by master code 40
   */
  if (dscPanelData[dscPanelByte] >= 0xC0 && dscPanelData[dscPanelByte] <= 0xE4) {
    byte dscCode = dscPanelData[dscPanelByte] - 0xBF;
    if (dscCode >= 35) dscCode += 5;
    printf("Disarmed by ");
    switch (dscCode) {
      case 33: printf("duress "); break;
      case 34: printf("duress "); break;
      case 40: printf("master "); break;
      case 41: printf("supervisor "); break;
      case 42: printf("supervisor "); break;
      default: printf("user "); break;
    }
    printf("code %d", dscCode);
    return;
  }

  printf("Unrecognized data");
}


// Status messages for commands 0xA5, 0xEB
void dscPrintPanelStatus1(byte dscPanelByte) {
  switch (dscPanelData[dscPanelByte]) {
    /*
     *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
     *  10100101 0 00011000 01001111 11001010 10001001 00000011 11111111 01100001 [0xA5] 03/30/2018 10:34 | Cross zone alarm
     *  10100101 0 00010001 01101101 01101010 00000001 00000100 11111111 10010001 [0xA5] 11/11/2011 10:00 | Delinquency alarm
     *  10100101 0 00010001 01101101 01100000 10101001 00100100 00000000 01010000 [0xA5] 11/11/2011 00:42 | Auto-arm cancelled by duress code 33
     *  10100101 0 00010001 01101101 01100000 10110101 00100101 00000000 01011101 [0xA5] 11/11/2011 00:45 | Auto-arm cancelled by duress code 34
     *  10100101 0 00010001 01101101 01100000 00101001 00100110 00000000 11010010 [0xA5] 11/11/2011 00:10 | Auto-arm cancelled by master code 40
     *  10100101 0 00010001 01101101 01100000 10010001 00100111 00000000 00111011 [0xA5] 11/11/2011 00:36 | Auto-arm cancelled by supervisor code 41
     *  10100101 0 00010001 01101101 01100000 10111001 00101000 00000000 01100100 [0xA5] 11/11/2011 00:46 | Auto-arm cancelled by supervisor code 42
     *  10100101 0 00011000 01001111 10100000 10011101 00101011 00000000 01110100 [0xA5] 03/29/2018 00:39 | Armed by auto-arm
     *  10100101 0 00011000 01001101 00001010 00001101 10101100 00000000 11001101 [0xA5] 03/08/2018 10:03 | Exit *8 programming
     *  10100101 0 00011000 01001101 00001001 11100001 10101101 00000000 10100001 [0xA5] 03/08/2018 09:56 | Enter *8
     *  10100101 0 00010001 01101101 01100010 11001101 11010000 00000000 00100010 [0xA5] 11/11/2011 02:51 | Command output 4
     *  10100101 0 00010110 01010110 00101011 11010001 11010010 00000000 11011111 [0xA5] 2016.05.17 11:52 | Armed with no entry delay cancelled
     */
    case 0x03: printf("Cross zone alarm"); return;
    case 0x04: printf("Delinquency alarm"); return;
    case 0x24: printf("Auto-arm cancelled by duress code 33"); return;
    case 0x25: printf("Auto-arm cancelled by duress code 34"); return;
    case 0x26: printf("Auto-arm cancelled by master code 40"); return;
    case 0x27: printf("Auto-arm cancelled by supervisor code 41"); return;
    case 0x28: printf("Auto-arm cancelled by supervisor code 42"); return;
    case 0x2B: printf("Armed by auto-arm"); return;
    // 0x6C - 0x8B: Zone fault restored, zones 1-32
    // 0x8C - 0xAB: Zone fault, zones 1-32
    case 0xAC: printf("Exit *8 programming"); return;
    case 0xAD: printf("Enter *8 programming"); return;
    // 0xB0 - 0xCF: Zones bypassed, zones 1-32
    case 0xD0: printf("Command output 4"); return;
    case 0xD2: printf("Armed with no entry delay cancelled"); return;
  }

  /*
   *  Zone fault restored, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00010001 01101101 01101011 01000001 01101100 11111111 00111010 [0xA5] 11/11/2011 11:16 | Zone fault restored: 1
   *  10100101 0 00010001 01101101 01101011 01010101 01101101 11111111 01001111 [0xA5] 11/11/2011 11:21 | Zone fault restored: 2
   *  10100101 0 00010001 01101101 01101011 10000101 01101111 11111111 10000001 [0xA5] 11/11/2011 11:33 | Zone fault restored: 4
   *  10100101 0 00010001 01101101 01101011 10001001 01110000 11111111 10000110 [0xA5] 11/11/2011 11:34 | Zone fault restored: 5
   */
  if (dscPanelData[dscPanelByte] >= 0x6C && dscPanelData[dscPanelByte] <= 0x8B) {
    printf("Zone fault restored: %d", dscPanelData[dscPanelByte] - 0x6B);
    return;
  }

  /*
   *  Zone fault, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00010001 01101101 01101011 00111101 10001100 11111111 01010110 [0xA5] 11/11/2011 11:15 | Zone fault: 1
   *  10100101 0 00010001 01101101 01101011 01010101 10001101 11111111 01101111 [0xA5] 11/11/2011 11:21 | Zone fault: 2
   *  10100101 0 00010001 01101101 01101011 10000001 10001111 11111111 10011101 [0xA5] 11/11/2011 11:32 | Zone fault: 3
   *  10100101 0 00010001 01101101 01101011 10001001 10010000 11111111 10100110 [0xA5] 11/11/2011 11:34 | Zone fault: 4
   */
  if (dscPanelData[dscPanelByte] >= 0x8C && dscPanelData[dscPanelByte] <= 0xAB) {
    printf("Zone fault: %d", dscPanelData[dscPanelByte] - 0x8B);
    return;
  }

  /*
   *  Zones bypassed, zones 1-32
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 01001111 10110001 10101001 10110001 00000000 00010111 [0xA5] 03/29/2018 17:42 | Zone bypassed: 2
   *  10100101 0 00011000 01001111 10110001 11000001 10110101 00000000 00110011 [0xA5] 03/29/2018 17:48 | Zone bypassed: 6
   */
  if (dscPanelData[dscPanelByte] >= 0xB0 && dscPanelData[dscPanelByte] <= 0xCF) {
    printf("Zone bypassed: %d", dscPanelData[dscPanelByte] - 0xAF);
    return;
  }

  printf("Unrecognized data");
}


// Status messages for commands 0xA5, 0xEB
void dscPrintPanelStatus2(byte dscPanelByte) {
  switch (dscPanelData[dscPanelByte]) {

    /*
     *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
     *  10100101 0 00011000 01001111 10101111 10000110 00101010 00000000 01101011 [0xA5] 03/29/2018 15:33 | Quick exit
     *  10100101 0 00010001 01101101 01110101 00111010 01100011 00000000 00110101 [0xA5] 11/11/2011 21:14 | Keybus fault restored
     *  10100101 0 00011000 01001111 11110111 01110110 01100110 00000000 11011111 [0xA5] 03/31/2018 23:29 | Enter *1 zone bypass programming
     *  10100101 0 00010001 01101101 01100010 11001110 01101001 00000000 10111100 [0xA5] 11/11/2011 02:51 | Command output 3
     *  10100101 0 00011000 01010000 01000000 00000010 10001100 00000000 11011011 [0xA5] 04/02/2018 00:00 | Loss of system time
     *  10100101 0 00011000 01001111 10101110 00001110 10001101 00000000 01010101 [0xA5] 03/29/2018 14:03 | Power on
     *  10100101 0 00011000 01010000 01000000 00000010 10001110 00000000 11011101 [0xA5] 04/02/2018 00:00 | Panel factory default
     *  10100101 0 00011000 01001111 11101010 10111010 10010011 00000000 01000011 [0xA5] 03/31/2018 10:46 | Disarmed by keyswitch
     *  10100101 0 00011000 01001111 11101010 10101110 10010110 00000000 00111010 [0xA5] 03/31/2018 10:43 | Armed by keyswitch
     *  10100101 0 00011000 01001111 10100000 01100010 10011000 00000000 10100110 [0xA5] 03/29/2018 00:24 | Armed by quick-arm
     *  10100101 0 00010001 01101101 01100000 00101110 10011001 00000000 01001010 [0xA5] 11/11/2011 00:11 | Activate stay/away zones
     *  10100101 0 00011000 01001111 00101101 00011010 10011010 00000000 11101101 [0xA5] 03/25/2018 13:06 | Armed: stay
     *  10100101 0 00011000 01001111 00101101 00010010 10011011 00000000 11100110 [0xA5] 03/25/2018 13:04 | Armed: away
     *  10100101 0 00011000 01001111 00101101 10011010 10011100 00000000 01101111 [0xA5] 03/25/2018 13:38 | Armed with no entry delay
     *  10100101 0 00011000 01001111 00101100 11011110 11000011 00000000 11011001 [0xA5] 03/25/2018 12:55 | Enter *5 programming
     *  10100101 0 00011000 01001111 00101110 00000010 11100110 00000000 00100010 [0xA5] 03/25/2018 14:00 | Enter *6 programming
     */
    case 0x2A: printf("Quick exit"); return;
    case 0x63: printf("Keybus fault restored"); return;
    case 0x66: printf("Enter *1 zone bypass programming"); return;
    case 0x67: printf("Command output 1"); return;
    case 0x68: printf("Command output 2"); return;
    case 0x69: printf("Command output 3"); return;
    case 0x8C: printf("Loss of system time"); return;
    case 0x8D: printf("Power on"); return;
    case 0x8E: printf("Panel factory default"); return;
    case 0x93: printf("Disarmed by keyswitch"); return;
    case 0x96: printf("Armed by keyswitch"); return;
    case 0x97: printf("Armed by keypad away"); return;
    case 0x98: printf("Armed by quick-arm"); return;
    case 0x99: printf("Activate stay/away zones"); return;
    case 0x9A: printf("Armed: stay"); return;
    case 0x9B: printf("Armed: away"); return;
    case 0x9C: printf("Armed with no entry delay"); return;
    case 0xC3: printf("Enter *5 programming"); return;
    // 0xC6 - 0xE5: Auto-arm cancelled by user code
    case 0xE6: printf("Enter *6 programming"); return;
    // 0xE9 - 0xF0: Supervisory restored, keypad slots 1-8
    // 0xF1 - 0xF8: Supervisory trouble, keypad slots 1-8
  }

  /*
   *  Auto-arm cancelled by user code
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00010001 01101101 01100000 00111110 11000110 00000000 10000111 [0xA5] 11/11/2011 00:15 | Auto-arm cancelled by user code 1
   *  10100101 0 00010001 01101101 01100000 01111010 11100101 00000000 11100010 [0xA5] 11/11/2011 00:30 | Auto-arm cancelled by user code 32
   */
  if (dscPanelData[dscPanelByte] >= 0xC6 && dscPanelData[dscPanelByte] <= 0xE5) {
    printf("Auto-arm cancelled by user code %d", dscPanelData[dscPanelByte] - 0xC5);
    return;
  }

  /*
   *  Supervisory restored, keypad slots 1-8
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00010001 01101101 01110100 10001110 11101001 11111111 00001101 [0xA5] 11/11/2011 20:35 | Supervisory - module detected: Keypad slot 1
   *  10100101 0 00010001 01101101 01110100 00110010 11110000 11111111 10111000 [0xA5] 11/11/2011 20:12 | Supervisory - module detected: Keypad slot 8
   */
  if (dscPanelData[dscPanelByte] >= 0xE9 && dscPanelData[dscPanelByte] <= 0xF0) {
    printf("Supervisory - module detected: Keypad slot %d", dscPanelData[dscPanelByte] - 0xE8);
    return;
  }

  /*
   *  Supervisory trouble, keypad slots 1-8
   *
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00010001 01101101 01110100 10000110 11110001 11111111 00001101 [0xA5] 11/11/2011 20:33 | Supervisory - module trouble: Keypad slot 1
   *  10100101 0 00010001 01101101 01110100 00101110 11111000 11111111 10111100 [0xA5] 11/11/2011 20:11 | Supervisory - module trouble: Keypad slot 8
   */
  if (dscPanelData[dscPanelByte] >= 0xF1 && dscPanelData[dscPanelByte] <= 0xF8) {
    printf("Supervisory - module trouble: Keypad slot %d", dscPanelData[dscPanelByte] - 0xF0);
    return;
  }

  printf("Unrecognized data");
}


// Status messages for commands 0xA5, 0xEB
void dscPrintPanelStatus3(byte dscPanelByte) {
  printf("Unrecognized data: 0x%02X", dscPanelByte);
}


// Status messages for command 0xEB
void dscPrintPanelStatus4(byte dscPanelByte) {
  if (dscPanelData[dscPanelByte] <= 0x1F) {
    printf("Zone alarm: %d", dscPanelData[dscPanelByte] + 33);
    return;
  }

  if (dscPanelData[dscPanelByte] >= 0x20 && dscPanelData[dscPanelByte] <= 0x3F) {
    printf("Zone alarm restored: %d", dscPanelData[dscPanelByte] + 1);
    return;
  }

  if (dscPanelData[dscPanelByte] >= 0x40 && dscPanelData[dscPanelByte] <= 0x5F) {
    printf("Zone tamper: %d", dscPanelData[dscPanelByte] - 31);
    return;
  }

  if (dscPanelData[dscPanelByte] >= 0x60 && dscPanelData[dscPanelByte] <= 0x7F) {
    printf("Zone tamper restored: %d", dscPanelData[dscPanelByte] - 63);
    return;
  }


  printf("Unrecognized data");
}


// Status messages for command 0xEB
void dscPrintPanelStatus14(byte dscPanelByte) {
  if (dscPanelData[dscPanelByte] >= 0x40 && dscPanelData[dscPanelByte] <= 0x5F) {
    printf("Zone fault restored: %d", dscPanelData[dscPanelByte] - 31);
    return;
  }

  if (dscPanelData[dscPanelByte] >= 0x60 && dscPanelData[dscPanelByte] <= 0x7F) {
    printf("Zone fault: %d", dscPanelData[dscPanelByte] - 63);
    return;
  }


  printf("Unrecognized data");
}


// Prints individual bits as a number for partitions and zones
void dscPrintPanelBitNumbers(byte dscPanelByte, byte startNumber) {
  for (byte bit = 0; bit < 8; bit++) {
    if (bitRead(dscPanelData[dscPanelByte],bit)) {
      printf("%d ", startNumber + bit);
    }
  }
}


/*
 *  0x05: Status - partitions 1-4
 *  Interval: constant
 *  CRC: no
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4: Partition 2 lights
 *  Byte 5: Partition 2 status
 *
 *  PC5020/PC1616/PC1832/PC1864:
 *  Byte 6: Partition 3 lights
 *  Byte 7: Partition 3 status
 *  Byte 8: Partition 4 lights
 *  Byte 9: Partition 4 status
 *
 *  // PC1555MX, PC5015
 *  00000101 0 10000001 00000001 10010001 11000111 [0x05] Partition 1 | Lights: Ready Backlight | Partition ready | Partition 2: disabled
 *  00000101 0 10010000 00000011 10010001 11000111 [0x05] Status lights: Trouble Backlight | Partition not ready
 *  00000101 0 10001010 00000100 10010001 11000111 [0x05] Status lights: Armed Bypass Backlight | Armed stay
 *  00000101 0 10000010 00000101 10010001 11000111 [0x05] Status lights: Armed Backlight | Armed away
 *  00000101 0 10001011 00001000 10010001 11000111 [0x05] Status lights: Ready Armed Bypass Backlight | Exit delay in progress
 *  00000101 0 10000010 00001100 10010001 11000111 [0x05] Status lights: Armed Backlight | Entry delay in progress
 *  00000101 0 10000001 00010000 10010001 11000111 [0x05] Status lights: Ready Backlight | Keypad lockout
 *  00000101 0 10000010 00010001 10010001 11000111 [0x05] Status lights: Armed Backlight | Partition in alarm
 *  00000101 0 10000001 00110011 10010001 11000111 [0x05] Status lights: Ready Backlight | Partition busy
 *  00000101 0 10000001 00111110 10010001 11000111 [0x05] Status lights: Ready Backlight | Partition disarmed
 *  00000101 0 10000001 01000000 10010001 11000111 [0x05] Status lights: Ready Backlight | Keypad blanked
 *  00000101 0 10000001 10001111 10010001 11000111 [0x05] Status lights: Ready Backlight | Invalid access code
 *  00000101 0 10000000 10011110 10010001 11000111 [0x05] Status lights: Backlight | Quick armed pressed
 *  00000101 0 10000001 10100011 10010001 11000111 [0x05] Status lights: Ready Backlight | Door chime enabled
 *  00000101 0 10000001 10100100 10010001 11000111 [0x05] Status lights: Ready Backlight | Door chime disabled

 *  00000101 0 10000010 00001101 10010001 11000111 [0x05] Status lights: Armed Backlight   *  Delay zone tripped after dscPrevious alarm tripped
 *  00000101 0 10000000 00111101 10010001 11000111 [0x05] Status lights: Backlight   *  Disarmed after dscPrevious alarm tripped
 *  00000101 0 10000000 00100010 10010001 11000111 [0x05] Status lights: Backlight   *  Disarmed after dscPrevious alarm tripped +4s
 *  00000101 0 10000010 10100110 10010001 11000111 [0x05] Status lights: Armed Backlight   *  In *5 programming
 *
 *  // PC5020, PC1616, PC1832, PC1864
 *  00000101 0 10000000 00000011 10000010 00000101 10000010 00000101 00000000 11000111 [0x05] Status lights: Backlight | Zones open
 */
void dscPrintPanel_0x05() {
  printf("Partition 1: ");
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 2: disabled");
  }
  else {
    printf(" | Partition 2: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  if (dscPanelByteCount > 9) {
    if (dscPanelData[7] == 0xC7) {
      printf(" | Partition 3: disabled");
    }
    else {
      printf(" | Partition 3: ");
      dscPrintPanelLights(6);
      printf("- ");
      dscPrintPanelMessages(7);
    }

    if (dscPanelData[9] == 0xC7) {
      printf(" | Partition 4: disabled");
    }
    else {
      printf(" | Partition 4: ");
      dscPrintPanelLights(8);
      printf("- ");
      dscPrintPanelMessages(9);
    }
  }
}


/*
 *  0x0A: Status in alarm, programming
 *  Interval: constant in *8 programming
 *  CRC: yes
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4-7: Zone lights
 *  Byte 8: Zone lights for *5 access codes 33,34,41,42
 *
 *  00001010 0 10000010 11100100 00000000 00000000 00000000 00000000 00000000 01110000 [0x0A] Status lights: Armed | Zone lights: none
 *  00001010 0 10000001 11101110 01100101 00000000 00000000 00000000 00000000 11011110 [0x0A] Status lights: Ready | Zone lights: 1 3 6 7
 */
void dscPrintPanel_0x0A() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  bool zoneLights = false;
  printf(" | Zone lights: ");
  for (byte dscPanelByte = 4; dscPanelByte < 8; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte-4) *  8));
        }
      }
    }
  }

  if (dscPanelData[8] != 0 && dscPanelData[8] != 128) {
    zoneLights = true;
    if (bitRead(dscPanelData[8],0)) printf("33 ");
    if (bitRead(dscPanelData[8],1)) printf("34 ");
    if (bitRead(dscPanelData[8],3)) printf("41 ");
    if (bitRead(dscPanelData[8],4)) printf("42 ");
  }

  if (!zoneLights) printf("none");
}


/*
 *  0x11: Keypad slot query
 *  Interval: 30s
 *  CRC: no
 *
 *  00010001 0 10101010 10101010 10101010 10101010 10101010 [0x11] Keypad slot query
 *  11111111 1 00111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1
 *  11111111 1 11111111 11111100 11111111 11111111 11111111 [Keypad] Slot 8
 */
void dscPrintPanel_0x11() {
  printf("Keypad slot query");
}


/*
 *  0x16: Zone wiring
 *  Interval: 4min
 *  CRC: yes
 *  Byte 2: TBD, identical with PC1555MX, PC5015, PC1832
 *  Byte 3: TBD, different between PC1555MX, PC5015, PC1832
 *  Byte 4 bits 2-7: TBD, identical with PC1555MX and PC5015
 *
 *  00010110 0 00001110 00100011 11010001 00011001 [0x16] PC1555MX | Zone wiring: NC | Exit *8 programming
 *  00010110 0 00001110 00100011 11010010 00011001 [0x16] PC1555MX | Zone wiring: EOL | Exit *8 programming
 *  00010110 0 00001110 00100011 11010011 00011001 [0x16] PC1555MX | Zone wiring: DEOL | Exit *8 programming
 *  00010110 0 00001110 00100011 11100001 00101000 [0x16] PC1555MX | Zone wiring: NC | In *8
 *  00010110 0 00001110 00100011 11100110 00101101 [0x16] PC1555MX | Zone wiring: EOL | Enter *8 programming
 *  00010110 0 00001110 00100011 11110010 00111001 [0x16] PC1555MX | Zone wiring: EOL | Armed, Exit *8 +15s, Power-on +2m
 *  00010110 0 00001110 00100011 11110111 00111101 [0x16] PC1555MX | Zone wiring: DEOL | Interval 4m
 *  00010110 0 00001110 00010000 11110011 00100111 [0x16] PC5015 | Zone wiring: DEOL | Armed, Exit *8 +15s, Power-on +2m
 *  00010110 0 00001110 01000001 11110101 01011010 [0x16] PC1832 | Zone wiring: NC | Interval 4m
 *  00010110 0 00001110 01000010 10110101 00011011 [0x16] PC1864 | Zone wiring: NC | Interval 4m
 *  00010110 0 00001110 01000010 10110001 00010111 [0x16] PC1864 | Zone wiring: NC | Armed
 */
void dscPrintPanel_0x16() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  if (dscPanelData[2] == 0x0E) {

    switch (dscPanelData[3]) {
      case 0x10: printf("PC5015 "); break;
      case 0x23: printf("PC1555MX "); break;
      case 0x41: printf("PC1832 "); break;
      case 0x42: printf("PC1864 "); break;
      default: printf("Unknown panel "); break;
    }

    switch (dscPanelData[4] & 0x03) {
      case 0x01: printf("| Zone wiring: NC "); break;
      case 0x02: printf("| Zone wiring: EOL "); break;
      case 0x03: printf("| Zone wiring: DEOL "); break;
    }

    switch (dscPanelData[4] >> 2) {
      case 0x2C: printf("| Armed"); break;
      case 0x2D: printf("| Interval 4m"); break;
      case 0x34: printf("| Exit *8 programming"); break;
      case 0x39: printf("| *8 programming"); break;
      case 0x3C: printf("| Armed, Exit *8 +15s, Power-on +2m"); break;
      case 0x3D: printf("| Interval 4m"); break;
      default: printf("| Unrecognized data"); break;
    }
  }
  else printf("Unrecognized data");
}


/*
 *  0x1B: Status - partitions 5-8
 *  Interval: constant
 *  CRC: no
 *  Byte 2: Partition 5 lights
 *  Byte 3: Partition 5 status
 *  Byte 4: Partition 6 lights
 *  Byte 5: Partition 6 status
 *  Byte 6: Partition 7 lights
 *  Byte 7: Partition 7 status
 *  Byte 8: Partition 8 lights
 *  Byte 9: Partition 8 status
 *
 *  00011011 0 10010001 00000001 00010000 11000111 00010000 11000111 00010000 11000111 [0x1B]
 */
void dscPrintPanel_0x1B() {

  if (dscPanelData[3] == 0xC7) {
    printf("Partition 5: disabled");
  }
  else {
    printf("Partition 5: ");
    dscPrintPanelLights(2);
    printf("- ");
    dscPrintPanelMessages(3);
  }

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 6: disabled");
  }
  else {
    printf(" | Partition 6: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  if (dscPanelData[7] == 0xC7) {
    printf(" | Partition 7: disabled");
  }
  else {
    printf(" | Partition 7: ");
    dscPrintPanelLights(6);
    printf("- ");
    dscPrintPanelMessages(7);
  }

  if (dscPanelData[9] == 0xC7) {
    printf(" | Partition 8: disabled");
  }
  else {
    printf(" | Partition 8: ");
    dscPrintPanelLights(8);
    printf("- ");
    dscPrintPanelMessages(9);
  }
}


/*
 *  0x1C: Verify keypad Fire/Auxiliary/Panic
 *  Interval: immediate after keypad button press
 *  CRC: no
 *
 *  01110111 1 11111111 11111111 11111111 11111111 [Keypad] Fire alarm
 *  00011100 0  [0x1C] Verify keypad Fire/Auxiliary/Panic
 *  01110111 1  [Keypad] Fire alarm
 */
void dscPrintPanel_0x1C() {
  printf("Verify keypad Fire/Auxiliary/Panic");
}


/*
 *  0x27: Status with zones 1-8
 *  Interval: 4m
 *  CRC: yes
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4: Partition 2 lights
 *  Byte 5: Partition 2 status
 *  Byte 6: Zones 1-8
 *
 *  00100111 0 10000001 00000001 10010001 11000111 00000000 00000001 [0x27] Status lights: Ready Backlight | Zones lights: none   // Unarmed, zones closed
 *  00100111 0 10000001 00000001 10010001 11000111 00000010 00000011 [0x27] Status lights: Ready Backlight | Zones lights: 2   // Unarmed, zone 2 open
 *  00100111 0 10001010 00000100 10010001 11000111 00000000 00001101 [0x27] Status lights: Armed Bypass Backlight | Zones lights: none  // Armed stay  // Periodic while armed
 *  00100111 0 10001010 00000100 11111111 11111111 00000000 10110011 [0x27] Status lights: Armed Bypass Backlight | Zones lights: none  // Armed stay +1s
 *  00100111 0 10000010 00000101 10010001 11000111 00000000 00000110 [0x27] Status lights: Armed Backlight | Zones lights: none  // Armed away  // Periodic while armed
 *  00100111 0 10000010 00000101 11111111 11111111 00000000 10101100 [0x27] Status lights: Armed Backlight | Zones lights: none  // Armed away +1s
 *  00100111 0 10000010 00001100 10010001 11000111 00000001 00001110 [0x27] Status lights: Armed Backlight | Zones lights: 1  // Delay zone 1 tripped, entrance delay
 *  00100111 0 10000010 00010001 10010001 11000111 00000001 00010011 [0x27] Status lights: Armed Backlight | Zones lights: 1  // Periodic after delay zone 1 tripped, alarm on   *  Periodic after fire alarm, alarm on
 *  00100111 0 10000010 00001101 10010001 11000111 00000001 00001111 [0x27] Status lights: Armed Backlight | Zones lights: 1  // Immediate after delay zone 1 tripped after dscPrevious alarm tripped
 *  00100111 0 10000010 00010001 11011011 11111111 00000010 10010110 [0x27] Status lights: Armed Backlight | Zones lights: 2  // Instant zone 2 tripped away
 *  00100111 0 00000001 00000001 11111111 11111111 00000000 00100111 [0x27] Status lights: Ready | Zones open 1-8: none  // Immediate after power on after panel reset
 *  00100111 0 10010001 00000001 11111111 11111111 00000000 10110111 [0x27] Status lights: Ready Trouble Backlight | Zones open 1-8: none  // +15s after exit *8
 *  00100111 0 10010001 00000001 10100000 00000000 00000000 01011001 [0x27] Status lights: Ready Trouble Backlight | Zones open 1-8: none  // +33s after power on after panel reset
 *  00100111 0 10010000 00000011 11111111 11111111 00111111 11110111 [0x27] Status lights: Trouble Backlight | Zones open: 1 2 3 4 5 6  // +122s after power on after panel reset
 *  00100111 0 10010000 00000011 10010001 11000111 00111111 01010001 [0x27] Status lights: Trouble Backlight | Zones open: 1 2 3 4 5 6  // +181s after power on after panel reset
 *  00100111 0 10000000 00000011 10000010 00000101 00011101 01001110 [0x27] Status lights: Backlight | Zones open | Zones 1-8 open: 1 3 4 5  // PC1832
 */
void dscPrintPanel_0x27() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1: ");
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 2: disabled");
  }
  else if (dscPanelData[5] != 0xFF) {
    printf(" | Partition 2: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  printf(" | Zones 1-8 open: ");
  if (dscPanelData[6] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(6,1);
  }
}

/*
 *  0x28: Zone expander query
 *  Interval: after zone expander status notification
 *  CRC: no
 *
 *  11111111 1 11111111 11111111 10111111 11111111 [Zone Expander] Status notification
 *  00101000 0 11111111 11111111 11111111 11111111 11111111 [0x28] Zone expander query
 *  11111111 1 01010111 01010101 11111111 11111111 01101111 [Zone Expander] Status
 */
void dscPrintPanel_0x28() {
  printf("Zone expander query");
}


/*
 *  0x2D: Status with zones 9-16
 *  Interval: 4m
 *  CRC: yes
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4: Partition 2 lights
 *  Byte 5: Partition 2 status
 *  Byte 6: Zones 9-16
 *
 *  00101101 0 10000000 00000011 10000001 11000111 00000001 11111001 [0x2D] Status lights: Backlight | Partition not ready | Open zones: 9
 *  00101101 0 10000000 00000011 10000010 00000101 00000000 00110111 [0x2D] Status lights: Backlight | Zones open | Zones 9-16 open: none  // PC1832
 */
void dscPrintPanel_0x2D() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1: ");
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 2: disabled");
  }
  else if (dscPanelData[5] != 0xFF) {
    printf(" | Partition 2: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  printf(" | Zones 9-16 open: ");
  if (dscPanelData[6] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(6,9);
  }
}


/*
 *  0x34: Status with zones 17-24
 *  Interval: 4m
 *  CRC: yes
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4: Partition 2 lights
 *  Byte 5: Partition 2 status
 *  Byte 6: Zones 17-24
 */
void dscPrintPanel_0x34() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1: ");
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 2: disabled");
  }
  else if (dscPanelData[5] != 0xFF) {
    printf(" | Partition 2: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  printf(" | Zones 17-24 open: ");
  if (dscPanelData[6] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(6,17);
  }
}


/*
 *  0x3E: Status with zones 25-32
 *  Interval: 4m
 *  CRC: yes
 *  Byte 2: Partition 1 lights
 *  Byte 3: Partition 1 status
 *  Byte 4: Partition 2 lights
 *  Byte 5: Partition 2 status
 *  Byte 6: Zones 25-32
 */
void dscPrintPanel_0x3E() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1: ");
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);

  if (dscPanelData[5] == 0xC7) {
    printf(" | Partition 2: disabled");
  }
  else if (dscPanelData[5] != 0xFF) {
    printf(" | Partition 2: ");
    dscPrintPanelLights(4);
    printf("- ");
    dscPrintPanelMessages(5);
  }

  printf(" | Zones 25-32 open: ");
  if (dscPanelData[6] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(6,25);
  }
}


/*
 *  0x4C: Keybus query
 *  Interval: immediate after exiting *8 programming, immediate on keypad query
 *  CRC: no
 *
 *  11111111 1 11111111 11111111 11111110 11111111 [Keypad] Keybus notification
 *  01001100 0 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 [0x4C] Keybus query
 */
void dscPrintPanel_0x4C() {
  printf("Keybus query");
}


/*
 *  0x58: Keybus query - valid response produces 0xA5 command, byte 6 0xB1-0xC0
 *  Interval: immediate after power on after panel reset
 *  CRC: no
 *
 *  11111111 1 11111111 11111111 11111111 11011111
 *  01011000 0 10101010 10101010 10101010 10101010 [0x58] Keybus query
 *  11111111 1 11111100 11111111 11111111 11111111
 *  10100101 0 00011000 01010101 01000000 11010111 10110011 11111111 11011011 [0xA5] 05/10/2018 00:53 | Unrecognized data, add to 0xA5_Byte7_0xFF, Byte 6: 0xB3
 */
void dscPrintPanel_0x58() {
  printf("Keybus query");
}


/*
 *  0x5D: Flash panel lights: status and zones 1-32, partition 1
 *  Interval: 30s
 *  CRC: yes
 *  Byte 2: Status lights
 *  Byte 3: Zones 1-8
 *  Byte 4: Zones 9-16
 *  Byte 5: Zones 17-24
 *  Byte 6: Zones 25-32
 *
 *  01011101 0 00000000 00000000 00000000 00000000 00000000 01011101 [0x5D] Partition 1 | Status lights flashing: none | Zones 1-32 flashing: none
 *  01011101 0 00100000 00000000 00000000 00000000 00000000 01111101 [0x5D] Partition 1 | Status lights flashing: Program | Zones 1-32 flashing: none
 *  01011101 0 00000000 00100000 00000000 00000000 00000000 01111101 [0x5D] Partition 1 | Status lights flashing: none  | Zones 1-32 flashing: none 6
 *  01011101 0 00000100 00100000 00000000 00000000 00000000 10000001 [0x5D] Partition 1 | Status lights flashing: Memory | Zones 1-32 flashing: 6
 *  01011101 0 00000000 00000000 00000001 00000000 00000000 01011110 [0x5D] Partition 1 | Status lights flashing: none | Zones flashing: 9
 */
void dscPrintPanel_0x5D() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1 | Status lights flashing: ");
  dscPrintPanelLights(2);

  bool zoneLights = false;
  printf("| Zones 1-32 flashing: ");
  for (byte dscPanelByte = 3; dscPanelByte <= 6; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte-3) *  8));
        }
      }
    }
  }
  if (!zoneLights) printf("none");
}


/*
 *  0x63: Flash panel lights: status and zones 1-32, partition 2
 *  Interval: 30s
 *  CRC: yes
 *  Byte 2: Status lights
 *  Byte 3: Zones 1-8
 *  Byte 4: Zones 9-16
 *  Byte 5: Zones 17-24
 *  Byte 6: Zones 25-32
 *
 *  01100011 0 00000000 00000000 00000000 00000000 00000000 01100011 [0x63] Partition 2 | Status lights flashing: none | Zones 1-32 flashing: none
 *  01100011 0 00000100 10000000 00000000 00000000 00000000 11100111 [0x63] Partition 2 | Status lights flashing:Memory | Zones 1-32 flashing: 8
 */
void dscPrintPanel_0x63() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 2 | Status lights flashing: ");
  dscPrintPanelLights(2);

  bool zoneLights = false;
  printf("| Zones 1-32 flashing: ");
  for (byte dscPanelByte = 3; dscPanelByte <= 6; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte-3) *  8));
        }
      }
    }
  }
  if (!zoneLights) printf("none");
}


/*
 *  0x64: Beep - one-time, partition 1
 *  CRC: yes
 *
 *  01100100 0 00001100 01110000 [0x64] Partition 1 | Beep: 6 beeps
 */
void dscPrintPanel_0x64() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1 | Beep: ");
  switch (dscPanelData[2]) {
    case 0x04: printf("2 beeps"); break;
    case 0x06: printf("3 beeps"); break;
    case 0x08: printf("4 beeps"); break;
    case 0x0C: printf("6 beeps"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x69: Beep - one-time, partition 2
 *  CRC: yes
 *
 *  01101001 0 00001100 01110101 [0x69] Partition 2 | Beep: 6 beeps
 */
void dscPrintPanel_0x69() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 2 | Beep: ");
  switch (dscPanelData[2]) {
    case 0x04: printf("2 beeps"); break;
    case 0x06: printf("3 beeps"); break;
    case 0x08: printf("4 beeps"); break;
    case 0x0C: printf("6 beeps"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x75: Beep pattern - repeated, partition 1
 *  CRC: yes
 *
 *  01110101 0 10000000 11110101 [0x75] Partition 1 | Beep pattern: solid tone
 *  01110101 0 00000000 01110101 [0x75] Partition 1 | Beep pattern: off
 */
void dscPrintPanel_0x75() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1 | Beep pattern: ");
  switch (dscPanelData[2]) {
    case 0x00: printf("off"); break;
    case 0x11: printf("single beep (exit delay)"); break;
    case 0x31: printf("triple beep (exit delay)"); break;
    case 0x80: printf("solid tone"); break;
    case 0xB1: printf("triple beep (entrance delay)"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x7A: Beep pattern - repeated, partition 2
 *  CRC: yes
 *
 *  01111010 0 00000000 01111010 [0x7A] Partition 2 | Beep pattern: off
 */
void dscPrintPanel_0x7A() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 2 | Beep pattern: ");
  switch (dscPanelData[2]) {
    case 0x00: printf("off"); break;
    case 0x11: printf("single beep (exit delay)"); break;
    case 0x31: printf("triple beep (exit delay)"); break;
    case 0x80: printf("solid tone"); break;
    case 0xB1: printf("triple beep (entrance delay)"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x7F: Beep - one-time
 *  CRC: yes
 *
 *  01111111 0 00000001 10000000 [0x7F] Beep: long beep
 */
void dscPrintPanel_0x7F() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 1 | ");
  switch (dscPanelData[2]) {
    case 0x01: printf("Beep: long beep"); break;
    case 0x02: printf("Beep: long beep | Failed to arm"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x82: Beep - one-time
 *  CRC: yes
 *
 *  01111111 0 00000001 10000000 [0x82] Beep: long beep
 */
void dscPrintPanel_0x82() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Partition 2 | ");
  switch (dscPanelData[2]) {
    case 0x01: printf("Beep: long beep"); break;
    case 0x02: printf("Beep: long beep | Failed to arm"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0x87: Panel outputs
 *  CRC: yes
 *
 *  10000111 0 00000000 00000000 10000111 [0x87] Panel output: Bell off | PGM1 off | PGM2 off
 *  10000111 0 11111111 11110000 01110110 [0x87] Panel output: Bell on | PGM1 off | PGM2 off
 *  10000111 0 11111111 11110010 01111000 [0x87] Panel output: Bell on | PGM1 off | PGM2 on
 *  10000111 0 00000000 00000001 10001000 [0x87] Panel output: Bell off | PGM1 on | PGM2 off
 *  10000111 0 00000000 00001000 10001111 [0x87] Panel output: Bell off | Unrecognized command: Add to 0x87
 */
void dscPrintPanel_0x87() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Panel output:");
  switch (dscPanelData[2] & 0xF0) {
    case 0xF0: printf(" Bell on"); break;
    default: printf(" Bell off"); break;
  }

  if ((dscPanelData[3] & 0x0F) <= 0x03) {
    if (bitRead(dscPanelData[3],0)) printf(" | PGM1 on");
    else printf(" | PGM1 off");

    if (bitRead(dscPanelData[3],1)) printf(" | PGM2 on");
    else printf(" | PGM2 off");
  }
  else printf(" | Unrecognized data");

  if ((dscPanelData[2] & 0x0F) != 0x0F) {
    if (bitRead(dscPanelData[2],0)) printf(" | PGM3 on");
    else printf(" | PGM3 off");

    if (bitRead(dscPanelData[2],1)) printf(" | PGM4 on");
    else printf(" | PGM4 off");
  }
}


/*
 *  0x8D: User code programming key response, codes 17-32
 *  CRC: yes
 *  Byte 2: TBD
 *  Byte 3: TBD
 *  Byte 4: TBD
 *  Byte 5: TBD
 *  Byte 6: TBD
 *  Byte 7: TBD
 *  Byte 8: TBD
 *
 *  10001101 0 00110001 00000001 00000000 00010111 11111111 11111111 11111111 11010011 [0x8D]   // Code 17 Key 1
 *  10001101 0 00110001 00000100 00000000 00011000 11111111 11111111 11111111 11010111 [0x8D]   // Code 18 Key 1
 *  10001101 0 00110001 00000100 00000000 00010010 11111111 11111111 11111111 11010001 [0x8D]   // Code 18 Key 2
 *  10001101 0 00110001 00000101 00000000 00111000 11111111 11111111 11111111 11111000 [0x8D]   // Code 18 Key 3
 *  10001101 0 00110001 00000101 00000000 00110100 11111111 11111111 11111111 11110100 [0x8D]   // Code 18 Key 4
 *  10001101 0 00110001 00100101 00000000 00001001 11111111 11111111 11111111 11101001 [0x8D]   // Code 29 Key 0
 *  10001101 0 00110001 00100101 00000000 00000001 11111111 11111111 11111111 11100001 [0x8D]   // Code 29 Key 1
 *  10001101 0 00110001 00110000 00000000 00000000 11111111 11111111 11111111 11101011 [0x8D]   // Message after 4th key entered
 */
void dscPrintPanel_0x8D() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("User code programming key response");
}


/*
 *  0x94: ???
 *  Interval: immediate after entering *5 access code programming
 *  CRC: no
 *  Byte 2: TBD
 *  Byte 3: TBD
 *  Byte 4: TBD
 *  Byte 5: TBD
 *  Byte 6: TBD
 *  Byte 7: TBD
 *  Byte 8: TBD
 *  Byte 9: TBD
 *
 *  10010100 0 00010001 00000000 00000000 10100101 00000000 00000000 00000000 00010111 10100000 [0x94] Unknown command 1
 *  10010100 0 00010001 00000000 00000000 10100101 00000000 00000000 00000000 01001100 11111100 [0x94] Unknown command 2
 */
void dscPrintPanel_0x94() {
  switch (dscPanelData[9]) {
    case 0x17: printf("Unknown command 1"); break;
    case 0x4C: printf("Unknown command 2"); break;
    default: printf("Unrecognized data");
  }
}


/*
 *  0xA5: Date, time, system status messages - partitions 1-2
 *  CRC: yes
 */
void dscPrintPanel_0xA5() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  /*
   *  Date and time
   *  Interval: 4m
   *             YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
   *  10100101 0 00011000 00001110 11101101 10000000 00000000 00000000 00111000 [0xA5] 03/23/2018 13:32 | Timestamp
   */
  byte dscYear3 = dscPanelData[2] >> 4;
  byte dscYear4 = dscPanelData[2] & 0x0F;
  byte dscMonth = dscPanelData[3] << 2; dscMonth >>=4;
  byte dscDay1 = dscPanelData[3] << 6; dscDay1 >>= 3;
  byte dscDay2 = dscPanelData[4] >> 5;
  byte dscDay = dscDay1 | dscDay2;
  byte dscHour = dscPanelData[4] & 0x1F;
  byte dscMinute = dscPanelData[5] >> 2;

  if (dscYear3 >= 7) printf("19");
  else printf("20");
  printf("%d%d.%02d.%02d %02d:%02d", dscYear3, dscYear4, dscMonth, dscDay, dscHour, dscMinute);

  /*printf("%d", dscYear3);
  printf("%d", dscYear4);
  printf(".");
  if (dscMonth < 10) printf("0");
  printf("%d", dscMonth);
  printf(".");
  if (dscDay < 10) printf("0");
  printf("%d", dscDay);
  printf(" ");
  if (dscHour < 10) printf("0");
  printf("%d", dscHour);
  printf(":");
  if (dscMinute < 10) printf("0");
  printf("%d", dscMinute);*/

  if (dscPanelData[6] == 0 && dscPanelData[7] == 0) {
    printf(" | Timestamp");
    return;
  }

  switch (dscPanelData[3] >> 6) {
    case 0x00: printf(" | "); break;
    case 0x01: printf(" | Partition 1 | "); break;
    case 0x02: printf(" | Partition 2 | "); break;
  }

  switch (dscPanelData[5] & 0x03) {
    case 0x00: dscPrintPanelStatus0(6); return;
    case 0x01: dscPrintPanelStatus1(6); return;
    case 0x02: dscPrintPanelStatus2(6); return;
    case 0x03: dscPrintPanelStatus3(6); return;
  }
}


/*
 *  0xB1: Enabled zones 1-32, partitions 1,2
 *  Interval: 4m
 *  CRC: yes
 *  Bytes 2-5: partition 1
 *  Bytes 6-9: partition 2
 *
 *  10110001 0 11111111 00000000 00000000 00000000 00000000 00000000 00000000 00000000 10110000 [0xB1] Enabled zones - Partition 1: 1 2 3 4 5 6 7 8 | Partition 2: none
 *  10110001 0 10010001 10001010 01000001 10100100 00000000 00000000 00000000 00000000 10110001 [0xB1] Enabled zones - Partition 1: 1 5 8 10 12 16 17 23 27 30 32 | Partition 2: none
 *  10110001 0 11111111 00000000 00000000 11111111 00000000 00000000 00000000 00000000 10101111 [0xB1] Enabled zones - Partition 1: 1 2 3 4 5 6 7 8 25 26 27 28 29 30 31 32 | Partition 2: none
 *  10110001 0 01111111 11111111 00000000 00000000 10000000 00000000 00000000 00000000 10101111 [0xB1] Enabled zones - Partition 1: 1 2 3 4 5 6 7 9 10 11 12 13 14 15 16 | Partition 2: 8
 */
void dscPrintPanel_0xB1() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  bool enabledZones = false;
  printf("Enabled zones 1-32 | Partition 1: ");
  for (byte dscPanelByte = 2; dscPanelByte <= 5; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      enabledZones = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte - 2) * 8));
        }
      }
    }
  }
  if (!enabledZones) printf("none ");

  enabledZones = false;
  printf("| Partition 2: ");
  for (byte dscPanelByte = 6; dscPanelByte <= 9; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      enabledZones = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte - 6) * 8));
        }
      }
    }
  }
  if (!enabledZones) printf("none");
}


/*
 *  0xBB: Bell
 *  Interval: immediate after alarm tripped except silent zones
 *  CRC: yes
 *
 *  10111011 0 00100000 00000000 11011011 [0xBB] Bell: on
 *  10111011 0 00000000 00000000 10111011 [0xBB] Bell: off
 */
void dscPrintPanel_0xBB() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  printf("Bell: ");
  if (bitRead(dscPanelData[2],5)) printf("on");
  else printf("off");
}


/*
 *  0xC3: Keypad status
 *  Interval: 30s (PC1616/PC1832/PC1864)
 *  CRC: yes
 *
 *  11000011 0 00010000 11111111 11010010 [0xC3] Unknown command 1: Power-on +33s
 *  11000011 0 00110000 11111111 11110010 [0xC3] Keypad lockout
 *  11000011 0 00000000 11111111 11000010 [0xC3] Keypad ready
 */
void dscPrintPanel_0xC3() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  if (dscPanelData[3] == 0xFF) {
    switch (dscPanelData[2]) {
      case 0x00: printf("Keypad ready"); break;
      case 0x10: printf("Unknown command 1: Power-on +33s"); break;
      case 0x30:
      case 0x40: printf("Keypad lockout"); break;
      default: printf("Unrecognized data"); break;
    }
  }
  else printf("Unrecognized data");
}


/*
 *  0xCE: Unknown command
 *  CRC: yes
 *
 * 11001110 0 00000001 10100000 00000000 00000000 00000000 01101111 [0xCE]  // Partition 1 exit delay
 * 11001110 0 00000001 10110001 00000000 00000000 00000000 10000000 [0xCE]  // Partition 1 armed stay
 * 11001110 0 00000001 10110011 00000000 00000000 00000000 10000010 [0xCE]  // Partition 1 armed away
 * 11001110 0 00000001 10100100 00000000 00000000 00000000 01110011 [0xCE]  // Partition 2 armed away
 * 11001110 0 01000000 11111111 11111111 11111111 11111111 00001010 [0xCE]  // Partition 1,2 activity
 */
void dscPrintPanel_0xCE() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  switch (dscPanelData[2]) {
    case 0x01: {
      switch (dscPanelData[3]) {
        case 0xA0: printf("Partition 1,2 exit delay, partition 1,2 disarmed"); break;
        case 0xA4: printf("Partition 2 armed away"); break;
        case 0xB1: printf("Partition 1 armed stay"); break;
        case 0xB3: printf("Partition 1 armed away"); break;
        default: printf("Unrecognized data"); break;
      }
      break;
    }
    case 0x40: printf("Partition 1,2 activity"); break;
    default: printf("Unrecognized data"); break;
  }
}

/*
 *  0xD5: Keypad zone query
 *  CRC: no
 *
 *  11111111 1 11111111 11111111 11111111 11111011 [Keypad] Status notification
 *  11010101 0 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 [0xD5] Keypad zone query
 *  11111111 1 11111111 11111111 11111111 11111111 11111111 11111111 11111111 00001111 [Keypad] Slot 8
 */
void dscPrintPanel_0xD5() {
  printf("Keypad zone query");
}


/*
 *  0xE6: Status, partitions 1-8
 *  CRC: yes
 *  Panels: PC5020, PC1616, PC1832, PC1864
 */
void dscPrintPanel_0xE6() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  switch (dscPanelData[2]) {
    case 0x03: dscPrintPanel_0xE6_0x03(); break;  // Status in alarm/programming, partitions 5-8
    case 0x09: dscPrintPanel_0xE6_0x09(); break;  // Zones 33-40 status
    case 0x0B: dscPrintPanel_0xE6_0x0B(); break;  // Zones 41-48 status
    case 0x0D: dscPrintPanel_0xE6_0x0D(); break;  // Zones 49-56 status
    case 0x0F: dscPrintPanel_0xE6_0x0F(); break;  // Zones 57-64 status
    case 0x17: dscPrintPanel_0xE6_0x17(); break;  // Flash panel lights: status and zones 1-32, partitions 1-8
    case 0x18: dscPrintPanel_0xE6_0x18(); break;  // Flash panel lights: status and zones 33-64, partitions 1-8
    case 0x19: dscPrintPanel_0xE6_0x19(); break;  // Beep - one-time, partitions 3-8
    case 0x1A: dscPrintPanel_0xE6_0x1A(); break;  // Unknown command
    case 0x1D: dscPrintPanel_0xE6_0x1D(); break;  // Beep pattern, partitions 3-8
    case 0x20: dscPrintPanel_0xE6_0x20(); break;  // Status in programming, zone lights 33-64
    case 0x2B: dscPrintPanel_0xE6_0x2B(); break;  // Enabled zones 1-32, partitions 3-8
    case 0x2C: dscPrintPanel_0xE6_0x2C(); break;  // Enabled zones 33-64, partitions 3-8
    case 0x41: dscPrintPanel_0xE6_0x41(); break;  // Status in access code programming, zone lights 65-95
    default: printf("Unrecognized data");
  }
}


/*
 *  0xE6_0x03: Status in alarm/programming, partitions 5-8
 */
void dscPrintPanel_0xE6_0x03() {
  dscPrintPanelLights(2);
  printf("- ");
  dscPrintPanelMessages(3);
}


/*
 *  0xE6_0x09: Zones 33-40 status
 */
void dscPrintPanel_0xE6_0x09() {
  printf("Zones 33-40 open: ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,33);
  }
}


/*
 *  0xE6_0x0B: Zones 41-48 status
 */
void dscPrintPanel_0xE6_0x0B() {
  printf("Zones 41-48 open: ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,41);
  }
}


/*
 *  0xE6_0x0D: Zones 49-56 status
 */
void dscPrintPanel_0xE6_0x0D() {
  printf("Zones 49-56 open: ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,49);
  }
}


/*
 *  0xE6_0x0F: Zones 57-64 status
 */
void dscPrintPanel_0xE6_0x0F() {
  printf("Zones 57-64 open: ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,57);
  }
}


/*
 *  0xE6_0x17: Flash panel lights: status and zones 1-32, partitions 1-8
 *
 *  11100110 0 00010111 00000100 00000000 00000100 00000000 00000000 00000000 00000101 [0xE6] Partition 3 |  // Zone 3
 */
void dscPrintPanel_0xE6_0x17() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  printf("| Status lights flashing: ");
  dscPrintPanelLights(4);

  bool zoneLights = false;
  printf("| Zones 1-32 flashing: ");
  for (byte dscPanelByte = 5; dscPanelByte <= 8; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte-5) *  8));
        }
      }
    }
  }
  if (!zoneLights) printf("none");
}


/*
 *  0xE6_0x18: Flash panel lights: status and zones 33-64, partitions 1-8
 *
 *  11100110 0 00011000 00000001 00000000 00000001 00000000 00000000 00000000 00000000 [0xE6] Partition 1 |  // Zone 33
 *  11100110 0 00011000 00000001 00000100 00000000 00000000 00000000 10000000 10000011 [0xE6] Partition 1 |  // Zone 64
 */
void dscPrintPanel_0xE6_0x18() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  printf("| Status lights flashing: ");
  dscPrintPanelLights(4);

  bool zoneLights = false;
  printf("| Zones 33-64 flashing: ");
  for (byte dscPanelByte = 5; dscPanelByte <= 8; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 33) + ((dscPanelByte-5) *  8));
        }
      }
    }
  }
  if (!zoneLights) printf("none");
}


/*
 *  0xE6_0x19: Beep - one time, partitions 3-8
 */
void dscPrintPanel_0xE6_0x19() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  printf("| Beep: ");
  switch (dscPanelData[4]) {
    case 0x04: printf("2 beeps"); break;
    case 0x06: printf("3 beeps"); break;
    case 0x08: printf("4 beeps"); break;
    case 0x0C: printf("6 beeps"); break;
    default: printf("Unrecognized data"); break;
  }
}


void dscPrintPanel_0xE6_0x1A() {
  printf("0x1A: ");
  printf("Unrecognized data");
}


/*
 *  0xE6_0x1D: Beep pattern, partitions 3-8
 */
void dscPrintPanel_0xE6_0x1D() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  printf("| Beep pattern: ");
  switch (dscPanelData[4]) {
    case 0x00: printf("off"); break;
    case 0x11: printf("single beep (exit delay)"); break;
    case 0x31: printf("triple beep (exit delay)"); break;
    case 0x80: printf("solid tone"); break;
    case 0xB1: printf("triple beep (entrance delay)"); break;
    default: printf("Unrecognized data"); break;
  }
}


/*
 *  0xE6_0x20: Status in programming, zone lights 33-64
 *  Interval: constant in *8 programming
 *  CRC: yes
 */
void dscPrintPanel_0xE6_0x20() {
  printf("Status lights: ");
  dscPrintPanelLights(3);
  printf("- ");
  dscPrintPanelMessages(4);

  bool zoneLights = false;
  printf(" | Zone lights: ");
  for (byte dscPanelByte = 5; dscPanelByte <= 8; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 33) + ((dscPanelByte-5) *  8));
        }
      }
    }
  }

  if (!zoneLights) printf("none");
}


/*
 *  0xE6_0x2B: Enabled zones 1-32, partitions 3-8
 */
void dscPrintPanel_0xE6_0x2B() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  bool enabledZones = false;
  printf("| Enabled zones  1-32: ");
  for (byte dscPanelByte = 4; dscPanelByte <= 7; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      enabledZones = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 1) + ((dscPanelByte - 4) * 8));
        }
      }
    }
  }
  if (!enabledZones) printf("none");
}


/*
 *  0xE6_0x2C: Enabled zones 33-64, partitions 1-8
 */
void dscPrintPanel_0xE6_0x2C() {
  printf("Partition ");
  if (dscPanelData[3] == 0) printf("none");
  else {
    dscPrintPanelBitNumbers(3,1);
  }

  bool enabledZones = false;
  printf("| Enabled zones 33-64: ");
  for (byte dscPanelByte = 4; dscPanelByte <= 7; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      enabledZones = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 33) + ((dscPanelByte - 4) * 8));
        }
      }
    }
  }
  if (!enabledZones) printf("none");
}


/*
 *  0xE6_0x41: Status in programming, zone lights 65-95
 *  CRC: yes
 */
void dscPrintPanel_0xE6_0x41() {
  printf("Status lights: ");
  dscPrintPanelLights(3);
  printf("- ");
  dscPrintPanelMessages(4);

  bool zoneLights = false;
  printf(" | Zone lights: ");
  for (byte dscPanelByte = 5; dscPanelByte <= 8; dscPanelByte++) {
    if (dscPanelData[dscPanelByte] != 0) {
      zoneLights = true;
      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(dscPanelData[dscPanelByte],zoneBit)) {
          printf("%d ", (zoneBit + 65) + ((dscPanelByte-5) *  8));
        }
      }
    }
  }

  if (!zoneLights) printf("none");
}


/*
 *  0xEB: Date, time, system status messages - partitions 1-8
 *  CRC: yes
 *
 *                     YYY1YYY2   MMMMDD DDDHHHHH MMMMMM
 * 11101011 0 00000001 00011000 00011000 10001010 00101100 00000000 10111011 00000000 10001101 [0xEB] 06/04/2018 10:11 | Partition: 1  // Armed stay
 * 11101011 0 00000001 00011000 00011000 10001010 00111000 00000000 10111011 00000000 10011001 [0xEB] 06/04/2018 10:14 | Partition: 1  // Armed away
 * 11101011 0 00000001 00011000 00011000 10001010 00111000 00000010 10011011 00000000 01111011 [0xEB] 06/04/2018 10:14 | Partition: 1  // Armed away
 * 11101011 0 00000001 00011000 00011000 10001010 00110100 00000000 11100010 00000000 10111100 [0xEB] 06/04/2018 10:13 | Partition: 1  // Disarmed
 * 11101011 0 00000001 00011000 00011000 10001111 00101000 00000100 00000000 10010001 01101000 [0xEB] 06/04/2018 15:10 | Partition: 1 | Unrecognized data, add to dscPrintPanelStatus0, Byte 8: 0x00
 * 11101011 0 00000001 00000001 00000100 01100000 00010100 00000100 01000000 10000001 00101010 [0xEB] 2001.01.03 00:05 | Partition 1 | Zone tamper: 33
 * 11101011 0 00000001 00000001 00000100 01100000 00001000 00000100 01011111 10000001 00111101 [0xEB] 2001.01.03 00:02 | Partition 1 | Zone tamper: 64
 * 11101011 0 00000001 00000001 00000100 01100000 00011000 00000100 01100000 11111111 11001100 [0xEB] 2001.01.03 00:06 | Partition 1 | Zone tamper restored: 33
 * 11101011 0 00000000 00000001 00000100 01100000 01001000 00010100 01100000 10000001 10001101 [0xEB] 2001.01.03 00:18 | Zone fault: 33
 * 11101011 0 00000000 00000001 00000100 01100000 01001100 00010100 01000000 11111111 11101111 [0xEB] 2001.01.03 00:19 | Zone fault restored: 33
 * 11101011 0 00000000 00000001 00000100 01100000 00001100 00010100 01011111 11111111 11001110 [0xEB] 2001.01.03 00:03 | Zone fault restored: 64
 */
void dscPrintPanel_0xEB() {
  if (!dscValidCRC()) {
    printf("[CRC Error]");
    return;
  }

  byte dscYear3 = dscPanelData[3] >> 4;
  byte dscYear4 = dscPanelData[3] & 0x0F;
  byte dscMonth = dscPanelData[4] << 2; dscMonth >>=4;
  byte dscDay1 = dscPanelData[4] << 6; dscDay1 >>= 3;
  byte dscDay2 = dscPanelData[5] >> 5;
  byte dscDay = dscDay1 | dscDay2;
  byte dscHour = dscPanelData[5] & 0x1F;
  byte dscMinute = dscPanelData[6] >> 2;

  if (dscYear3 >= 7) printf("19");
  else printf("20");
  printf("%d%d.%02d.%02d %02d:%02d", dscYear3, dscYear4, dscMonth, dscDay, dscHour, dscMinute);

  /*printf("%d", dscYear3);
  printf("%d", dscYear4);
  printf(".");
  if (dscMonth < 10) printf("0");
  printf("%d", dscMonth);
  printf(".");
  if (dscDay < 10) printf("0");
  printf("%d", dscDay);
  printf(" ");
  if (dscHour < 10) printf("0");
  printf("%d", dscHour);
  printf(":");
  if (dscMinute < 10) printf("0");
  printf("%d", dscMinute);*/

  if (dscPanelData[2] == 0) printf(" | ");
  else {
    printf(" | Partition ");
    dscPrintPanelBitNumbers(2,1);
    printf("| ");
  }

  switch (dscPanelData[7]) {
    case 0x00: dscPrintPanelStatus0(8); return;
    case 0x01: dscPrintPanelStatus1(8); return;
    case 0x02: dscPrintPanelStatus2(8); return;
    case 0x03: dscPrintPanelStatus3(8); return;
    case 0x04: dscPrintPanelStatus4(8); return;
    case 0x14: dscPrintPanelStatus14(8); return;
  }
}


/*
 *  Print keypad and module messages
 */


/*
 *  Keypad: Fire alarm
 *
 *  01110111 1 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Fire alarm
 */
void dscPrintModule_0x77() {
  printf("[Keypad] Fire alarm");
}


/*
 *  Keypad: Auxiliary alarm
 *
 *  10111011 1 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Aux alarm
 */
void dscPrintModule_0xBB() {
  printf("[Keypad] Auxiliary alarm");
}


/*
 *  Keypad: Panic alarm
 *
 *  11011101 1 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Panic alarm
 */
void dscPrintModule_0xDD() {
  printf("[Keypad] Panic alarm");
}


/*
 *  Keybus status notifications
 */

void dscPrintModule_Notification() {
  switch (dscModuleData[4]) {
    // Zone expander: status update notification, panel responds with 0x28
    // 11111111 1 11111111 11111111 10111111 11111111 [Zone Expander] Status notification
    // 00101000 0 11111111 11111111 11111111 11111111 11111111 [0x28] Zone expander query
    case 0xBF:
      printf("[Zone Expander] Status notification");
      break;

    // Keypad: Unknown Keybus notification, panel responds with 0x4C query
    // 11111111 1 11111111 11111111 11111110 11111111 [Keypad] Unknown Keybus notification
    // 01001100 0 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 [0x4C] Unknown Keybus query
    case 0xFE:
      printf("[Keypad] Unknown Keybus notification");
      break;
  }

  switch (dscModuleData[5]) {
    // Keypad: zone status update notification, panel responds with 0xD5 query
    // 11111111 1 11111111 11111111 11111111 11111011 [Keypad] Zone status notification
    // 11010101 0 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 [0xD5] Keypad zone query
    case 0xFB:
      printf("[Keypad] Zone status notification");
      break;
  }
}


/*
 *  Keypad: Slot query response
 *
 *  00010001 0 10101010 10101010 10101010 10101010 10101010 [0x11] Keypad slot query
 *  11111111 1 00111111 11111111 11111111 11111111 11111111 [Keypad] Slots active: 1
 */
void dscPrintModule_Panel_0x11() {
  printf("[Keypad] Slots active: ");
  if ((dscModuleData[2] & 0xC0) == 0) printf("1 ");
  if ((dscModuleData[2] & 0x30) == 0) printf("2 ");
  if ((dscModuleData[2] & 0x0C) == 0) printf("3 ");
  if ((dscModuleData[2] & 0x03) == 0) printf("4 ");
  if ((dscModuleData[3] & 0xC0) == 0) printf("5 ");
  if ((dscModuleData[3] & 0x30) == 0) printf("6 ");
  if ((dscModuleData[3] & 0x0C) == 0) printf("7 ");
  if ((dscModuleData[3] & 0x03) == 0) printf("8 ");
}


/*
 *  Keypad: Panel 0xD5 zone query response
 *  Bytes 2-9: Keypad slots 1-8
 *  Bits 2,3: TBD
 *  Bits 6,7: TBD
 *
 *  11111111 1 11111111 11111111 11111111 11111011 [Keypad] Status update
 *  11010101 0 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010 [0xD5] Keypad zone query
 *
 *  11111111 1 00000011 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1 | Zone open
 *  11111111 1 00001111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1 | Zone open  // Exit *8 programming
 *  11111111 1 11111111 11111111 11111111 11111111 11111111 11111111 11111111 00001111 [Keypad] Slot 8 | Zone open  // Exit *8 programming
 *
 *  11111111 1 11110000 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1   //Zone closed while unconfigured
 *  11111111 1 11110011 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1   //Zone closed while unconfigured after opened once
 *  11111111 1 00110000 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1 | Zone closed // NC
 *  11111111 1 00111100 11111111 11111111 11111111 11111111 11111111 11111111 11111111 [Keypad] Slot 1 | Zone closed  //After exiting *8 programming after NC
 */
void dscPrintModule_Panel_0xD5() {
  printf("[Keypad] ");
  bool firstData = true;
  for (byte dscModuleByte = 2; dscModuleByte <= 9; dscModuleByte++) {
    byte slotData = dscModuleData[dscModuleByte];
    if (slotData < 0xFF) {
      if (firstData) printf("Slot ");
      else printf(" | Slot ");
      printf("%d", dscModuleByte - 1);
      if ((slotData & 0x03) == 0x03 && (slotData & 0x30) == 0) printf(" zone open");
      if ((slotData & 0x03) == 0 && (slotData & 0x30) == 0x30) printf(" zone closed");
      firstData = false;
    }
  }
}


/*
 *  Keypad: keys
 *
 *  11111111 1 00000101 11111111 11111111 11111111 [Keypad] 1
 *  11111111 1 00101101 11111111 11111111 11111111 [Keypad] #
 */
void dscPrintModule_Keys() {
  printf("[Keypad] ");

  byte keyByte = 2;
  if (dscCurrentCmd == 0x05) {
    if (dscModuleData[2] != 0xFF) {
      printf("Partition 1 | Key: ");
    }
    else if (dscModuleData[3] != 0xFF) {
      printf("Partition 2 | Key: ");
      keyByte = 3;
    }
    else if (dscModuleData[8] != 0xFF) {
      printf("Partition 3 | Key: ");
      keyByte = 8;
    }

    else if (dscModuleData[9] != 0xFF) {
      printf("Partition 4 | Key: ");
      keyByte = 9;
    }
  }
  else if (dscCurrentCmd == 0x1B) {
    if (dscModuleData[2] != 0xFF) {
      printf("Partition 5 | Key: ");
    }
    else if (dscModuleData[3] != 0xFF) {
      printf("Partition 6 | Key: ");
      keyByte = 3;
    }
    else if (dscModuleData[8] != 0xFF) {
      printf("Partition 7 | Key: ");
      keyByte = 8;
    }

    else if (dscModuleData[9] != 0xFF) {
      printf("Partition 8 | Key: ");
      keyByte = 9;
    }
  }

  switch (dscModuleData[keyByte]) {
    case 0x00: printf("0"); break;
    case 0x05: printf("1"); break;
    case 0x0A: printf("2"); break;
    case 0x0F: printf("3"); break;
    case 0x11: printf("4"); break;
    case 0x16: printf("5"); break;
    case 0x1B: printf("6"); break;
    case 0x1C: printf("7"); break;
    case 0x22: printf("8"); break;
    case 0x27: printf("9"); break;
    case 0x28: printf("*"); break;
    case 0x2D: printf("#"); break;
    case 0x52: printf("Identified voice prompt help"); break;
    case 0x70: printf("Command output 3"); break;
    case 0xAF: printf("Arm stay"); break;
    case 0xB1: printf("Arm away"); break;
    case 0xB6: printf("*9 No entry delay arm, requires access code"); break;
    case 0xBB: printf("Door chime configuration"); break;
    case 0xBC: printf("*6 System test"); break;
    case 0xC3: printf("*1 Zone bypass programming"); break;
    case 0xC4: printf("*2 Trouble menu"); break;
    case 0xC9: printf("*3 Alarm memory display"); break;
    case 0xCE: printf("*5 Programming, requires master code"); break;
    case 0xD0: printf("*6 Programming, requires master code"); break;
    case 0xD5: printf("Command output 1"); break;
    case 0xDA: printf("Reset / Command output 2"); break;
    case 0xDF: printf("General voice prompt help"); break;
    case 0xE1: printf("Quick exit"); break;
    case 0xE6: printf("Activate stay/away zones"); break;
    case 0xEB: printf("Function key [20] Future Use"); break;
    case 0xEC: printf("Command output 4"); break;
    case 0xF7: printf("Left/right arrow"); break;
    default:
      printf("Unrecognized data: 0x%02X", dscModuleData[keyByte]);
      break;
  }
}


/*
 * Print binary
 */

void dscPrintPanelBinary(bool printSpaces) {
  for (byte dscPanelByte = 0; dscPanelByte < dscPanelByteCount; dscPanelByte++) {
    if (dscPanelByte == 1) printf("%d", dscPanelData[dscPanelByte]);  // Prints the stop bit
    else {
      for (byte mask = 0x80; mask; mask >>= 1) {
        if (mask & dscPanelData[dscPanelByte]) printf("1");
        else printf("0");
      }
    }
    if (printSpaces && (dscPanelByte != dscPanelByteCount - 1)) printf(" ");
  }
}


void dscPrintModuleBinary(bool printSpaces) {
  for (byte dscModuleByte = 0; dscModuleByte < dscModuleByteCount; dscModuleByte++) {
    if (dscModuleByte == 1) printf("%d", dscModuleData[dscModuleByte]);  // Prints the stop bit
    else {
      for (byte mask = 0x80; mask; mask >>= 1) {
        if (mask & dscModuleData[dscModuleByte]) printf("1");
        else printf("0");
      }
    }
    if (printSpaces && (dscModuleByte != dscModuleByteCount - 1)) printf(" ");
  }
}


/*
 * Print panel command as hex
 */
void dscPrintPanelCommand() {
  // Prints the hex value of command byte 0
  printf("0x%02X", dscPanelData[0]);
}
