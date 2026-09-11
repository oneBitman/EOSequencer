/*

   Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com

*/
void createScreen() {

  leds[0] = leds[1] = leds[2] = 0;

  if (!seqPlaying || seqPlaying)  //ABCD leds always
  {
    byte xVar = variation;
    if (forceVariation >= 0) xVar = forceVariation;
    if (mirror) leds[0] |= B11110000;
    else {
      if (forceVariation >= 0) {
        leds[0] |= B11110000;
        bitClear(leds[0], xVar + 4);
      } else bitSet(leds[0], xVar + 4);
    }
  }

  if (curRightScreen == kRightPatternSelection) {
    // Clear all LEDs first and display current bank/pattern on 16 step leds
    leds[1] = leds[2] = 0;

    if (isSelectingBank) {
      if (nextPatternBank < 8) {
        bitSet(leds[1], nextPatternBank);  // Set LED on the first byte (1-8 patterns)
      } else {
        bitSet(leds[2], nextPatternBank - 8);  // Set LED on the second byte (9-16 patterns)
      }
    } else {
      //
      if (nextPattern < 8) {
        bitSet(leds[1], nextPattern);  // Set LED on the first byte (1-8 patterns)
      } else {
        bitSet(leds[2], nextPattern - 8);  // Set LED on the second byte (9-16 patterns)
      }
    }
  }

  //where to show selection led
  if (curRightScreen == kRightTrackSelection || curRightScreen == kRightSteps || (curRightScreen == kRightMenu && (menuPosition == menuMIDIChannel || menuPosition == menuNote || menuPosition == menuNoteLen || menuPosition == menuProgramChange || menuPosition == menuTrackLen || menuPosition == menuTrackProbability))) {
    if (curTrack < 8) bitSet(leds[1], curTrack);
    else bitSet(leds[2], curTrack - 8);
  }
  if (seqPlaying && (curRightScreen != kRightTrackSelection && curRightScreen != kRightMenu)) {
    leds[1] = leds[1] ^ chaseLEDs[0];
    leds[2] = leds[2] ^ chaseLEDs[1];
  }

  // Main control interface (LeftScreen, as previously all three boards were side by side)

  if (curLeftScreen == kLeftMain && forceAccent == false && (curRightScreen != kRightMenu && curRightScreen != kMuteMenu)) {
    resetSegments(0, 2);
    if (editingNoteTranspose != -127 && !editingNote && curRightScreen == kRightSteps && curTrack >= DRUM_TRACKS) {

      if (noteTransposeEditAllSteps) {
        recordEnabled = false;  //auto turn off record mode to show A for all transpose
        segments[0][0] = S_A;   //letter for transpose all
        printNumber(0, 1, editingNoteTranspose);
      } else {
        recordEnabled = false;       //auto turn off record mode to show N for note transpose
        segments[0][0] = B01010100;  //small n for note transpose
        byte holdingNote = 0;
        for (byte x = 0; x < 8; x++) {
          for (byte i = 0; i < 2; i++) {
            if (bitRead(buttonEventWasHolding[i + 1], x)) {
              if (holdingNote == 0) holdingNote = stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][editVariation];
              else {
                holdingNote = 0;
                break;
              }
            }
          }
        }
        //
        if (holdingNote == 0) printNumber(0, 1, editingNoteTranspose);
        else {
          segments[0][2] = 0;
          printNumber(0, 1, holdingNote);
        }
      }
      byte pattern[] = { B01010100, B01010100, S_A, S_A };
      segments[0][7] = pattern[quantizedValue % 4];

    }

    else

    {
      if (midiClockInternal) {
        printNumber(0, 1, realBPM);
      } else {
        segments[0][0] = B00000000;
        segments[0][1] = S_E;
        segments[0][2] = S_X;
        segments[0][3] = S_T;
      }

      if (seqPosition % 4 == 0) {
        if (repeatMode == kRepeatModeNormal) {
          segments[0][0] = B01011100;
        } else if (repeatMode == kRepeatModeChain) {
          segments[0][0] = S_C;
        } else if (repeatMode == kRepeatModePattern) {
          segments[0][0] = S_P;
        }
      } else {
        segments[0][0] = B00000000;
      }

      // Blinking R on record, BPM synced
      if (recordEnabled) {
        if (seqPosition % 4 != 0) {
          segments[0][0] = S_R;
        }
      } else {
        if (seqPosition % 4 != 0) {
          segments[0][0] = B00000000;
        }
      }

      if (nextPattern != currentPattern) printNumber(0, 5, nextPattern + 1);

      else printNumber(0, 5, currentPattern + 1);

      if (streamNextPattern || loadPatternNow) {

        repeatMode = kRepeatModeNormal;  //cancel any repeat mode
        // Rotating 8 animation on pattern load
        const byte segmentMapping[8] = {
          B00001000,  // First position
          B00010000,  // Second position
          B01000000,  // Third position
          B00000010,  // Fourth position
          B00000001,  // Fifth position
          B00100000,  // Sixth position
          B01000000,  // Seventh position
          B00000100   // Eighth position
        };

        // Assign the segment value based on `seqPosition`
        segments[0][4] = segmentMapping[seqPosition % 8];

      } else {

        segments[0][4] = visualPattern;  // Else display midi bandwidth/load
      }

      byte theBank = currentPatternBank + 1;
      if (currentPatternBank != nextPatternBank) theBank = nextPatternBank + 1;
      segments[0][5] = getBankLetter(theBank);
      segments[0][5] |= B10000000;
    }
  }

  // screen manipulations based on various parameters

  if (curRightScreen == kMuteMenu) {  // mute screen
    segments[0][0] = B00000000;
    segments[0][1] = B00000000;
    segments[0][2] = S_N;
    segments[0][3] = S_U;
    segments[0][4] = S_T;
    segments[0][5] = S_E;
    segments[0][6] = B00000000;

    // RETURN animation
    segments[0][7] = exitKeyMapping[quantizedValue % 8];
  }

  if (curRightScreen == kRightMenu) {
    segments[0][2] = B00011100;  //DOWN
    segments[0][3] = B00100011;  //UP
    segments[0][4] = B01011000;  //LEFT
    segments[0][5] = B01001100;  //RIGHT
    segments[0][6] = B00000000;

    segments[0][7] = exitKeyMapping[quantizedValue % 8];
    recordEnabled = false;  //We do not want to record in setup
  }

  if (forceAccent == true && curRightScreen != kRightMenu && curLeftScreen == kLeftMain && (segments[0][0] != S_A && segments[0][0] != B01010100)) {  //segments: prevent splash from showing in transpose mode
    segments[0][2] = B00001100;
    segments[0][3] = B00011000;
    segments[0][4] = S_R;
    segments[0][5] = S_E;
    segments[0][6] = B00000000;

    // Define mapping array for ABCD animation
    const byte ABCDMapping[4] = {
      S_A,  // First position
      S_b,  // Second position
      S_C,  // Third position
      S_d   // Fourth position
    };

    segments[0][7] = ABCDMapping[quantizedValue % 4];

    if (repeatMode == kRepeatModeNormal) {
      segments[0][0] = B01011100;
      segments[0][1] = B00000000;
    }
    if (repeatMode == kRepeatModeChain) {
      segments[0][0] = S_C;
      segments[0][1] = B00000000;
    }
    if (repeatMode == kRepeatModePattern) {
      segments[0][0] = S_P;
      segments[0][1] = B00000000;
      ;
    }
  }

  else if (curLeftScreen == kRandom) {
    segments[0][1] = S_R;
    segments[0][2] = S_A;
    segments[0][3] = S_N;
    segments[0][4] = S_d;
    segments[0][5] = S_O;
    segments[0][6] = S_N;
    segments[0][0] = B00000000;
    segments[0][7] = B00000000;
  }

  else if (curLeftScreen == kErase) {
    segments[0][0] = S_E;
    segments[0][1] = S_R;
    segments[0][2] = S_A;
    segments[0][3] = S_S;
    segments[0][4] = S_E;
    segments[0][5] = S_T;
    segments[0][6] = S_R;
    segments[0][7] = S_K;
  }

  if (forceAccent == true && curRightScreen != kRightMenu && segments[0][0] == B01010100 && segments[0][2] != S_T) {
    segments[0][0] = S_A;
    segments[0][1] = S_L;
    segments[0][2] = S_L;
    segments[0][3] = B00000000;
    segments[0][4] = S_T;
    segments[0][5] = S_R;
    segments[0][6] = S_S;
    segments[0][7] = S_P;
  } else if (forceAccent == true && curRightScreen != kRightMenu && segments[0][0] == S_A && segments[0][1] != S_L) {
    segments[0][0] = B01010100;
    segments[0][1] = S_O;
    segments[0][2] = S_T;
    segments[0][3] = S_E;
    segments[0][4] = S_E;
    segments[0][5] = S_d;
    segments[0][6] = S_I;
    segments[0][7] = S_T;
  }

  resetSegments(1, 2);

  if (temporaryMessageCounter > 0) {
    if (showTemporaryMessage == kMemoryProtectMessage) showMemoryProtected();
    else if (showTemporaryMessage == kPatternRepeatMessage) {
      if (repeatMode == kRepeatModeNormal) {
        segments[1][1] = S_N;  // normal playback or normal chain playback
        segments[1][2] = S_O;
        segments[1][3] = S_R;
        segments[1][4] = S_N;
        segments[1][5] = S_A;
        segments[1][6] = S_L;
        //
        segments[2][1] = S_P;
        segments[2][2] = S_L;
        segments[2][3] = S_A;
        segments[2][4] = S_Y;
      } else if (repeatMode == kRepeatModeChain) {

        segments[1][3] = S_K;
        segments[1][4] = S_E;
        segments[1][5] = S_E;
        segments[1][6] = S_P;

        segments[2][1] = S_C;  // repeat whole chain regardless of chain repeat parameter
        segments[2][2] = S_H;
        segments[2][3] = S_A;
        segments[2][4] = S_I;
        segments[2][5] = S_N;
        //

      } else {
        segments[1][1] = S_O;  // 1 pattern repeat
        segments[1][2] = S_N;
        segments[1][3] = S_E;

        segments[1][5] = S_P;
        segments[1][6] = S_A;
        segments[1][7] = S_T;

        segments[2][1] = S_R;
        segments[2][2] = S_E;
        segments[2][3] = S_P;
        segments[2][4] = S_E;
        segments[2][5] = S_A;
        segments[2][6] = S_T;
      }
    }

    else if (showTemporaryMessage == kSysexDump) {
      segments[1][2] = S_S;
      segments[1][3] = S_Y;
      segments[1][4] = S_S;
      segments[1][5] = S_E;
      segments[1][6] = S_X;

      segments[2][1] = S_d;
      segments[2][2] = S_U;
      segments[2][3] = S_N;
      segments[2][4] = S_P;

    }

    else if (showTemporaryMessage == kStopSequencer) {
      segments[1][4] = S_S;
      segments[1][5] = S_T;
      segments[1][6] = S_O;
      segments[1][7] = S_P;

      segments[2][0] = S_P;
      segments[2][1] = S_L;
      segments[2][2] = S_A;
      segments[2][3] = S_Y;
      segments[2][4] = S_b;
      segments[2][5] = S_A;
      segments[2][6] = S_C;
      segments[2][7] = S_K;
    }

    temporaryMessageCounter++;
    if (temporaryMessageCounter >= TEMPORARY_MESSAGE_TIME) temporaryMessageCounter = 0;
    return;
  }


  // MUTE Screen
  if (curRightScreen == kMuteMenu) {
    createScreenMute();
    return;
  }

  // Right Screen (Sewuencer steps)
  if (curRightScreen == kRightMenuCopyPaste) {
    showMenuCopyPaste();
  } else if (curRightScreen == kRightMenu)  //setup
  {
    showMenu();
  }

  if (curRightScreen == kRightPatternSelection && isSelectingBank) {
    segments[1][2] = S_b;
    segments[1][3] = S_A;
    segments[1][4] = S_N;
    segments[1][5] = S_K;

    segments[2][1] = S_S;
    segments[2][2] = S_E;
    segments[2][3] = S_L;
    segments[2][4] = S_E;
    segments[2][5] = S_C;
    segments[2][6] = S_T;
  }

  else if (curRightScreen == kRightPatternSelection) {
    segments[1][1] = S_P;
    segments[1][2] = S_A;
    segments[1][3] = S_T;
    segments[1][4] = S_T;
    segments[1][5] = S_E;
    segments[1][6] = S_R;
    segments[1][7] = S_N;

    segments[2][1] = S_S;
    segments[2][2] = S_E;
    segments[2][3] = S_L;
    segments[2][4] = S_E;
    segments[2][5] = S_C;
    segments[2][6] = S_T;
  }


  else if (curRightScreen == kRightTrackSelection) {
    segments[1][2] = S_T;
    segments[1][3] = S_R;
    segments[1][4] = S_A;
    segments[1][5] = S_C;
    segments[1][6] = S_K;

    segments[2][1] = S_S;
    segments[2][2] = S_E;
    segments[2][3] = S_L;
    segments[2][4] = S_E;
    segments[2][5] = S_C;
    segments[2][6] = S_T;
  }

  else if (curRightScreen == kRightSteps) {

    byte xVar = variation;
    if (forceVariation >= 0) xVar = forceVariation;

    if (curTrack < DRUM_TRACKS) {
      for (byte xs = 0; xs < 2; xs++) {
        for (byte x = 0; x < 8; x++) {
          getStepVelocity(x + (xs * 8), curTrack, xVar, false);
          //
          segments[xs + 1][x] = (char)pgm_read_word(&stepChars[stepVelocity]);
          //
          if (bitRead(stepsData[x + (xs * 8)].stepsDouble[xVar], curTrack)) segments[xs + 1][x] |= B00001000;  //double step char icon
        }
      }
    } else  // NOTE TRACKS

      for (byte xs = 0; xs < 2; xs++) {
        for (byte x = 0; x < 8; x++) {
          xm = bitRead(stepsData[x + (xs * 8)].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (xVar * 2)) << 1;
          xm |= bitRead(stepsData[x + (xs * 8)].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (xVar * 2));
          getStepVelocity(x + (xs * 8), curTrack, xVar, false);
          //
          if (xm != 3 && stepsData[x + (xs * 8)].noteSteps[curTrack - DRUM_TRACKS][xVar] > 0 && stepVelocity != 0)
            segments[xs + 1][x] = (char)pgm_read_word(&stepChars[stepVelocity]);
          //
          if (xm == 1) segments[xs + 1][x] |= B00011000;  //set segment icon for slide/legato
          //else if (xm == 2) segments[xs + 1][x] |= B10000000;  //double step note track - not used
          else if (xm == 3) segments[xs + 1][x] |= B01001001;  //note off segment icon
        }
      }
  }
  if ((curRightScreen == kRightMenu && menuPosition == menuNote) || curRightScreen == kRightSteps || curRightScreen == kRightTrackSelection) {
    for (byte t = 0; t < 16; t++) {  // visual feedback for track activity in multitrack recording, segment dot
      if (allMidiActivityFlags & (1 << t)) {
        if (t < 8) {
          segments[1][t] |= B10000000;
        } else {
          segments[2][t - 8] |= B10000000;
        }
      }
    }
  }

  if (isLastPatternRepeat && seqPosition % 4 == !0) segments[0][6] = segments[0][7] = 0;  //blank the display to indicate last pattern repeat
  if (isLastChainRepeat && seqPosition % 4 == !0) {                                       //blank the display to indicate last chain repeat
    segments[0][1] = 0;
    segments[0][2] = 0;
    segments[0][3] = 0;
    segments[0][4] = 0;
    segments[0][5] = 0;
    segments[0][6] = 0;
    segments[0][7] = 0;
  }
}

