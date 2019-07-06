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


// Resets the state of all status components as changed for sketches to get the current status
void dscResetStatus() {
  dscStatusChanged = true;
  dscKeybusChanged = true;
  dscTroubleChanged = true;
  dscPowerChanged = true;
  dscBatteryChanged = true;
  for (byte partition = 0; partition < dscPartitions; partition++) {
    dscReadyChanged[partition] = true;
    dscArmedChanged[partition] = true;
    dscAlarmChanged[partition] = true;
    dscFireChanged[partition] = true;
  }
  dscOpenZonesStatusChanged = true;
  dscAlarmZonesStatusChanged = true;
  for (byte zoneGroup = 0; zoneGroup < dscZones; zoneGroup++) {
    dscOpenZonesChanged[zoneGroup] = 0xFF;
    dscAlarmZonesChanged[zoneGroup] = 0xFF;
  }
}


// Sets the panel time
void dscSetTime(unsigned int dscYear, byte dscMonth, byte dscDay, byte dscHour, byte dscMinute, const char* dscAccessCode) {
  if (!dscReady[0]) return;  // Skips if partition 1 is not ready
  if (dscHour > 23 || dscMinute > 59 || dscMonth > 12 || dscDay > 31 || dscYear > 2099 || (dscYear > 99 && dscYear < 1900)) return;  // Skips if input date/time is invalid
  static char timeEntry[21];
  strcpy(timeEntry, "*6");
  strcat(timeEntry, dscAccessCode);
  strcat(timeEntry, "1");

  char timeChar[3];
  if (dscHour < 10) strcat(timeEntry, "0");
  itoa(dscHour, timeChar, 10);
  strcat(timeEntry, timeChar);

  if (dscMinute < 10) strcat(timeEntry, "0");
  itoa(dscMinute, timeChar, 10);
  strcat(timeEntry, timeChar);

  if (dscMonth < 10) strcat(timeEntry, "0");
  itoa(dscMonth, timeChar, 10);
  strcat(timeEntry, timeChar);

  if (dscDay < 10) strcat(timeEntry, "0");
  itoa(dscDay, timeChar, 10);
  strcat(timeEntry, timeChar);

  if (dscYear >= 2000) dscYear -= 2000;
  else if (dscYear >= 1900) dscYear -= 1900;
  if (dscYear < 10) strcat(timeEntry, "0");
  itoa(dscYear, timeChar, 10);
  strcat(timeEntry, timeChar);

  strcat(timeEntry, "#");
  dscWriteKeys(timeEntry);
}


