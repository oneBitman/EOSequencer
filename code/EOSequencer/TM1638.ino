/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */

#define BUTTON_HOLD 20  // time to register a long button hold
#define BUTTON_CLICK 1  // time to debounce a single click

// The Following Values are Hardcoded using direct port manipulation //
//#define STROBE 8    //#define CLOCK 9   //#define SETIO1 5    //#define SETIO2 6    //#define SETIO3 7

#define CLOCK_LOW PORTB &= B11111101
#define STROBE_LOW PORTB &= B11111110
#define SETI123_LOW PORTD &= B00011111
#define SETI01_LOW PORTD &= B11011111
#define SETI02_LOW PORTD &= B10111111
#define SETI03_LOW PORTD &= B01111111

#define S_C_HIGH PORTB |= B00000011
#define CLOCK_HIGH PORTB |= B00000010
#define STROBE_HIGH PORTB |= B00000001
#define SETI123_HIGH PORTD |= B11100000
#define SETI01_HIGH PORTD |= B00100000
#define SETI02_HIGH PORTD |= B01000000
#define SETI03_HIGH PORTD |= B10000000

void initTM1638() {
  DDRB = B11101111;   // PB4 is INPUT for MISO on Flash SPI //
  PORTB = B11111111;  // Also set SS Flash HIGH //
  DDRD = B11111111;

  S_C_HIGH;
  SETI123_LOW;

  sendData(0b10001010);

  sendData(0x40);  // set auto increment mode
  STROBE_LOW;
  sendDataConst(0xc0, 0xc0, 0xc0);  // Start Address
  SETI123_LOW;
  for (uint8_t i = 0; i < (16 * 8); i++) {
    CLOCK_LOW;
    waitMs(0);
    CLOCK_HIGH;
  }
  STROBE_HIGH;
}

void sendData(byte data) {
  STROBE_LOW;
  for (byte x = 0; x < 8; x++) {
    CLOCK_LOW;
    if (bitRead(data, x) == 0) SETI123_LOW;
    else SETI123_HIGH;
    CLOCK_HIGH;
  }
  STROBE_HIGH;
}

void sendDataConst(byte data1, byte data2, byte data3) {
  for (byte x = 0; x < 8; x++) {
    CLOCK_LOW;
    //
    PORTD &= B00011111;
    if (bitRead(data1, x) == 1) PORTD |= B00100000;
    if (bitRead(data2, x) == 1) PORTD |= B01000000;
    if (bitRead(data3, x) == 1) PORTD |= B10000000;
    //
    CLOCK_HIGH;
  }
}

void sendScreen() {
  // Get the corresponding PWM byte value for the current brightness setting
  byte pwmValue = pwmBrightness[globalData.Brightness];
  //Send the PWM value to the display to adjust brightness
  if (startup) pwmValue = 0b10001010;
  sendData(pwmValue);  // Update the brightness display using the correct PWM value

  sendData(0x40);  // set auto increment mode
  STROBE_LOW;
  sendDataConst(0xc0, 0xc0, 0xc0);  // Start Address
  for (byte x = 0; x < 8; x++) {
    sendDataConst(segments[0][x], segments[1][x], segments[2][x]);
    sendDataConst(bitRead(leds[0], x), bitRead(leds[1], x), bitRead(leds[2], x));
  }
  STROBE_HIGH;
}

void readButtons(void) {
  STROBE_LOW;
  sendDataConst(0x42, 0x42, 0x42);  // Request Button States

  DDRD = B00000000;

  buttons[0] = buttons[1] = buttons[2] = 0x00;
  for (byte i = 0; i < 4; i++) {
    byte v[3] = { 0x00, 0x00, 0x00 };
    for (byte x = 0; x < 8; x++) {
      CLOCK_LOW;
      bitWrite(v[0], x, bitRead(PIND, 5));
      bitWrite(v[1], x, bitRead(PIND, 6));
      bitWrite(v[2], x, bitRead(PIND, 7));
      CLOCK_HIGH;
    }
    buttons[0] |= v[0] << i;
    buttons[1] |= v[1] << i;
    buttons[2] |= v[2] << i;
  }

  DDRD = B11111111;

  STROBE_HIGH;

  if (startup) return;  //to avoid triggering steps when selecting memory slot
  
  for (byte x = 0; x < 8; x++) {
    for (byte i = 0; i < 3; i++) {
      if (bitRead(buttons[i], x) && (!ignoreButtons || (i == 0 && x <= 1 && curRightScreen != kMuteMenu))) {
        if (buttonDownTime[i][x] < BUTTON_HOLD) buttonDownTime[i][x]++;
        if ((buttonDownTime[i][x] >= BUTTON_HOLD && !bitRead(buttonEventWasHolding[i], x)) || (i == 0 && curRightScreen != kMuteMenu && x >= 4 && buttonDownTime[i][x] > BUTTON_CLICK && !bitRead(buttonEventWasHolding[i], x)))  //code to prevent rapid value cycling of buttons 4-7, probably needed for fast response of reading code
        {
          somethingClicked = false;
          bitSet(buttonEventWasHolding[i], x);
          if (i == 0 && curRightScreen != kMuteMenu && (x == 2 || x == 3))  //button 2 increase button 3 decrease, retrigger mechanic for rapid value cycling
          {
            buttonEvent[i][x] = kButtonClicked;
            bitClear(buttonEventWasHolding[i], x);
          } else {
            buttonEvent[i][x] = kButtonHold;
            if (i >= 1 && curRightScreen == kRightSteps && curTrack >= DRUM_TRACKS)  //probably logic to allow transposition note tracks when step is held
            {
              if (editingNoteTranspose == -127 && !forceAccent) {
                editingNoteTranspose = 0;
                noteTransposeWasChanged = false;
              }
            }
          }
          somethingHappened = true;
        }
      } else {
        if (buttonDownTime[i][x] > 0) {
          if ((buttonDownTime[i][x]) > BUTTON_CLICK) {
            if (ignoreNextButton) {
              ignoreNextButton = false;
            } else {
              if ((buttonDownTime[i][x]) >= BUTTON_HOLD || bitRead(buttonEventWasHolding[i], x)) {
                if (somethingClicked) buttonEvent[i][x] = kButtonRelease;
                else buttonEvent[i][x] = kButtonReleaseNothingClicked;
                somethingHappened = true;
              } else {
                if ((buttonDownTime[i][x]) < BUTTON_HOLD) {
                  buttonEvent[i][x] = kButtonClicked;
                  somethingHappened = true;
                }
                somethingClicked = true;
              }
            }
          }
        }
        buttonDownTime[i][x] = 0;
        bitClear(buttonEventWasHolding[i], x);
      }
    }
  }
}