void checkInterface()  //Tactile buttons
{
  if (curRightScreen == kMuteMenu)  // MUTE Editing
  {
    checkInterfaceMute();
    return;
  }

  if (!somethingHappened) return;
  somethingHappened = false;

  // Start Sequencer
  if (buttonEvent[0][0] >= kButtonClicked && (curRightScreen != kRightMenu)) {
    if (buttonEvent[0][0] >= kButtonClicked || buttonEvent[0][0] >= kButtonHold) {

      if (seqPlaying) {
        if (forceAccent) {
          repeatMode++;
          if (repeatMode > kRepeatModePattern) repeatMode = kRepeatModeNormal;
          ShowTemporaryMessage(kPatternRepeatMessage);
          preventABCD = 1;
        } else {
          recordEnabled = !recordEnabled;
        }
      } else if (!seqPlaying && midiClockInternal) {
        startSequencer();
      }
    }
    buttonEvent[0][0] = 0;  // Reset the button state
  }
  if (curRightScreen == kRightMenu) buttonEvent[0][0] = 0;


  //SYX dump current pattern
  if (buttonEvent[0][1] >= kButtonClicked && forceAccent == true && curRightScreen != kRightMenu) {
    sendSysExPatternData();
    buttonEvent[0][1] = 0;
    preventABCD = 1;  // Prevent ABCD change
  }

  // Stop Sequencer
  if ((buttonEvent[0][1] >= kButtonClicked && forceAccent == false) && curRightScreen != kRightMenu) {

    if (midiClockInternal) {
      stopSequencer();

    } else {
      midiClockInternal = true;
      stopSequencer();
      if (globalData.seqSyncOut) Serial.write(0xFC);
    }

    buttonEvent[0][1] = 0;
    editingNoteTranspose = -127;
  }
  if (curRightScreen == kRightMenu) buttonEvent[0][1] = 0;
  //
  // (-) BPM or Menu decrease
  if (buttonEvent[0][2] >= kButtonClicked) {
    if (buttonEvent[0][2] >= kButtonClicked && forceAccent) {
      rotateSteps(true);  // Rotate to the left
      preventABCD = 1;    // Prevent ABCD change
    }
    if (buttonEvent[0][2] == kButtonClicked && forceAccent == false) {
      if (editingNoteTranspose != -127 && curRightScreen == kRightSteps && curTrack >= DRUM_TRACKS) {
        bool foundOne = false;
        for (byte x = 0; x < 8; x++) {
          for (byte i = 0; i < 2; i++) {
            if ((noteTransposeEditAllSteps || bitRead(buttonEventWasHolding[i + 1], x))) {
              for (byte xp = 0; xp < 4; xp++) {
                if (!mirror) xp = editVariation;
                if (stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp] > 0) {
                  stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp]--;

                  patternData.lastNote[curTrack - DRUM_TRACKS] = stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp];

                  if (patternData.lastNote[curTrack - DRUM_TRACKS] <= 1) {
                    patternData.lastNote[curTrack - DRUM_TRACKS] = DEFAULT_NOTE;  // 48
                  }

                  //
                  if (!foundOne && editingNoteTranspose > -125) editingNoteTranspose--;
                  foundOne = true;
                }
                //
                if (!mirror) break;
              }
              //
              noteTransposeWasChanged = true;
            }
          }
        }
      } else if (curRightScreen == kRightMenu) {
        processMenu(-1);
      } else if (curRightScreen == kRightPatternSelection && (!isSelectingBank)) {
        nextPattern = (nextPattern + 15) % 16;  // decrement and wrap
        loadPattern(nextPattern);
      } else if (curRightScreen == kRightTrackSelection) {
        curTrack = (curTrack + 15) % 16;  // decrement and wrap
      } else {
        if (realBPM > 40) {
          realBPM--;
        }
        updateSequencerSpeed(false);
      }
    }

    buttonEvent[0][2] = 0;
  }
  // (+) BPM or Menu increase

  if (buttonEvent[0][3] >= kButtonClicked) {
    if (buttonEvent[0][3] >= kButtonClicked && forceAccent) {
      rotateSteps(false);  // Rotate to the left
      preventABCD = 1;     // Prevent ABCD change
    }


    if (buttonEvent[0][3] == kButtonClicked && forceAccent == false) {

      if (editingNoteTranspose != -127 && curRightScreen == kRightSteps && curTrack >= DRUM_TRACKS) {
        bool foundOne = false;
        for (byte x = 0; x < 8; x++) {
          for (byte i = 0; i < 2; i++) {
            if ((noteTransposeEditAllSteps || bitRead(buttonEventWasHolding[i + 1], x))) {
              for (byte xp = 0; xp < 4; xp++) {
                if (!mirror) xp = editVariation;
                if (stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp] < 127) {
                  stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp]++;
                  patternData.lastNote[curTrack - DRUM_TRACKS] = stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][xp];
                  //
                  if (!foundOne && editingNoteTranspose < 127) editingNoteTranspose++;
                  foundOne = true;
                }
                //
                if (!mirror) break;
              }
              //
              noteTransposeWasChanged = true;
            }
          }
        }
      } else if (curRightScreen == kRightMenu) {
        processMenu(1);
      } else if (curRightScreen == kRightPatternSelection && (!isSelectingBank)) {
        nextPattern = (nextPattern + 1) % 16;  // increment and wrap
        loadPattern(nextPattern);
      } else if (curRightScreen == kRightTrackSelection) {
        curTrack = (curTrack + 1) % 16;  // increment and wrap
      } else {
        if (realBPM < 999) {
          realBPM++;
        }
        updateSequencerSpeed(false);
      }
    }


    buttonEvent[0][3] = 0;
  }

  if (curRightScreen == kRightSteps)  // Regular DRUM Steps Editing
  {
    byte xVar = variation;
    if (forceVariation >= 0) xVar = forceVariation;
    if (mirror) xVar = 0;

    if (curTrack < DRUM_TRACKS) {
      for (byte x = 0; x < 8; x++) {
        for (byte i = 0; i < 2; i++) {
          if (buttonEvent[i + 1][x] > kButtonNone) {
            getStepVelocity(x + (i * 8), curTrack, xVar, (buttonEvent[i + 1][x] == kButtonClicked));
            //
            if (buttonEvent[i + 1][x] == kButtonClicked) {
              somethingChangedPattern = true;
              //
              if (forceAccent) {
                if (stepVelocity == 0) {
                  stepVelocity = 1;
                  preventABCD = 1;
                } else if (stepVelocity == 1) {
                  stepVelocity = 2;
                  preventABCD = 1;
                } else if (stepVelocity == 2) {
                  stepVelocity = 3;
                  preventABCD = 1;
                } else if (stepVelocity == 3) {
                  stepVelocity = 1;
                  preventABCD = 1;
                }
                drumStepLastVelocity = stepVelocity;
              } else {
                if (stepVelocity == 0) stepVelocity = drumStepLastVelocity;
                else stepVelocity = 0;  //which velocity for drumstep without force accent button
              }
              stepsData[x + (i * 8)].steps[curTrack] |= stepVelocity << (xVar * 2);
              //
              if (mirror) {
                stepsData[x + (i * 8)].steps[curTrack] = 0x00;
                stepsData[x + (i * 8)].steps[curTrack] |= stepVelocity;
                stepsData[x + (i * 8)].steps[curTrack] |= stepVelocity << 2;
                stepsData[x + (i * 8)].steps[curTrack] |= stepVelocity << 4;
                stepsData[x + (i * 8)].steps[curTrack] |= stepVelocity << 6;
              }
            } else if (buttonEvent[i + 1][x] == kButtonHold) {

              somethingChangedPattern = true;

              if (bitRead(stepsData[x + (i * 8)].stepsDouble[xVar], curTrack)) {
                bitClear(stepsData[x + (i * 8)].stepsDouble[xVar], curTrack);
                if (mirror) {
                  bitClear(stepsData[x + (i * 8)].stepsDouble[1], curTrack);
                  bitClear(stepsData[x + (i * 8)].stepsDouble[2], curTrack);
                  bitClear(stepsData[x + (i * 8)].stepsDouble[3], curTrack);
                }
              } else {
                bitSet(stepsData[x + (i * 8)].stepsDouble[xVar], curTrack);
                if (mirror) {
                  bitSet(stepsData[x + (i * 8)].stepsDouble[1], curTrack);
                  bitSet(stepsData[x + (i * 8)].stepsDouble[2], curTrack);
                  bitSet(stepsData[x + (i * 8)].stepsDouble[3], curTrack);
                }
              }
            }
            buttonEvent[i + 1][x] = 0;
          }
        }
      }
    } else  // NOTE TRACKS
    {
      for (byte x = 0; x < 8; x++) {
        for (byte i = 0; i < 2; i++) {
          if (buttonEvent[i + 1][x] > kButtonNone) {
            somethingChangedPattern = true;

            editStep = x + (i * 8);
            editVariation = xVar;
            getStepVelocity(editStep, curTrack, editVariation, false);
            if ((buttonEvent[i + 1][x] == kButtonClicked || buttonEvent[i + 1][x] == kButtonHold) && (editingNoteTranspose == -127)) {
              // Allow longer hold on note steps
              if (stepsData[editStep].noteSteps[curTrack - DRUM_TRACKS][editVariation] == 0 && patternData.lastNote[curTrack - DRUM_TRACKS] == 0) {
                clearStepsExtrasBits(editStep, editVariation, curTrack - DRUM_TRACKS);
                editingNote = true;
                patternData.lastNote[curTrack - DRUM_TRACKS] = 60;
                stepsData[editStep].noteSteps[curTrack - DRUM_TRACKS][editVariation] = patternData.lastNote[curTrack - DRUM_TRACKS];
                stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0] |= lastVelocity << (editVariation * 2);
              } else if (stepsData[editStep].noteSteps[curTrack - DRUM_TRACKS][editVariation] == 0 && patternData.lastNote[curTrack - DRUM_TRACKS] > 0) {
                clearStepsExtrasBits(editStep, editVariation, curTrack - DRUM_TRACKS);
                stepsData[editStep].noteSteps[curTrack - DRUM_TRACKS][editVariation] = patternData.lastNote[curTrack - DRUM_TRACKS];
                stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0] |= lastVelocity << (editVariation * 2);
              } else if (forceAccent) {
                if (buttonEvent[i + 1][x] == kButtonHold) {
                  // Handle slide toggle on long hold with forceAccent
                  if (stepVelocity == 0) stepVelocity = lastVelocity;
                  byte slideState = (bitRead(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (editVariation * 2)) | (bitRead(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (editVariation * 2)) << 1));

                  if (slideState == 1) {  // If currently slide, switch to note off

                    bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 1 + (editVariation * 2));
                    bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 0 + (editVariation * 2));
                    bitSet(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (editVariation * 2));
                    bitSet(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (editVariation * 2));

                    preventABCD = 1;
                  } else {  // Otherwise, set it as slide
                    bitSet(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (editVariation * 2));
                    bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (editVariation * 2));  // Ensure it's only slide
                    preventABCD = 1;
                  }
                } else {  // Cycle velocities only for short or mid hold with forceAccent
                  getStepVelocity(editStep, curTrack, editVariation, true);
                  getNoteStepGlideDoubleOff(editStep, curTrack - DRUM_TRACKS, editVariation, true);

                  if (stepVelocity == 0) {
                    stepVelocity = 1;
                    preventABCD = 1;
                  } else if (stepVelocity == 1) {
                    stepVelocity = 2;
                    preventABCD = 1;
                  } else if (stepVelocity == 2) {
                    stepVelocity = 3;
                    preventABCD = 1;
                  } else if (stepVelocity == 3) {
                    stepVelocity = 1;
                    preventABCD = 1;
                  }

                  lastVelocity = stepVelocity;
                  stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0] |= stepVelocity << (editVariation * 2);
                  stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1] |= noteStepGlideDoubleOff << (editVariation * 2);
                }

                checkIfMirrorAndCopy(editStep, curTrack - DRUM_TRACKS);
              } else if (stepsData[editStep].noteSteps[curTrack - DRUM_TRACKS][editVariation] > 0) {
                getNoteStepGlideDoubleOff(editStep, curTrack - DRUM_TRACKS, editVariation, false);
                if (noteStepGlideDoubleOff == 3) {
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (editVariation * 2));
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (editVariation * 2));
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 1 + (editVariation * 2));
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 0 + (editVariation * 2));
                }

                if (stepVelocity == 0) {
                  stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0] |= (lastVelocity & 0x03) << (editVariation * 2);

                } else {  // clear step
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 1 + (editVariation * 2));
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][0], 0 + (editVariation * 2));
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 0 + (editVariation * 2));  // clear slide/note off
                  bitClear(stepsData[editStep].noteStepsExtras[curTrack - DRUM_TRACKS][1], 1 + (editVariation * 2));  // clear slide/note off
                }
              }
            }

            checkIfMirrorAndCopy(editStep, curTrack - DRUM_TRACKS);
          }
          // Reset transpose on button release
          if (buttonEvent[i + 1][x] == kButtonRelease) {
            editingNoteTranspose = -127;
          }
          // Reset button events
          buttonEvent[i + 1][x] = 0;
        }
      }
    }
  }


  // 16 step button logic when NOT step editing (selecting track, pattern, copypaste, menu)
  if (curRightScreen == kRightTrackSelection || curRightScreen == kRightPatternSelection || curRightScreen == kRightMenuCopyPaste || curRightScreen == kRightMenu) {
    char leButton = -1;
    leButtonHeld = false;
    for (byte x = 0; x < 8; x++) {
      for (byte i = 0; i < 2; i++) {
        if (buttonEvent[i + 1][x] >= kButtonClicked || buttonEvent[i + 1][x] == kButtonHold || buttonEvent[i + 1][x] >= kButtonHold) {

          leButton = x + (i * 8);
        }
        if (buttonDownTime[i + 1][x] > 0) {
          leButtonHeld = true;
        }

        buttonEvent[i + 1][x] = 0;
      }
    }

    if (leButton >= 0) {
      if (curRightScreen == kRightMenu && menuPosition == menuInit) {
        processMenuFORMAT();
      } else if (curRightScreen == kRightTrackSelection || curRightScreen == kRightMenu) {

        if (leButton != curTrack) {
          trackChanged = true;
          midiLearnActive = false;
        }
        editingNote = false;
        curTrack = leButton;
        midiLearnActive = false;

        if (curRightScreen == kRightMenu && menuPosition == menuNote && (!trackChanged) && (leButtonHeld))  // midi drum note learn function
        {
          midiLearnActive = true;
        }
        trackChanged = false;

      }


      else if (curRightScreen == kRightMenuCopyPaste) {
        processMenuCopyPaste(leButton);
      } else if (curRightScreen == kRightPatternSelection) {


        if (isSelectingBank) {
          // Bank selection logic
          bitSet(patternBitsSelector, leButton);  // Set the bit for bank selection
          nextPatternBank = leButton;
          nextPattern = 0;  // Reset pattern to 0 when switching banks
        } else {

          // Pattern selection logic
          bitSet(patternBitsSelector, leButton);  // Set the bit for pattern selection
          nextPattern = leButton;
        }

        // Load pattern only if there's a change
        if (currentPattern != nextPattern || nextPatternBank != currentPatternBank) {
          loadPattern(nextPattern);  // Load the selected pattern from the correct bank
        }
      }
    } else {
      //nothing
    }
  }

  // Button combos
  // Combo 1: Bank + Pattern = Setup Menu
  if ((isSelectingBank && buttonEvent[0][5] == kButtonHold) || (!isSelectingBank && curRightScreen == kRightPatternSelection && buttonEvent[0][4] == kButtonHold) || (buttonEvent[0][4] == kButtonHold && buttonEvent[0][5] == kButtonHold)) {

    curRightScreen = kRightMenu;                // Activate Setup Menu
    initMode = 0;                               // deactivated: menuPosition = 0; - when entering menu last position is kept
    buttonEvent[0][4] = buttonEvent[0][5] = 0;  // Reset button events
    isSelectingBank = false;                    // Exit bank selection mode after combo

  }

  // Combo 2: Pattern + Track = Copy Menu
  else if (((!isSelectingBank && curRightScreen == kRightPatternSelection && buttonEvent[0][6] == kButtonHold) || (curRightScreen == kRightTrackSelection && buttonEvent[0][5] == kButtonHold) || (buttonEvent[0][5] == kButtonHold && buttonEvent[0][6] == kButtonHold)) && curRightScreen != kRightMenu)

  {
    curRightScreen = kRightMenuCopyPaste;       // Copy Menu
    buttonEvent[0][5] = buttonEvent[0][6] = 0;  // Reset button events
  }

  // Combo 3: Track + Shift = Mute Menu
  else if ((curRightScreen == kRightTrackSelection && buttonEvent[0][7] == kButtonHold) || (forceAccent && buttonEvent[0][6] == kButtonHold) || (buttonEvent[0][6] == kButtonHold && buttonEvent[0][7] == kButtonHold)) {

    curRightScreen = kMuteMenu;  // Mute Screen
    forceAccent = false;
    isSelectingBank = false;                    // Reset bank selection flag if active
    preventABCD = 0;                            // use variable to debounce
    buttonEvent[0][6] = buttonEvent[0][7] = 0;  // Reset button events
  }

  // Logic for reading single buttons 4 (Bank), 5 (Pattern), 6 (Track), 7 (Shift/ABCD/Velocity/Transpose)

  // Button 4: Bank Selection
  if (curRightScreen != kMuteMenu) {  // Ensure action only if not in mute screen
    if (curRightScreen == kRightMenu) {
      // In setup menu, Button 4 decreases setup option
      if (buttonEvent[0][4] == kButtonHold) {
        if (menuPosition > menuFirst) menuPosition--;
        else menuPosition = lastMenu;
        initMode = 0;
        buttonEvent[0][4] = 0;
      }
    } else {
      // Normal bank selection logic
      if (buttonEvent[0][4] == kButtonHold && forceAccent == true) {
        randomTrack();
        curLeftScreen = kRandom;
      }

      else if (buttonEvent[0][4] == kButtonHold) {
        isSelectingBank = true;
        curRightScreen = kRightPatternSelection;
        buttonEvent[0][4] = 0;
      } else if (buttonEvent[0][4] > kButtonHold || buttonEvent[0][4] == kButtonClicked) {
        isSelectingBank = false;
        curRightScreen = kRightSteps;
        curLeftScreen = kLeftMain;
        buttonEvent[0][4] = 0;
        preventABCD = 1;
      }
    }
  }

  // Button 5: Pattern Selection
  if (curRightScreen != kMuteMenu) {  // Ensure action only if not in mute screen
    if (curRightScreen == kRightMenu) {
      // In setup menu, Button 5 increases setup option
      if (buttonEvent[0][5] == kButtonHold) {
        menuPosition++;
        initMode = 0;
        if (menuPosition > lastMenu) menuPosition = menuFirst;
        buttonEvent[0][5] = 0;
      }
    } else {
      // Normal pattern selection logic
      if (buttonEvent[0][5] == kButtonHold && forceAccent == true) {
        eraseTrack();
        curLeftScreen = kErase;
      }

      else if (buttonEvent[0][5] == kButtonHold) {
        isSelectingBank = false;
        curRightScreen = kRightPatternSelection;
        buttonEvent[0][5] = 0;
      } else if (buttonEvent[0][5] > kButtonHold || buttonEvent[0][5] == kButtonClicked) {
        curRightScreen = kRightSteps;
        curLeftScreen = kLeftMain;
        buttonEvent[0][5] = 0;
        preventABCD = 1;
      }
    }
  }

  // Button 6: Track Selection
  if (curRightScreen != kMuteMenu && curRightScreen != kRightMenu) {  // Ensure action only if not in mute or setup menu
    if (buttonEvent[0][6] == kButtonHold) {
      curRightScreen = kRightTrackSelection;
      buttonEvent[0][6] = 0;
    } else if (buttonEvent[0][6] > kButtonHold || buttonEvent[0][6] == kButtonClicked) {
      curRightScreen = kRightSteps;
      curLeftScreen = kLeftMain;
      buttonEvent[0][6] = 0;
    }
  }

  // Button 7: Shift/ABCD/Exit - ACTION BUTTON

  if (buttonEvent[0][7] == kButtonHold)  // Button 7 logic (Shift/ABCD/Exit)

  {
    buttonEvent[0][7] = 0;

    if (preventABCD == 1 && curRightScreen == kRightSteps) preventABCD = 0;

    forceAccent = true;
    isSelectingBank = false;
  } else if (buttonEvent[0][7] > kButtonHold || buttonEvent[0][7] == kButtonClicked)  // Button release
  {
    forceAccent = false;
    isSelectingBank = false;
    buttonEvent[0][7] = 0;
    curLeftScreen = kLeftMain;

    if (curRightScreen == kRightMenu) {
      curRightScreen = kRightSteps;
      preventABCD = 1;
    }
    if (editingNoteTranspose != -127) {
      noteTransposeEditAllSteps = !noteTransposeEditAllSteps;
    }

    else if (preventABCD == 0)  // ABCD variation flag check
    {
      if (buttonEvent[0][6] != kButtonHold)  // Skip ABCD logic if both buttons were pressed simultaneously (i.e., mute screen was triggered)
      {
        if (patternData.totalVariations == 1) {
          mirror = false;  // Only one variation (A), so disable mirror mode
          forceVariation = 0;
        }
        // Cycle through edit modes: !mirror, mirror, only A, only B, only C, only D
        else if (forceVariation == -1) {
          if (mirror) {
            mirror = false;  // Exit mirror mode and start cycling ABCD
            forceVariation = 0;
          } else {
            mirror = true;
          }
        } else if (patternData.totalVariations > 1) {
          // Cycle through forceVariation
          forceVariation++;  // A=1, B=2, C=3, D=4
          if (forceVariation == patternData.totalVariations) {
            forceVariation = -1;  //ABCD cycling finished, toggle back to mirror mode
          }
        }
      }
    }
  }
}