// Processes status commands: 0x05 (Partitions 1-4) and 0x1B (Partitions 5-8)
void dscProcessPanelStatus() {

  // Trouble status
  if (bitRead(dscPanelData[2],4)) dscTrouble = true;
  else dscTrouble = false;
  if (dscTrouble != dscPreviousTrouble && (dscPanelData[3] < 0x05 || dscPanelData[3] == 0xC7)) {  // Ignores trouble light status in intermittent states
    dscPreviousTrouble = dscTrouble;
    dscTroubleChanged = true;
    if (!dscPauseStatus) dscStatusChanged = true;
  }

  byte partitionStart = 0;
  byte partitionCount = 0;
  if (dscPanelData[0] == 0x05) {
    partitionStart = 0;
    if (dscPanelByteCount < 9) partitionCount = 2;
    else partitionCount = 4;
    if (dscPartitions < partitionCount) partitionCount = dscPartitions;
  }
  else if (dscPartitions > 4 && dscPanelData[0] == 0x1B) {
    partitionStart = 4;
    partitionCount = 8;
  }

  for (byte partitionIndex = partitionStart; partitionIndex < partitionCount; partitionIndex++) {
    byte statusByte, messageByte;
    if (partitionIndex < 4) {
      statusByte = (partitionIndex * 2) + 2;
      messageByte = (partitionIndex * 2) + 3;
    }
    else {
      statusByte = ((partitionIndex - 4) * 2) + 2;
      messageByte = ((partitionIndex - 4) * 2) + 3;
    }

    // Status lights
    dscLights[partitionIndex] = dscPanelData[statusByte];
    if (dscLights[partitionIndex] != dscPreviousLights[partitionIndex]) {
      dscPreviousLights[partitionIndex] = dscLights[partitionIndex];
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    // Status messages
    dscStatus[partitionIndex] = dscPanelData[messageByte];
    if (dscStatus[partitionIndex] != dscPreviousStatus[partitionIndex]) {
      dscPreviousStatus[partitionIndex] = dscStatus[partitionIndex];
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    // Fire status
    if (bitRead(dscPanelData[statusByte],6)) dscFire[partitionIndex] = true;
    else dscFire[partitionIndex] = false;
    if (dscFire[partitionIndex] != dscPreviousFire[partitionIndex] && dscPanelData[messageByte] < 0x12) {  // Ignores fire light status in intermittent states
      dscPreviousFire[partitionIndex] = dscFire[partitionIndex];
      dscFireChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }


    // Messages
    switch (dscPanelData[messageByte]) {

      // Ready
      case 0x01:         // Partition ready
      case 0x02: {       // Stay/away zones open
        dscReady[partitionIndex] = true;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscEntryDelay[partitionIndex] = false;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscArmedStay[partitionIndex] = false;
        dscArmedAway[partitionIndex] = false;
        dscArmed[partitionIndex] = false;
        if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex]) {
          dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
          dscArmedChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Zones open
      case 0x03: {
        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscEntryDelay[partitionIndex] = false;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Armed
      case 0x04:         // Armed stay
      case 0x05: {       // Armed away
        if (dscPanelData[messageByte] == 0x04) {
          dscArmedStay[partitionIndex] = true;
          dscArmedAway[partitionIndex] = false;
        }
        else {
          dscArmedStay[partitionIndex] = false;
          dscArmedAway[partitionIndex] = true;
        }

        dscWriteArm[partitionIndex] = false;

        dscArmed[partitionIndex] = true;
        if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex] || dscArmedStay[partitionIndex] != dscPreviousArmedStay[partitionIndex]) {
          dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
          dscPreviousArmedStay[partitionIndex] = dscArmedStay[partitionIndex];
          dscArmedChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscExitDelay[partitionIndex] = false;
        if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
          dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
          dscExitDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscExitState[partitionIndex] = 0;

        dscEntryDelay[partitionIndex] = false;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Exit delay in progress
      case 0x08: {
        dscWriteArm[partitionIndex] = false;
        dscAccessCodePrompt = false;

        dscExitDelay[partitionIndex] = true;
        if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
          dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
          dscExitDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        if (dscExitState[partitionIndex] != DSC_EXIT_NO_ENTRY_DELAY) {
          if (bitRead(dscLights[partitionIndex],3)) dscExitState[partitionIndex] = DSC_EXIT_STAY;
          else dscExitState[partitionIndex] = DSC_EXIT_AWAY;
          if (dscExitState[partitionIndex] != dscPreviousExitState[partitionIndex]) {
            dscPreviousExitState[partitionIndex] = dscExitState[partitionIndex];
            dscExitDelayChanged[partitionIndex] = true;
            dscExitStateChanged[partitionIndex] = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }

        dscReady[partitionIndex] = true;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Arming with no entry delay
      case 0x09: {
        dscReady[partitionIndex] = true;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscExitState[partitionIndex] = DSC_EXIT_NO_ENTRY_DELAY;
        break;
      }

      // Entry delay in progress
      case 0x0C: {
        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscEntryDelay[partitionIndex] = true;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Partition in alarm
      case 0x11: {
        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscEntryDelay[partitionIndex] = false;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscAlarm[partitionIndex] = true;
        if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
          dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
          dscAlarmChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Partition armed with no entry delay
      case 0x16: {
        dscNoEntryDelay[partitionIndex] = true;

        // Sets an armed mode if not already set, used if interface is initialized while the panel is armed
        if (!dscArmedStay[partitionIndex] && !dscArmedAway[partitionIndex]) dscArmedStay[partitionIndex] = true;

        dscArmed[partitionIndex] = true;
        if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex]) {
          dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
          dscPreviousArmedStay[partitionIndex] = dscArmedStay[partitionIndex];
          dscArmedChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Partition disarmed
      case 0x3E: {
        dscExitDelay[partitionIndex] = false;
        if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
          dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
          dscExitDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscExitState[partitionIndex] = 0;

        dscEntryDelay[partitionIndex] = false;
        if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
          dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
          dscEntryDelayChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscAlarm[partitionIndex] = false;
        if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
          dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
          dscAlarmChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Invalid access code
      case 0x8F: {
        if (!dscArmed[partitionIndex]) {
          dscReady[partitionIndex] = true;
          if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
            dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
            dscReadyChanged[partitionIndex] = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        break;
      }

      // Enter * function code
      case 0x9E: {
        dscWroteAsterisk = false;  // Resets the flag that delays writing after '*' is pressed
        dscWriteAsterisk = false;
        dscPanelKeyPending = false;
        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      // Enter access code
      case 0x9F: {
        if (dscWriteArm[partitionIndex]) {  // Ensures access codes are only sent when an arm command is sent through this interface
          dscAccessCodePrompt = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }

        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }

      default: {
        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        break;
      }
    }
  }
}


// Panel status and zones 1-8 status
void dscProcessPanel_0x27() {
  if (!dscValidCRC()) return;

  for (byte partitionIndex = 0; partitionIndex < 2; partitionIndex++) {
    byte messageByte = (partitionIndex * 2) + 3;

    // Messages
    if (dscPanelData[messageByte] == 0x04 || dscPanelData[messageByte] == 0x05) {

      dscReady[partitionIndex] = false;
      if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
        dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
        dscReadyChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }

      if (dscPanelData[messageByte] == 0x04) {
        dscArmedStay[partitionIndex] = true;
        dscArmedAway[partitionIndex] = false;
      }
      else if (dscPanelData[messageByte] == 0x05) {
        dscArmedStay[partitionIndex] = false;
        dscArmedAway[partitionIndex] = true;
      }

      dscArmed[partitionIndex] = true;
      if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex] || dscArmedStay[partitionIndex] != dscPreviousArmedStay[partitionIndex]) {
        dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
        dscPreviousArmedStay[partitionIndex] = dscArmedStay[partitionIndex];
        dscArmedChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }

      dscExitDelay[partitionIndex] = false;
      if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
        dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
        dscExitDelayChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }

      dscExitState[partitionIndex] = 0;
    }

    // Armed with no entry delay
    else if (dscPanelData[messageByte] == 0x16) {
      dscNoEntryDelay[partitionIndex] = true;

      // Sets an armed mode if not already set, used if interface is initialized while the panel is armed
      if (!dscArmedStay[partitionIndex] && !dscArmedAway[partitionIndex]) dscArmedStay[partitionIndex] = true;

      dscArmed[partitionIndex] = true;
      if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex]) {
        dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
        dscPreviousArmedStay[partitionIndex] = dscArmedStay[partitionIndex];
        dscArmedChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }

      dscExitDelay[partitionIndex] = false;
      if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
        dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
        dscExitDelayChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }

      dscExitState[partitionIndex] = 0;

      dscReady[partitionIndex] = false;
      if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
        dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
        dscReadyChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
      }
    }
  }

  // Open zones 1-8 status is stored in dscOpenZones[0] and dscOpenZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
  dscOpenZones[0] = dscPanelData[6];
  byte zonesChanged = dscOpenZones[0] ^ dscPreviousOpenZones[0];
  if (zonesChanged != 0) {
    dscPreviousOpenZones[0] = dscOpenZones[0];
    dscOpenZonesStatusChanged = true;
    if (!dscPauseStatus) dscStatusChanged = true;

    for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
      if (bitRead(zonesChanged, zoneBit)) {
        bitWrite(dscOpenZonesChanged[0], zoneBit, 1);
        if (bitRead(dscPanelData[6], zoneBit)) bitWrite(dscOpenZones[0], zoneBit, 1);
        else bitWrite(dscOpenZones[0], zoneBit, 0);
      }
    }
  }
}


// Zones 9-16 status
void dscProcessPanel_0x2D() {
  if (!dscValidCRC()) return;
  if (dscZones < 2) return;

  // Open zones 9-16 status is stored in dscOpenZones[1] and dscOpenZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
  dscOpenZones[1] = dscPanelData[6];
  byte zonesChanged = dscOpenZones[1] ^ dscPreviousOpenZones[1];
  if (zonesChanged != 0) {
    dscPreviousOpenZones[1] = dscOpenZones[1];
    dscOpenZonesStatusChanged = true;
    if (!dscPauseStatus) dscStatusChanged = true;

    for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
      if (bitRead(zonesChanged, zoneBit)) {
        bitWrite(dscOpenZonesChanged[1], zoneBit, 1);
        if (bitRead(dscPanelData[6], zoneBit)) bitWrite(dscOpenZones[1], zoneBit, 1);
        else bitWrite(dscOpenZones[1], zoneBit, 0);
      }
    }
  }
}