void printNumber(byte segment, byte offset, int number) {


  bool isPositive = true;
  if (number < 0) {
    isPositive = false;
    number *= -1;
  }

  int x = number / 100;
  segments[segment][offset] = (char)pgm_read_word(&numbers[0 + x]);
  number -= x * 100;

  x = number / 10;
  segments[segment][offset + 1] = (char)pgm_read_word(&numbers[0 + x]);
  number -= x * 10;

  segments[segment][offset + 2] = (char)pgm_read_word(&numbers[0 + number]);

  if (!isPositive) segments[segment][offset] = B01000000;
}

void showErrorMsg(byte error) {
  showErrorMsg(error, false);
}
void showErrorMsg(byte error, bool errors) {
  memset(segments, 0, sizeof(segments));
  segments[2][0] = S_E;
  segments[2][1] = S_r;
  segments[2][2] = S_r;

  printNumber(2, 5, error);
  if (errors) segments[2][5] = S_S;
  sendScreen();
  if (totalFlashErrors < 0xFF) totalFlashErrors++;
  waitMs(2000);
}

void showWaitMsg(char porcentage) {
  memset(segments, 0, sizeof(segments));
  segments[2][0] = 0x3c;
  segments[2][1] = 0x1e;
  segments[2][2] = S_A;
  segments[2][3] = S_I;
  segments[2][4] = S_T;
  if (porcentage >= 0) printNumber(2, 5, porcentage);
  sendScreen();
}

void printDashDash(byte segment, byte offset) {
  segments[segment][offset] = S_DASH;
  segments[segment][offset + 1] = S_DASH;
}

void printMIDInote(byte note, byte segment, byte offset, byte offsetOctave) {
  // compute note % 12 and /12 without div/mod (saves flash)
  byte n = note, oct = 0;
  while (n >= 12) { n -= 12; oct++; }

  switch (n) {
    case 1:  segments[segment][offset + 1] = S_H;  /* fallthrough */  // C#
    case 0:  segments[segment][offset]     = S_C;  break;

    case 3:  segments[segment][offset + 1] = S_H;  /* fallthrough */  // D#
    case 2:  segments[segment][offset]     = S_d;  break;

    case 4:  segments[segment][offset]     = S_E;  break;

    case 6:  segments[segment][offset + 1] = S_H;  /* fallthrough */  // F#
    case 5:  segments[segment][offset]     = S_F;  break;

    case 8:  segments[segment][offset + 1] = S_H;  /* fallthrough */  // G#
    case 7:  segments[segment][offset]     = S_G;  break;

    case 10: segments[segment][offset + 1] = S_H;  /* fallthrough */  // A#
    case 9:  segments[segment][offset]     = S_A;  break;

    case 11: segments[segment][offset]     = S_b;  break;
  }

  if (oct) oct--;      // same octave shift as before
  if (oct > 9) oct = 9;
  segments[segment][offsetOctave] = (char)pgm_read_word(&numbers[oct]);
}

// void splashScreen() {
//   uint16_t t = 0;

//   while (t < 153) {  // ~2.5 sec at 20ms
//     for (byte r = 0; r < 3; r++) {
//       for (byte x = 0; x < 8; x++) {
//         segments[r][x] = random(1, 255);
//       }
//     }

//     leds[0] = leds[1] = leds[2] = random(0xFF);

//     sendScreenAndWait(3);
//     t++;
//   }

//   resetSegments(0, 2);

//   leds[0] = leds[1] = leds[2] = 0;


//   segments[0][0] = S_E;

//   segments[0][1] = S_O;

//   segments[0][2] = S_S;

//   segments[0][3] = S_E;

//   segments[0][4] = B10111011;

//   segments[0][5] = B00000000;

//   segments[0][6] = B11011011;

//   segments[0][7] = B00111111,
//   sendScreenAndWait(1848);
//}
//END OF LINE