void checkIfMirrorAndCopy(byte thestep, byte track) {
  if (mirror) {
    stepsData[thestep].noteSteps[track][1] = stepsData[thestep].noteSteps[track][0];
    stepsData[thestep].noteSteps[track][2] = stepsData[thestep].noteSteps[track][0];
    stepsData[thestep].noteSteps[track][3] = stepsData[thestep].noteSteps[track][0];

    stepsData[thestep].noteStepsExtras[track][0] &= B00000011;
    stepsData[thestep].noteStepsExtras[track][0] |= stepsData[thestep].noteStepsExtras[track][0] << 2;
    stepsData[thestep].noteStepsExtras[track][0] |= stepsData[thestep].noteStepsExtras[track][0] << 4;
    stepsData[thestep].noteStepsExtras[track][0] |= stepsData[thestep].noteStepsExtras[track][0] << 6;

    stepsData[thestep].noteStepsExtras[track][1] &= B00000011;
    stepsData[thestep].noteStepsExtras[track][1] |= stepsData[thestep].noteStepsExtras[track][1] << 2;
    stepsData[thestep].noteStepsExtras[track][1] |= stepsData[thestep].noteStepsExtras[track][1] << 4;
    stepsData[thestep].noteStepsExtras[track][1] |= stepsData[thestep].noteStepsExtras[track][1] << 6;
  }
}