// Zones 17-24 status
void dscProcessPanel_0x34() {
  if (!dscValidCRC()) return;
  if (dscZones < 3) return;

  // Open zones 17-24 status is stored in dscOpenZones[2] and dscOpenZonesChanged[2]: Bit 0 = Zone 17 ... Bit 7 = Zone 24
  dscOpenZones[2] = dscPanelData[6];
  byte zonesChanged = dscOpenZones[2] ^ dscPreviousOpenZones[2];
  if (zonesChanged != 0) {
    dscPreviousOpenZones[2] = dscOpenZones[2];
    dscOpenZonesStatusChanged = true;
    if (!dscPauseStatus) dscStatusChanged = true;

    for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
      if (bitRead(zonesChanged, zoneBit)) {
        bitWrite(dscOpenZonesChanged[2], zoneBit, 1);
        if (bitRead(dscPanelData[6], zoneBit)) bitWrite(dscOpenZones[2], zoneBit, 1);
        else bitWrite(dscOpenZones[2], zoneBit, 0);
      }
    }
  }
}


// Zones 25-32 status
void dscProcessPanel_0x3E() {
  if (!dscValidCRC()) return;
  if (dscZones < 4) return;

  // Open zones 25-32 status is stored in dscOpenZones[3] and dscOpenZonesChanged[3]: Bit 0 = Zone 25 ... Bit 7 = Zone 32
  dscOpenZones[3] = dscPanelData[6];
  byte zonesChanged = dscOpenZones[3] ^ dscPreviousOpenZones[3];
  if (zonesChanged != 0) {
    dscPreviousOpenZones[3] = dscOpenZones[3];
    dscOpenZonesStatusChanged = true;
    if (!dscPauseStatus) dscStatusChanged = true;

    for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
      if (bitRead(zonesChanged, zoneBit)) {
        bitWrite(dscOpenZonesChanged[3], zoneBit, 1);
        if (bitRead(dscPanelData[6], zoneBit)) bitWrite(dscOpenZones[3], zoneBit, 1);
        else bitWrite(dscOpenZones[3], zoneBit, 0);
      }
    }
  }
}


