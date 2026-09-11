/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */
 #include <util/atomic.h>  
void createScreenMute() {
  resetSegments(1, 2);
  uint16_t muteSnap;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { muteSnap = muteTrack; }
  byte xseg = 0;
  byte xboard = 1;
  for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {
    segments[xboard][xseg] = bitRead(muteSnap, x) ? S_MUTE : S_UNMUTE;
    xseg++;
    if (xseg >= 8) {
      xseg = 0;
      xboard++;
    }

    if (noteLenCounters[x] > 0 || noteLenCountersLED[x] > 0) {
      // Turn on the respective LED based on track number
      if (x < 8) {
        bitSet(leds[1], x);  // Set bit for track 0-7 in leds[1]
      } else {
        bitSet(leds[2], x - 8);  // Set bit for track 8-15 in leds[2]
      }
    } else {
      // Turn off the respective LED based on track number
      if (x < 8) {
        bitClear(leds[1], x);  // Clear bit for track 0-7 in leds[1]
      } else {
        bitClear(leds[2], x - 8);  // Clear bit for track 8-15 in leds[2]
      }
    }
  }

  segments[0][4] = S_T;
}

void checkInterfaceMute() {
  for (byte x = 0; x < 8; x++) {
    for (byte i = 0; i < 2; i++) {

      if (buttonEvent[i + 1][x] == kButtonClicked || buttonEvent[i + 1][x] == kButtonHold) {
        uint8_t bitIndex = x + (i * 8);
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
         {
        if (bitRead(muteTrack, bitIndex))
          bitClear(muteTrack, bitIndex);
        else
          bitSet(muteTrack, bitIndex);
      }
      buttonEvent[i + 1][x] = 0;  // Clear button state
    }
  }
}

// check EXIT BUTTON
if (buttonEvent[0][7] > 0 && preventABCD == 1 && curRightScreen == kMuteMenu) {
  memset(buttonEvent, 0, sizeof(buttonEvent));  // Clear button events
  leds[1] = B00000000;                          // Clear track activity LEDs
  leds[2] = B00000000;

  preventABCD = 1;
  curRightScreen = kRightSteps;
} else if ((buttonEvent[0][7] == kButtonClicked || buttonEvent[0][7] > kButtonHold) && preventABCD == 0) {

  preventABCD = 1;
  buttonEvent[0][7] = 0;
}
}
//END OF LINE