void clearStepsExtrasBits(byte thestep, byte xVar, byte track) {
  bitClear(stepsData[thestep].noteStepsExtras[track][0], (editVariation * 2));
  bitClear(stepsData[thestep].noteStepsExtras[track][0], (editVariation * 2) + 1);
  bitClear(stepsData[thestep].noteStepsExtras[track][1], (editVariation * 2));
  bitClear(stepsData[thestep].noteStepsExtras[track][1], (editVariation * 2) + 1);
}

void resetSegments(byte xs, byte xe) {
  for (byte xx = xs; xx <= xe; xx++) {
    memset(segments[xx], 0, sizeof(segments[0]));
  }
}

void getStepVelocity(byte theStep, byte track, byte variation, bool cleanBits) {
  if (track < DRUM_TRACKS) {
    stepVelocity = bitRead(stepsData[theStep].steps[track], 1 + (variation * 2)) << 1;
    stepVelocity |= bitRead(stepsData[theStep].steps[track], 0 + (variation * 2));
  } else {
    stepVelocity = bitRead(stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0], 1 + (variation * 2)) << 1;
    stepVelocity |= bitRead(stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0], 0 + (variation * 2));
  }
  //
  if (cleanBits) {
    if (track < DRUM_TRACKS) {
      bitClear(stepsData[theStep].steps[track], 1 + (variation * 2));
      bitClear(stepsData[theStep].steps[track], 0 + (variation * 2));
    } else {
      bitClear(stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0], (variation * 2));
      bitClear(stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0], (variation * 2) + 1);
    }
  }
}