void dscProcessPanel_0xA5() {
  if (!dscValidCRC()) return;

  byte dscYear3 = dscPanelData[2] >> 4;
  byte dscYear4 = dscPanelData[2] & 0x0F;
  dscYear = (dscYear3 * 10) + dscYear4;
  if (dscYear3 >= 7) dscYear += 1900;
  else dscYear += 2000;
  dscMonth = dscPanelData[3] << 2; dscMonth >>= 4;
  byte dscDay1 = dscPanelData[3] << 6; dscDay1 >>= 3;
  byte dscDay2 = dscPanelData[4] >> 5;
  dscDay = dscDay1 | dscDay2;
  dscHour = dscPanelData[4] & 0x1F;
  dscMinute = dscPanelData[5] >> 2;

  // Timestamp
  if (dscPanelData[6] == 0 && dscPanelData[7] == 0) {
    dscStatusChanged = true;
    dscTimestampChanged = true;
    return;
  }

  byte partition = dscPanelData[3] >> 6;
  switch (dscPanelData[5] & 0x03) {
    case 0x00: dscProcessPanelStatus0(partition, 6); break;
    case 0x02: dscProcessPanelStatus2(partition, 6); break;
  }
}


void dscProcessPanel_0xEB() {
  if (!dscValidCRC()) return;
  if (dscPartitions < 3) return;

  byte dscYear3 = dscPanelData[3] >> 4;
  byte dscYear4 = dscPanelData[3] & 0x0F;
  dscYear = (dscYear3 * 10) + dscYear4;
  if (dscYear3 >= 7) dscYear += 1900;
  else dscYear += 2000;
  dscMonth = dscPanelData[4] << 2; dscMonth >>=4;
  byte dscDay1 = dscPanelData[4] << 6; dscDay1 >>= 3;
  byte dscDay2 = dscPanelData[5] >> 5;
  dscDay = dscDay1 | dscDay2;
  dscHour = dscPanelData[5] & 0x1F;
  dscMinute = dscPanelData[6] >> 2;

  byte partition;
  switch (dscPanelData[2]) {
    case 0x01: partition = 1; break;
    case 0x02: partition = 2; break;
    case 0x04: partition = 3; break;
    case 0x08: partition = 4; break;
    case 0x10: partition = 5; break;
    case 0x20: partition = 6; break;
    case 0x40: partition = 7; break;
    case 0x80: partition = 8; break;
    default: partition = 0; break;
  }

  switch (dscPanelData[7] & 0x07) {
    case 0x00: dscProcessPanelStatus0(partition, 8); break;
    case 0x02: dscProcessPanelStatus2(partition, 8); break;
    case 0x04: dscProcessPanelStatus4(partition, 8); break;
  }
}