void getNoteStepGlideDoubleOff(byte theStep, byte track, byte variation, bool cleanBits) {
  noteStepGlideDoubleOff = bitRead(stepsData[theStep].noteStepsExtras[track][1], 1 + (variation * 2)) << 1;
  noteStepGlideDoubleOff |= bitRead(stepsData[theStep].noteStepsExtras[track][1], 0 + (variation * 2));
  //
  if (cleanBits) {
    bitClear(stepsData[theStep].noteStepsExtras[track][1], (variation * 2));
    bitClear(stepsData[theStep].noteStepsExtras[track][1], (variation * 2) + 1);
  }
}

void rotateSteps(bool rotateLeft) {
  byte noteTrackIndex = curTrack - DRUM_TRACKS;  // Note track index
  byte buffer, bufferDouble;

  // Determine rotation target range
  byte startVar = 0;
  byte endVar = 3;  // Always rotate all 4, totalVariations ignored

  for (byte var = startVar; var <= endVar; var++) {
    // Apply logic only to target variation(s)
    if (mirror == true) {
      // Mirror mode: always rotate each individually
    } else if (forceVariation >= 0) {
      if (var != forceVariation) continue;  // Only rotate forced
    } else if (forceVariation == -1) {
      // No filter — rotate all (as in mirror)
    }

    if (curTrack < DRUM_TRACKS) {
      // DRUM track rotation
      buffer = rotateLeft ? stepsData[0].steps[curTrack] & (0b11 << (var * 2))
                          : stepsData[STEPS - 1].steps[curTrack] & (0b11 << (var * 2));
      bufferDouble = rotateLeft ? stepsData[0].stepsDouble[var] & (1 << curTrack)
                                : stepsData[STEPS - 1].stepsDouble[var] & (1 << curTrack);

      for (byte step = 0; step < STEPS - 1; step++) {
        if (rotateLeft) {
          stepsData[step].steps[curTrack] =
            (stepsData[step + 1].steps[curTrack] & (0b11 << (var * 2))) | (stepsData[step].steps[curTrack] & ~(0b11 << (var * 2)));
          stepsData[step].stepsDouble[var] =
            (stepsData[step + 1].stepsDouble[var] & (1 << curTrack)) | (stepsData[step].stepsDouble[var] & ~(1 << curTrack));
        } else {
          stepsData[STEPS - 1 - step].steps[curTrack] =
            (stepsData[STEPS - 2 - step].steps[curTrack] & (0b11 << (var * 2))) | (stepsData[STEPS - 1 - step].steps[curTrack] & ~(0b11 << (var * 2)));
          stepsData[STEPS - 1 - step].stepsDouble[var] =
            (stepsData[STEPS - 2 - step].stepsDouble[var] & (1 << curTrack)) | (stepsData[STEPS - 1 - step].stepsDouble[var] & ~(1 << curTrack));
        }
      }

      if (rotateLeft) {
        stepsData[STEPS - 1].steps[curTrack] =
          (buffer & (0b11 << (var * 2))) | (stepsData[STEPS - 1].steps[curTrack] & ~(0b11 << (var * 2)));
        stepsData[STEPS - 1].stepsDouble[var] =
          (bufferDouble & (1 << curTrack)) | (stepsData[STEPS - 1].stepsDouble[var] & ~(1 << curTrack));
      } else {
        stepsData[0].steps[curTrack] =
          (buffer & (0b11 << (var * 2))) | (stepsData[0].steps[curTrack] & ~(0b11 << (var * 2)));
        stepsData[0].stepsDouble[var] =
          (bufferDouble & (1 << curTrack)) | (stepsData[0].stepsDouble[var] & ~(1 << curTrack));
      }

    } else {
      // NOTE track rotation
      uint8_t velocityMask = (0b11 << (var * 2));
      uint8_t attribMask = (0b11 << (var * 2));

      uint8_t bufferNote = rotateLeft ? stepsData[0].noteSteps[noteTrackIndex][var]
                                      : stepsData[STEPS - 1].noteSteps[noteTrackIndex][var];
      uint8_t bufferVelocity = rotateLeft ? (stepsData[0].noteStepsExtras[noteTrackIndex][0] & velocityMask)
                                          : (stepsData[STEPS - 1].noteStepsExtras[noteTrackIndex][0] & velocityMask);
      uint8_t bufferAttrib = rotateLeft ? (stepsData[0].noteStepsExtras[noteTrackIndex][1] & attribMask)
                                        : (stepsData[STEPS - 1].noteStepsExtras[noteTrackIndex][1] & attribMask);

      for (byte step = 0; step < STEPS - 1; step++) {
        byte src = rotateLeft ? step + 1 : STEPS - 2 - step;
        byte dst = rotateLeft ? step : STEPS - 1 - step;

        stepsData[dst].noteSteps[noteTrackIndex][var] =
          stepsData[src].noteSteps[noteTrackIndex][var];

        uint8_t vBits = stepsData[src].noteStepsExtras[noteTrackIndex][0] & velocityMask;
        stepsData[dst].noteStepsExtras[noteTrackIndex][0] =
          (stepsData[dst].noteStepsExtras[noteTrackIndex][0] & ~velocityMask) | vBits;

        uint8_t aBits = stepsData[src].noteStepsExtras[noteTrackIndex][1] & attribMask;
        stepsData[dst].noteStepsExtras[noteTrackIndex][1] =
          (stepsData[dst].noteStepsExtras[noteTrackIndex][1] & ~attribMask) | aBits;
      }

      byte wrapStep = rotateLeft ? STEPS - 1 : 0;
      stepsData[wrapStep].noteSteps[noteTrackIndex][var] = bufferNote;
      stepsData[wrapStep].noteStepsExtras[noteTrackIndex][0] =
        (stepsData[wrapStep].noteStepsExtras[noteTrackIndex][0] & ~velocityMask) | bufferVelocity;
      stepsData[wrapStep].noteStepsExtras[noteTrackIndex][1] =
        (stepsData[wrapStep].noteStepsExtras[noteTrackIndex][1] & ~attribMask) | bufferAttrib;
    }
  }
  somethingChangedPattern = true;
}