void dscProcessPanelStatus0(byte partition, byte dscPanelByte) {

  // Processes status messages that are not partition-specific
  if (partition == 0 && dscPanelData[0] == 0xA5) {
    switch (dscPanelData[dscPanelByte]) {

      // Keypad Fire alarm
      case 0x4E: {
        dscKeypadFireAlarm = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }

      // Keypad Aux alarm
      case 0x4F: {
        dscKeypadAuxAlarm = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }

      // Keypad Panic alarm
      case 0x50: {
        dscKeypadPanicAlarm = true;
        if (!dscPauseStatus) dscStatusChanged =true;
        return;
      }

      // Panel battery trouble
      case 0xE7: {
        dscBatteryTrouble = true;
        dscBatteryChanged = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }

      // Panel AC power failure
      case 0xE8: {
        dscPowerTrouble = true;
        dscPowerChanged = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }

      // Panel battery restored
      case 0xEF: {
        dscBatteryTrouble = false;
        dscBatteryChanged = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }

      // Panel AC power restored
      case 0xF0: {
        dscPowerTrouble = false;
        dscPowerChanged = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }
      default: return;
    }
  }

  // Processes partition-specific status
  if (partition > dscPartitions) return;  // Ensures that only the configured number of partitions are processed
  byte partitionIndex = partition - 1;

  // Disarmed
  if (dscPanelData[dscPanelByte] == 0x4A ||                                    // Disarmed after alarm in memory
      dscPanelData[dscPanelByte] == 0xE6 ||                                    // Disarmed special: keyswitch/wireless key/DLS
      (dscPanelData[dscPanelByte] >= 0xC0 && dscPanelData[dscPanelByte] <= 0xE4)) {  // Disarmed by access code

    dscNoEntryDelay[partitionIndex] = false;

    dscArmedAway[partitionIndex] = false;
    dscArmedStay[partitionIndex] = false;
    dscArmed[partitionIndex] = false;
    if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex]) {
      dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
      dscArmedChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscAlarm[partitionIndex] = false;
    if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
      dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
      dscAlarmChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscEntryDelay[partitionIndex] = false;
    if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
      dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
      dscEntryDelayChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }
    return;
  }

  // Partition in alarm
  if (dscPanelData[dscPanelByte] == 0x4B) {
    dscAlarm[partitionIndex] = true;
    if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
      dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
      dscAlarmChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }
    return;
  }

  // Zone alarm, zones 1-32
  // Zone alarm status is stored using 1 bit per zone in dscAlarmZones[] and dscAlarmZonesChanged[]:
  //   dscAlarmZones[0] and dscAlarmZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
  //   dscAlarmZones[1] and dscAlarmZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
  //   ...
  //   dscAlarmZones[7] and dscAlarmZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
  if (dscPanelData[dscPanelByte] >= 0x09 && dscPanelData[dscPanelByte] <= 0x28) {
    dscAlarm[partitionIndex] = true;
    if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
      dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
      dscAlarmChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscEntryDelay[partitionIndex] = false;
    if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
      dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
      dscEntryDelayChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    byte maxZones = dscZones * 8;
    if (maxZones > 32) maxZones = 32;
    for (byte zoneCount = 0; zoneCount < maxZones; zoneCount++) {
      if (dscPanelData[dscPanelByte] == 0x09 + zoneCount) {
        if (zoneCount < 8) {
          bitWrite(dscAlarmZones[0], zoneCount, 1);
          if (bitRead(dscPreviousAlarmZones[0], zoneCount) != 1) {
            bitWrite(dscPreviousAlarmZones[0], zoneCount, 1);
            bitWrite(dscAlarmZonesChanged[0], zoneCount, 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 8 && zoneCount < 16) {
          bitWrite(dscAlarmZones[1], (zoneCount - 8), 1);
          if (bitRead(dscPreviousAlarmZones[1], (zoneCount - 8)) != 1) {
            bitWrite(dscPreviousAlarmZones[1], (zoneCount - 8), 1);
            bitWrite(dscAlarmZonesChanged[1], (zoneCount - 8), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 16 && zoneCount < 24) {
          bitWrite(dscAlarmZones[2], (zoneCount - 16), 1);
          if (bitRead(dscPreviousAlarmZones[2], (zoneCount - 16)) != 1) {
            bitWrite(dscPreviousAlarmZones[2], (zoneCount - 16), 1);
            bitWrite(dscAlarmZonesChanged[2], (zoneCount - 16), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 24 && zoneCount < 32) {
          bitWrite(dscAlarmZones[3], (zoneCount - 24), 1);
          if (bitRead(dscPreviousAlarmZones[3], (zoneCount - 24)) != 1) {
            bitWrite(dscPreviousAlarmZones[3], (zoneCount - 24), 1);
            bitWrite(dscAlarmZonesChanged[3], (zoneCount - 24), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
      }
    }
    return;
  }

  // Zone alarm restored, zones 1-32
  // Zone alarm status is stored using 1 bit per zone in dscAlarmZones[] and dscAlarmZonesChanged[]:
  //   dscAlarmZones[0] and dscAlarmZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
  //   dscAlarmZones[1] and dscAlarmZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
  //   ...
  //   dscAlarmZones[7] and dscAlarmZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
  if (dscPanelData[dscPanelByte] >= 0x29 && dscPanelData[dscPanelByte] <= 0x48) {

    byte maxZones = dscZones * 8;
    if (maxZones > 32) maxZones = 32;
    for (byte zoneCount = 0; zoneCount < maxZones; zoneCount++) {
      if (dscPanelData[dscPanelByte] == 0x29 + zoneCount) {
        if (zoneCount < 8) {
          bitWrite(dscAlarmZones[0], zoneCount, 0);
          if (bitRead(dscPreviousAlarmZones[0], zoneCount) != 0) {
            bitWrite(dscPreviousAlarmZones[0], zoneCount, 0);
            bitWrite(dscAlarmZonesChanged[0], zoneCount, 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 8 && zoneCount < 16) {
          bitWrite(dscAlarmZones[1], (zoneCount - 8), 0);
          if (bitRead(dscPreviousAlarmZones[1], (zoneCount - 8)) != 0) {
            bitWrite(dscPreviousAlarmZones[1], (zoneCount - 8), 0);
            bitWrite(dscAlarmZonesChanged[1], (zoneCount - 8), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 16 && zoneCount < 24) {
          bitWrite(dscAlarmZones[2], (zoneCount - 16), 0);
          if (bitRead(dscPreviousAlarmZones[2], (zoneCount - 16)) != 0) {
            bitWrite(dscPreviousAlarmZones[2], (zoneCount - 16), 0);
            bitWrite(dscAlarmZonesChanged[2], (zoneCount - 16), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
        else if (zoneCount >= 24 && zoneCount < 32) {
          bitWrite(dscAlarmZones[3], (zoneCount - 24), 0);
          if (bitRead(dscPreviousAlarmZones[3], (zoneCount - 24)) != 0) {
            bitWrite(dscPreviousAlarmZones[3], (zoneCount - 24), 0);
            bitWrite(dscAlarmZonesChanged[3], (zoneCount - 24), 1);
            dscAlarmZonesStatusChanged = true;
            if (!dscPauseStatus) dscStatusChanged = true;
          }
        }
      }
    }
    return;
  }

  //Armed by access code
  if (dscPanelData[dscPanelByte] >= 0x99 && dscPanelData[dscPanelByte] <= 0xBD) {
    dscAccessCode[partitionIndex] = dscPanelData[dscPanelByte] - 0x98;
    if (dscAccessCode[partitionIndex] >= 35) dscAccessCode[partitionIndex] += 5;
    if (dscAccessCode[partitionIndex] != dscPreviousAccessCode[partitionIndex]) {
      dscPreviousAccessCode[partitionIndex] = dscAccessCode[partitionIndex];
      dscAccessCodeChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }
    return;
  }

  // Disarmed by access code
  if (dscPanelData[dscPanelByte] >= 0xC0 && dscPanelData[dscPanelByte] <= 0xE4) {
    dscAccessCode[partitionIndex] = dscPanelData[dscPanelByte] - 0xBF;
    if (dscAccessCode[partitionIndex] >= 35) dscAccessCode[partitionIndex] += 5;
    if (dscAccessCode[partitionIndex] != dscPreviousAccessCode[partitionIndex]) {
      dscPreviousAccessCode[partitionIndex] = dscAccessCode[partitionIndex];
      dscAccessCodeChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }
    return;
  }
}


void dscProcessPanelStatus2(byte partition, byte dscPanelByte) {
  if (partition == 0 || partition > dscPartitions) return;
  byte partitionIndex = partition - 1;

  // Armed: stay and Armed: away
  if (dscPanelData[dscPanelByte] == 0x9A || dscPanelData[dscPanelByte] == 0x9B) {
    if (dscPanelData[dscPanelByte] == 0x9A) {
      dscArmedStay[partitionIndex] = true;
      dscArmedAway[partitionIndex] = false;
    }
    else if (dscPanelData[dscPanelByte] == 0x9B) {
      dscArmedStay[partitionIndex] = false;
      dscArmedAway[partitionIndex] = true;
    }

    dscArmed[partitionIndex] = true;
    if (dscArmed[partitionIndex] != dscPreviousArmed[partitionIndex] || dscArmedStay[partitionIndex] != dscPreviousArmedStay[partitionIndex]) {
      dscPreviousArmed[partitionIndex] = dscArmed[partitionIndex];
      dscPreviousArmedStay[partitionIndex] = dscArmedStay[partitionIndex];
      dscArmedChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscExitDelay[partitionIndex] = false;
    if (dscExitDelay[partitionIndex] != dscPreviousExitDelay[partitionIndex]) {
      dscPreviousExitDelay[partitionIndex] = dscExitDelay[partitionIndex];
      dscExitDelayChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscExitState[partitionIndex] = 0;

    dscReady[partitionIndex] = false;
    if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
      dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
      dscReadyChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }
    return;
  }

  if (dscPanelData[dscPanelByte] == 0xA5) {
    switch (dscPanelData[dscPanelByte]) {
      case 0x99: {        // Activate stay/away zones
        dscArmed[partitionIndex] = true;
        dscArmedAway[partitionIndex] = true;
        dscArmedStay[partitionIndex] = false;
        dscArmedChanged[partitionIndex] = true;
        if (!dscPauseStatus) dscStatusChanged = true;
        return;
      }
      case 0x9C: {        // Armed with no entry delay
        dscNoEntryDelay[partitionIndex] = true;

        dscReady[partitionIndex] = false;
        if (dscReady[partitionIndex] != dscPreviousReady[partitionIndex]) {
          dscPreviousReady[partitionIndex] = dscReady[partitionIndex];
          dscReadyChanged[partitionIndex] = true;
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        return;
      }
    }
  }
}


void dscProcessPanelStatus4(byte partition, byte dscPanelByte) {
  if (partition == 0 || partition > dscPartitions) return;
  byte partitionIndex = partition - 1;

  // Zone alarm, zones 33-64
  // Zone alarm status is stored using 1 bit per zone in dscAlarmZones[] and dscAlarmZonesChanged[]:
  //   dscAlarmZones[0] and dscAlarmZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
  //   dscAlarmZones[1] and dscAlarmZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
  //   ...
  //   dscAlarmZones[7] and dscAlarmZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
  if (dscPanelData[dscPanelByte] <= 0x1F) {
    dscAlarmZonesStatusChanged = true;

    dscAlarm[partitionIndex] = true;
    if (dscAlarm[partitionIndex] != dscPreviousAlarm[partitionIndex]) {
      dscPreviousAlarm[partitionIndex] = dscAlarm[partitionIndex];
      dscAlarmChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    dscEntryDelay[partitionIndex] = false;
    if (dscEntryDelay[partitionIndex] != dscPreviousEntryDelay[partitionIndex]) {
      dscPreviousEntryDelay[partitionIndex] = dscEntryDelay[partitionIndex];
      dscEntryDelayChanged[partitionIndex] = true;
      if (!dscPauseStatus) dscStatusChanged = true;
    }

    byte maxZones = dscZones * 8;
    if (maxZones > 32) maxZones -= 32;
    else return;
    for (byte zoneCount = 0; zoneCount < maxZones; zoneCount++) {
      if (dscPanelData[dscPanelByte] == zoneCount) {
        if (zoneCount < 8) {
          bitWrite(dscAlarmZones[4], zoneCount, 1);
          bitWrite(dscAlarmZonesChanged[4], zoneCount, 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 8 && zoneCount < 16) {
          bitWrite(dscAlarmZones[5], (zoneCount - 8), 1);
          bitWrite(dscAlarmZonesChanged[5], (zoneCount - 8), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 16 && zoneCount < 24) {
          bitWrite(dscAlarmZones[6], (zoneCount - 16), 1);
          bitWrite(dscAlarmZonesChanged[6], (zoneCount - 16), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 24 && zoneCount < 32) {
          bitWrite(dscAlarmZones[7], (zoneCount - 24), 1);
          bitWrite(dscAlarmZonesChanged[7], (zoneCount - 24), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
      }
    }
    return;
  }

  // Zone alarm restored, zones 33-64
  // Zone alarm status is stored using 1 bit per zone in dscAlarmZones[] and dscAlarmZonesChanged[]:
  //   dscAlarmZones[0] and dscAlarmZonesChanged[0]: Bit 0 = Zone 1 ... Bit 7 = Zone 8
  //   dscAlarmZones[1] and dscAlarmZonesChanged[1]: Bit 0 = Zone 9 ... Bit 7 = Zone 16
  //   ...
  //   dscAlarmZones[7] and dscAlarmZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
  if (dscPanelData[dscPanelByte] >= 0x20 && dscPanelData[dscPanelByte] <= 0x3F) {
    dscAlarmZonesStatusChanged = true;

    byte maxZones = dscZones * 8;
    if (maxZones > 32) maxZones -= 32;
    else return;
    for (byte zoneCount = 0; zoneCount < maxZones; zoneCount++) {
      if (dscPanelData[dscPanelByte] == 0x20 + zoneCount) {
        if (zoneCount < 8) {
          bitWrite(dscAlarmZones[4], zoneCount, 0);
          bitWrite(dscAlarmZonesChanged[4], zoneCount, 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 8 && zoneCount < 16) {
          bitWrite(dscAlarmZones[5], (zoneCount - 8), 0);
          bitWrite(dscAlarmZonesChanged[5], (zoneCount - 8), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 16 && zoneCount < 24) {
          bitWrite(dscAlarmZones[6], (zoneCount - 16), 0);
          bitWrite(dscAlarmZonesChanged[6], (zoneCount - 16), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
        else if (zoneCount >= 24 && zoneCount < 32) {
          bitWrite(dscAlarmZones[7], (zoneCount - 24), 0);
          bitWrite(dscAlarmZonesChanged[7], (zoneCount - 24), 1);
          if (!dscPauseStatus) dscStatusChanged = true;
        }
      }
    }
    return;
  }

}


// Processes zones 33-64 status
void dscProcessPanel_0xE6() {
  if (!dscValidCRC()) return;

  switch (dscPanelData[2]) {
    case 0x09: dscProcessPanel_0xE6_0x09(); return;
    case 0x0B: dscProcessPanel_0xE6_0x0B(); return;
    case 0x0D: dscProcessPanel_0xE6_0x0D(); return;
    case 0x0F: dscProcessPanel_0xE6_0x0F(); return;
  }
}


// Open zones 33-40 status is stored in dscOpenZones[4] and dscOpenZonesChanged[4]: Bit 0 = Zone 33 ... Bit 7 = Zone 40
void dscProcessPanel_0xE6_0x09() {
  if (dscZones > 4) {
    dscOpenZones[4] = dscPanelData[3];
    byte zonesChanged = dscOpenZones[4] ^ dscPreviousOpenZones[4];
    if (zonesChanged != 0) {
      dscPreviousOpenZones[4] = dscOpenZones[4];
      dscOpenZonesStatusChanged = true;
      if (!dscPauseStatus) dscStatusChanged = true;

      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(zonesChanged, zoneBit)) {
          bitWrite(dscOpenZonesChanged[4], zoneBit, 1);
          if (bitRead(dscPanelData[3], zoneBit)) bitWrite(dscOpenZones[4], zoneBit, 1);
          else bitWrite(dscOpenZones[4], zoneBit, 0);
        }
      }
    }
  }
}


// Open zones 41-48 status is stored in dscOpenZones[5] and dscOpenZonesChanged[5]: Bit 0 = Zone 41 ... Bit 7 = Zone 48
void dscProcessPanel_0xE6_0x0B() {
  if (dscZones > 5) {
    dscOpenZones[5] = dscPanelData[3];
    byte zonesChanged = dscOpenZones[5] ^ dscPreviousOpenZones[5];
    if (zonesChanged != 0) {
      dscPreviousOpenZones[5] = dscOpenZones[5];
      dscOpenZonesStatusChanged = true;
      if (!dscPauseStatus) dscStatusChanged = true;

      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(zonesChanged, zoneBit)) {
          bitWrite(dscOpenZonesChanged[5], zoneBit, 1);
          if (bitRead(dscPanelData[3], zoneBit)) bitWrite(dscOpenZones[5], zoneBit, 1);
          else bitWrite(dscOpenZones[5], zoneBit, 0);
        }
      }
    }
  }
}


// Open zones 49-56 status is stored in dscOpenZones[6] and dscOpenZonesChanged[6]: Bit 0 = Zone 49 ... Bit 7 = Zone 56
void dscProcessPanel_0xE6_0x0D() {
  if (dscZones > 6) {
    dscOpenZones[6] = dscPanelData[3];
    byte zonesChanged = dscOpenZones[6] ^ dscPreviousOpenZones[6];
    if (zonesChanged != 0) {
      dscPreviousOpenZones[6] = dscOpenZones[6];
      dscOpenZonesStatusChanged = true;
      if (!dscPauseStatus) dscStatusChanged = true;

      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(zonesChanged, zoneBit)) {
          bitWrite(dscOpenZonesChanged[6], zoneBit, 1);
          if (bitRead(dscPanelData[3], zoneBit)) bitWrite(dscOpenZones[6], zoneBit, 1);
          else bitWrite(dscOpenZones[6], zoneBit, 0);
        }
      }
    }
  }
}


// Open zones 57-64 status is stored in dscOpenZones[7] and dscOpenZonesChanged[7]: Bit 0 = Zone 57 ... Bit 7 = Zone 64
void dscProcessPanel_0xE6_0x0F() {
  if (dscZones > 7) {
    dscOpenZones[7] = dscPanelData[3];
    byte zonesChanged = dscOpenZones[7] ^ dscPreviousOpenZones[7];
    if (zonesChanged != 0) {
      dscPreviousOpenZones[7] = dscOpenZones[7];
      dscOpenZonesStatusChanged = true;
      if (!dscPauseStatus) dscStatusChanged = true;

      for (byte zoneBit = 0; zoneBit < 8; zoneBit++) {
        if (bitRead(zonesChanged, zoneBit)) {
          bitWrite(dscOpenZonesChanged[7], zoneBit, 1);
          if (bitRead(dscPanelData[3], zoneBit)) bitWrite(dscOpenZones[7], zoneBit, 1);
          else bitWrite(dscOpenZones[7], zoneBit, 0);
        }
      }
    }
  }
}