void randomTrack() {
  //byte track = curTrack; // Use curTrack as the active track
  byte noteTrackIndex = curTrack - DRUM_TRACKS;  // Adjust for note track memory
  byte lastNote = (curTrack >= DRUM_TRACKS) ? patternData.lastNote[noteTrackIndex] : 0;
  byte targetVariation = (forceVariation >= 0 && forceVariation <= 3) ? forceVariation : 0;  // Adjusted range

  for (byte step = 0; step < STEPS; step++) {
    if (random(0, 2) == 0) continue;  // 50% chance to skip this step

    if (curTrack < DRUM_TRACKS) {
      // Drum Tracks Randomization
      for (byte var = 0; var < 4; var++) {
        // Mirror Mode: Generate only Variation A, others will be mirrored
        if (mirror && var != 0) continue;

        // Single Variation Mode: Generate only the selected variation
        if (!mirror && forceVariation >= 0 && var != targetVariation) continue;

        // Serial Mode: Generate for all variations when forceVariation == -1
        if ((stepsData[step].steps[curTrack] & (0b11 << (var * 2))) == 0) {
          byte randomVelocity = random(1, 4);                              // Random value for velocity (1-3)
          stepsData[step].steps[curTrack] |= randomVelocity << (var * 2);  // Set velocity

          // 21% chance to add a 32nd-note step
          bool doubleStep = (random(0, 21) == 0);
          if (doubleStep) {
            stepsData[step].stepsDouble[var] |= (1 << curTrack);  // Set 32nd note flag
          }
        }
      }
    } else {
      // Note Tracks Randomization
      for (byte var = 0; var < 4; var++) {
        // Mirror Mode: Generate only Variation A, others will be mirrored
        if (mirror && var != 0) continue;

        // Single Variation Mode: Generate only the selected variation
        if (!mirror && forceVariation >= 0 && var != targetVariation) continue;

        // Serial Mode: Generate for all variations when forceVariation == -1
        if ((stepsData[step].noteStepsExtras[noteTrackIndex][0] & (0b11 << (var * 2))) == 0) {
          char randomNote = lastNote + random(-12, 13);  // Random pitch within +/-12 semitones
          byte randomVelocity = random(1, 4);            // Random value for velocity (1-3)
          stepsData[step].noteSteps[noteTrackIndex][var] = randomNote;
          stepsData[step].noteStepsExtras[noteTrackIndex][0] &= ~(0b11 << (var * 2));         // Clear velocity bits
          stepsData[step].noteStepsExtras[noteTrackIndex][0] |= randomVelocity << (var * 2);  // Set velocity
        }
      }
    }
  }

  // Mirror Logic
  if (mirror) {
    for (byte step = 0; step < STEPS; step++) {
      if (curTrack < DRUM_TRACKS) {
        // Mirror velocity and 32nd-note flags for drum tracks
        byte baseVelocity = stepsData[step].steps[curTrack] & 0b11;  // Base velocity from A
        for (byte var = 1; var < 4; var++) {
          stepsData[step].steps[curTrack] &= ~(0b11 << (var * 2));       // Clear existing velocity
          stepsData[step].steps[curTrack] |= baseVelocity << (var * 2);  // Mirror velocity
          if (stepsData[step].stepsDouble[0] & (1 << curTrack)) {
            stepsData[step].stepsDouble[var] |= (1 << curTrack);  // Mirror 32nd note flag
          } else {
            stepsData[step].stepsDouble[var] &= ~(1 << curTrack);  // Clear 32nd note flag
          }
        }
      } else {
        // Mirror Note Tracks
        stepsData[step].noteSteps[noteTrackIndex][1] = stepsData[step].noteSteps[noteTrackIndex][0];
        stepsData[step].noteSteps[noteTrackIndex][2] = stepsData[step].noteSteps[noteTrackIndex][0];
        stepsData[step].noteSteps[noteTrackIndex][3] = stepsData[step].noteSteps[noteTrackIndex][0];

        // Copy velocity (Extras)
        stepsData[step].noteStepsExtras[noteTrackIndex][0] &= 0b11;
        stepsData[step].noteStepsExtras[noteTrackIndex][0] |= stepsData[step].noteStepsExtras[noteTrackIndex][0] << 2;
        stepsData[step].noteStepsExtras[noteTrackIndex][0] |= stepsData[step].noteStepsExtras[noteTrackIndex][0] << 4;
        stepsData[step].noteStepsExtras[noteTrackIndex][0] |= stepsData[step].noteStepsExtras[noteTrackIndex][0] << 6;
      }
    }
  }
  // Mark pattern as changed
  somethingChangedPattern = true;
}

void eraseTrack() {
  byte targetVariation = (forceVariation >= 0 && forceVariation <= 3) ? forceVariation : 0;

  for (byte step = 0; step < STEPS; step++) {
    if (curTrack < DRUM_TRACKS) {
      // Drum track: clear per variation
      for (byte var = 0; var < 4; var++) {
        if (mirror || forceVariation == -1 || var == targetVariation) {
          stepsData[step].steps[curTrack] &= ~(0b11 << (var * 2));  // Clear 2-bit velocity
          stepsData[step].stepsDouble[var] &= ~(1 << curTrack);     // Clear 32nd note flag
        }
      }
    } else {
      // Note track
      byte noteTrackIndex = curTrack - DRUM_TRACKS;

      for (byte var = 0; var < 4; var++) {
        if (mirror || forceVariation == -1 || var == targetVariation) {
          stepsData[step].noteStepsExtras[noteTrackIndex][0] &= ~(0b11 << (var * 2));  // Clear velocity bits
          stepsData[step].noteStepsExtras[noteTrackIndex][1] &= ~(0b11 << (var * 2));  // Clear extras (glide/double/etc)
        }
      }
    }
  }

  somethingChangedPattern = true;
}

char stepBeenHold() {
  char holdingNote = -2;
  for (byte x = 0; x < 8; x++) {
    for (byte i = 0; i < 2; i++) {
      if (bitRead(buttonEventWasHolding[i + 1], x)) {
        if (holdingNote == -2) holdingNote = stepsData[x + (i * 8)].noteSteps[curTrack - DRUM_TRACKS][editVariation];
        else return -1;
      }
    }
  }
  return holdingNote;
}

void ShowTemporaryMessage(byte message) {
  showTemporaryMessage = message;
  temporaryMessageCounter = 1;
}

void showOnOrOff(bool showOn) {
  if (showOn) {
    segments[2][6] = S_O;
    segments[2][7] = S_N;
  } else {
    segments[2][5] = S_O;
    segments[2][6] = S_F;
    segments[2][7] = S_F;
  }
}

void showMemoryProtected() {
  segments[1][0] = S_P;
  segments[1][1] = S_R;
  segments[1][2] = S_O;
  segments[1][3] = S_T;
  segments[1][4] = S_E;
  segments[1][5] = S_C;
  segments[1][6] = S_T;

  showOnOrOff(globalData.writeProtectFlash);
}

char getBankLetter(byte value) {
  if (value <= 9) return (char)pgm_read_word(&numbers[value]);
  else if (value == 10) return S_A;
  else if (value == 11) return S_b;
  else if (value == 12) return S_C;
  else if (value == 13) return S_d;
  else if (value == 14) return S_E;
  else if (value == 15) return S_F;
  else return S_G;
}

//END OF LINE