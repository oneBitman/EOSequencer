/*

   Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com

*/

void showMenuCopyPaste() {

  if (patCopy) {
    segments[1][0] = S_P;
    segments[1][1] = S_A;
    segments[1][2] = S_T;
    segments[1][4] = S_C;
    segments[2][5] = S_d;
    segments[2][6] = S_E;
    segments[2][7] = S_L;
  } else {
    segments[1][0] = S_T;
    segments[1][1] = S_R;
    segments[1][2] = S_A;
    segments[1][3] = S_K;
    segments[1][4] = B01011000;
    segments[2][5] = S_A;
    segments[2][6] = S_d;
    segments[2][7] = S_d;
  }


  segments[1][5] = S_O;
  segments[1][6] = S_P;
  segments[1][7] = S_Y;

  segments[2][0] = S_P;
  segments[2][1] = S_S;
  segments[2][2] = S_T;
  segments[2][3] = S_E;
}

void processMenuCopyPaste(byte button) {
  static TrackClipboard clipboard;  // Packed clipboard using noteSteps + noteStepsExtras
  static int8_t copiedForceVariation = -1;

  if (button <= 2) {
    patCopy = !patCopy;
    return;
  }

  if (button >= 4 && button <= 7) {  // COPY
    if (patCopy) {
      clipboardSlot = (clipboardSlot + 1) % CLIPBOARD_SLOTS;
      pagePos = CLIPBOARD_BASE_ADDR + (clipboardSlot * 16);
      cli();
      eraseSector(pagePos);

      if (!flash.writeAnything(pagePos, (uint8_t)0, patternData)) showErrorMsg(flash.error());
      sei();

      pagePos++;
      cli();
      if (!flash.writeAnything(pagePos, (uint8_t)0, stepsData)) showErrorMsg(flash.error());
      sei();
    } else {
      byte sourceVariation = (forceVariation >= 0 && forceVariation <= 3) ? forceVariation : 0;
      bool isDrum = curTrack < DRUM_TRACKS;
      copiedForceVariation = (forceVariation >= 0 && forceVariation <= 3) ? forceVariation : -1;

      for (byte s = 0; s < STEPS; s++) {
        clipboard.noteStepsExtras[s][0] = 0;
        clipboard.noteStepsExtras[s][1] = 0;

        for (byte v = 0; v < 4; v++) {
          if (copiedForceVariation >= 0 && v != sourceVariation) continue;

          byte vel = 0, ext = 0, note = 0;
          if (isDrum) {
            vel = (stepsData[s].steps[curTrack] >> (v * 2)) & 0x03;
            note = patternData.trackNote[curTrack];
            bool doubleStep = (stepsData[s].stepsDouble[v] >> curTrack) & 0x01;
            if (doubleStep) ext = 0b10;
          } else {
            byte idx = curTrack - DRUM_TRACKS;
            vel = (stepsData[s].noteStepsExtras[idx][0] >> (v * 2)) & 0x03;
            ext = (stepsData[s].noteStepsExtras[idx][1] >> (v * 2)) & 0x03;
            note = stepsData[s].noteSteps[idx][v];
          }

          clipboard.noteSteps[s][v] = note;
          clipboard.noteStepsExtras[s][0] |= (vel & 0x03) << (v * 2);
          clipboard.noteStepsExtras[s][1] |= (ext & 0x03) << (v * 2);
        }
      }
    }
  }

  else if ((button >= 8 && button <= 11) || (button >= 13 && button <= 15)) {
    if (patCopy) {
      if (button >= 13) {
        for (byte xs = 0; xs < STEPS; xs++) stepsData[xs].init();
        somethingChangedPattern = true;
      } else {
        pagePos = CLIPBOARD_BASE_ADDR + (clipboardSlot * 16);
        if (!flash.readAnything(pagePos, (uint8_t)0, patternData)) showErrorMsg(flash.error());
        pagePos++;
        if (!flash.readAnything(pagePos, (uint8_t)0, stepsData)) showErrorMsg(flash.error());
        somethingChangedPattern = true;
      }
    } else {
      bool mergePaste = (button >= 13);
      bool isDrum = curTrack < DRUM_TRACKS;
      bool hasCopyForce = copiedForceVariation >= 0;
      bool hasPasteForce = forceVariation >= 0;

      for (byte s = 0; s < STEPS; s++) {
        for (byte v = 0; v < 4; v++) {
          // Determine which variation to read from clipboard
          byte readV =
            (copiedForceVariation >= 0) ? copiedForceVariation : (hasPasteForce ? forceVariation : v);

          // Determine which variation to write into
          byte writeV = 0;
          if (hasPasteForce) {
            writeV = forceVariation;
          } else if (mirror && hasCopyForce) {
            writeV = v;
          } else if (hasCopyForce && !mirror) {
            if (v != copiedForceVariation) continue;
            writeV = v;
          } else {
            writeV = v;
          }

          byte pasteVel = (clipboard.noteStepsExtras[s][0] >> (readV * 2)) & 0x03;
          byte pasteExt = (clipboard.noteStepsExtras[s][1] >> (readV * 2)) & 0x03;
          byte pasteNote = clipboard.noteSteps[s][readV];

          if (isDrum) {
            byte &dst = stepsData[s].steps[curTrack];

            // Merge paste: skip this cell entirely if clipboard says it's empty
            if (mergePaste && pasteVel == 0) continue;

            // Always overwrite velocity bits for this variation
            dst &= ~(0x03 << (writeV * 2));
            dst |= (pasteVel & 0x03) << (writeV * 2);

            stepsData[s].stepsDouble[writeV] &= ~(1 << curTrack);  // Clear existing

            if (pasteExt == 0b10) {
              stepsData[s].stepsDouble[writeV] |= (1 << curTrack);  // Set if clipboard says so
            }
          } else {
            byte idx = curTrack - DRUM_TRACKS;
            if (mergePaste && pasteVel == 0) continue;

            stepsData[s].noteSteps[idx][writeV] = pasteNote;
            bitWrite(stepsData[s].noteStepsExtras[idx][0], writeV * 2 + 0, pasteVel & 0x01);
            bitWrite(stepsData[s].noteStepsExtras[idx][0], writeV * 2 + 1, pasteVel >> 1);
            bitWrite(stepsData[s].noteStepsExtras[idx][1], writeV * 2 + 0, pasteExt & 0x01);
            bitWrite(stepsData[s].noteStepsExtras[idx][1], writeV * 2 + 1, pasteExt >> 1);
          }
        }
      }
      somethingChangedPattern = true;
    }
  }

  curRightScreen = kRightSteps;
}

void showMenu() {
  switch (menuPosition) {

    case menuTrackLen:
      segments[0][0] = S_P_DOT;

      segments[0][1] = S_C_DOT;  //

      segments[1][0] = S_T;
      segments[1][1] = S_R;
      segments[1][2] = S_A;
      segments[1][3] = S_K;

      segments[1][4] = S_L;
      segments[1][5] = S_E;
      segments[1][6] = S_N;

      printNumber(2, 5, patternData.trackLen[curTrack]);
      break;

    case menuTrackProbability:
      segments[0][0] = S_P_DOT;

      segments[0][1] = S_E_DOT;  //

      segments[1][0] = S_T;
      segments[1][1] = S_R;
      segments[1][2] = S_A;
      segments[1][3] = S_K;

      segments[1][4] = S_S;
      segments[1][5] = S_T;
      segments[1][6] = S_E;
      segments[1][7] = S_P;
      segments[2][0] = S_S;
      segments[2][1] = S_K;
      segments[2][2] = S_I;
      segments[2][3] = S_P;

      printNumber(2, 5, patternData.trackProbability[curTrack]);
      break;


    case menuShuffle:
      segments[0][0] = S_P_DOT;

      segments[0][1] = S_d_DOT;  //

      segments[1][0] = S_S;
      segments[1][1] = S_H;
      segments[1][2] = S_U;
      segments[1][3] = S_F;
      segments[1][4] = S_F;
      segments[1][5] = S_L;
      segments[1][6] = S_E;


      printNumber(2, 5, patternData.shuffleDelay);
      break;


    case menuProgramChange:
      segments[0][0] = S_P_DOT;

      segments[0][1] = S_A_DOT;  // menuProgramChange

      segments[1][0] = S_P;
      segments[1][1] = S_R;
      segments[1][2] = S_O;
      segments[1][3] = S_G;
      segments[1][4] = S_C;
      segments[1][5] = S_H;
      segments[1][6] = S_N;
      segments[1][7] = S_G;

      if (patternData.programChange[curTrack] > 0) printNumber(2, 5, patternData.programChange[curTrack]);
      else showOnOrOff(false);
      break;

    case menuMIDIChannel:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B11100110;  // menuMIDIChannel
      segments[1][0] = S_N;
      segments[1][1] = S_I;
      segments[1][2] = S_d;
      segments[1][3] = S_I;

      segments[1][5] = S_C;
      segments[1][6] = S_H;
      segments[1][7] = S_N;

      printNumber(2, 5, patternData.trackMidiCH[curTrack] + 1);
      break;

    case menuPtPlays:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B10000110;  // menuPtPlays
      segments[1][0] = S_P;
      segments[1][1] = S_A;
      segments[1][2] = S_T;

      segments[1][4] = S_R;
      segments[1][5] = S_E;
      segments[1][6] = S_P;
      segments[1][7] = S_E;
      segments[2][0] = S_A;
      segments[2][1] = S_T;

      if (patternData.playsPattern == 0) {
        segments[2][5] = S_I;
        segments[2][6] = S_N;
        segments[2][7] = S_F;
      } else printNumber(2, 5, patternData.playsPattern);
      break;

    case menuPtPlaysChain:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B11001111;  // menuPtPlaysChain
      segments[1][0] = S_C;
      segments[1][1] = S_H;
      segments[1][2] = S_A;
      segments[1][3] = S_I;
      segments[1][4] = S_N;

      segments[1][6] = S_R;
      segments[1][7] = S_E;
      segments[2][0] = S_P;
      segments[2][1] = S_E;
      segments[2][2] = S_A;
      segments[2][3] = S_T;

      if (patternData.playsChain == 0) {
        segments[2][5] = S_I;
        segments[2][6] = S_N;
        segments[2][7] = S_F;
      } else if (patternData.playsChain == 1) showOnOrOff(false);
      else printNumber(2, 5, patternData.playsChain - 1);
      break;

    case menuPtNext:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B11011011;  // menuPtNext
      segments[1][0] = S_N;
      segments[1][1] = S_E;
      segments[1][2] = S_X;
      segments[1][3] = S_T;

      segments[1][5] = S_P;
      segments[1][6] = S_A;
      segments[1][7] = S_T;

      if (patternData.nextPattern == 0) {

        segments[2][0] = S_P;
        segments[2][1] = S_L;
        segments[2][2] = S_U;
        segments[2][3] = S_S;

        segments[2][5] = S_O;
        segments[2][6] = S_N;
        segments[2][7] = S_E;
      } else {
        printNumber(2, 5, ((patternData.nextPattern - 1) % 16) + 1);
        segments[2][5] = getBankLetter(((patternData.nextPattern - 1) / 16) + 1);
        segments[2][5] |= B10000000;
      }

      break;

    case menuNote:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B11101101;  // menuNote
      segments[1][0] = S_d;
      segments[1][1] = S_r;
      segments[1][2] = S_u;
      segments[1][3] = S_n;

      segments[1][4] = S_N;
      segments[1][5] = S_O;
      segments[1][6] = S_T;
      segments[1][7] = S_E;

      if (curTrack < DRUM_TRACKS) {

        printNumber(2, 0, patternData.trackNote[curTrack]);
        printMIDInote(patternData.trackNote[curTrack], 2, 4, 7);

      } else {
        printDashDash(2, 6);
      }
      break;

    case menuNoteLen:
      segments[0][0] = S_P_DOT;

      segments[0][1] = B11111101;  // menuNoteLen

      segments[1][0] = S_N;
      segments[1][1] = S_O;
      segments[1][2] = S_T;
      segments[1][3] = S_E;

      segments[1][5] = S_L;
      segments[1][6] = S_E;
      segments[1][7] = S_N;

      printNumber(2, 5, patternData.drumNoteLen[curTrack]);
      break;

    case menuPulseOut:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11111101;  // menuPulseOut

      segments[1][2] = S_P;
      segments[1][3] = S_U;
      segments[1][4] = S_L;
      segments[1][5] = S_S;
      segments[1][6] = S_E;

      segments[2][0] = S_R;
      segments[2][1] = S_A;
      segments[2][2] = S_T;
      segments[2][3] = S_E;

      if (globalData.tickOut > 0) printNumber(2, 5, globalData.tickOut);
      else showOnOrOff(false);
      break;

    case menuPulseOutLen:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B10000111;  // menuPulseOutLen
      segments[1][2] = S_P;
      segments[1][3] = S_U;
      segments[1][4] = S_L;
      segments[1][5] = S_S;
      segments[1][6] = S_E;

      segments[2][0] = S_L;
      segments[2][1] = S_E;
      segments[2][2] = S_N;
      segments[2][3] = S_G;

      printNumber(2, 5, globalData.tickOutLen);
      break;

    case menuBrightness:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11111100;  // menuBrightness

      segments[1][1] = S_b;
      segments[1][2] = S_R;
      segments[1][3] = S_I;
      segments[1][4] = S_G;
      segments[1][5] = S_H;
      segments[1][6] = S_T;
      segments[2][0] = S_d;
      segments[2][1] = S_I;
      segments[2][2] = S_S;
      segments[2][3] = S_P;

      printNumber(2, 5, globalData.Brightness);  // Print brightness as an integer (0-5)
      break;

    case menuProtect:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11101111;  // menuProtect
      showMemoryProtected();
      break;

    case menuAccent1:

    case menuAccent2:

    case menuAccent3:
      segments[0][0] = S_P_DOT;

      segments[0][1] = (char)pgm_read_word(&numbers[menuPosition - menuAccent1 + 7]);
      segments[0][1] |= B10000000;
      segments[1][0] = S_U;
      segments[1][1] = S_E;
      segments[1][2] = S_L;
      segments[1][3] = S_O;
      segments[1][4] = S_C;
      segments[1][5] = S_I;
      segments[1][6] = S_T;
      segments[1][7] = S_Y;

      //use actual segment char gfx instead of numbers
      segments[2][1] = (char)pgm_read_word(&stepChars[menuPosition - menuAccent1 + 1]);
      printNumber(2, 4, patternData.accentValues[menuPosition - menuAccent1]);
      break;

    case menuVariationsABCD:
      segments[0][0] = S_P_DOT;
      segments[0][1] = S_b_DOT;  // menuVariationsABCD
      segments[1][0] = S_P;
      segments[1][1] = S_A;
      segments[1][2] = S_T;

      segments[1][4] = S_U;
      segments[1][5] = S_A;
      segments[1][6] = S_R;
      segments[1][7] = S_I;

      segments[2][4] = S_A;
      if (patternData.totalVariations >= 2) segments[2][5] = S_b;
      if (patternData.totalVariations >= 3) segments[2][6] = S_C;
      if (patternData.totalVariations >= 4) segments[2][7] = S_d;
      break;

    case menuSyncOut:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11011011;  // menuSyncOut
      segments[1][0] = S_C;
      segments[1][1] = S_L;
      segments[1][2] = S_O;
      segments[1][3] = S_C;
      segments[1][4] = S_K;

      segments[1][5] = S_O;
      segments[1][6] = S_U;
      segments[1][7] = S_T;

      showOnOrOff(globalData.seqSyncOut);
      break;

    case menuClockDivider:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11001111;  //
      segments[1][0] = S_C;
      segments[1][1] = S_L;
      segments[1][2] = S_O;
      segments[1][3] = S_C;
      segments[1][4] = S_K;

      segments[1][5] = S_d;
      segments[1][6] = S_I;
      segments[1][7] = S_U;
      segments[2][0] = S_I;
      segments[2][1] = S_d;
      segments[2][2] = S_E;
      segments[2][3] = S_R;

      printNumber(2, 5, globalData.midiClockDivide);
      break;

    case menuMIDIinPattern:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11111111;  // menuMIDIinPattern
      segments[1][0] = S_N;
      segments[1][1] = S_I;
      segments[1][2] = S_d;
      segments[1][3] = S_I;

      segments[1][5] = S_P;
      segments[1][6] = S_A;
      segments[1][7] = S_T;

      segments[2][0] = S_C;
      segments[2][1] = S_H;
      segments[2][2] = S_N;
      segments[2][3] = S_G;

      if (globalData.midiInputToPatternChannel > 0) printNumber(2, 5, globalData.midiInputToPatternChannel);
      else showOnOrOff(false);
      break;

    case menuClockType:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B10000110;  // menuClockType
      segments[1][0] = S_C;
      segments[1][1] = S_L;
      segments[1][2] = S_O;
      segments[1][3] = S_C;
      segments[1][4] = S_K;

      if (globalData.midiClockAuto) {

        segments[2][2] = S_A;
        segments[2][3] = S_U;
        segments[2][4] = S_T;
        segments[2][5] = S_O;
      } else {
        segments[2][1] = S_I;
        segments[2][2] = S_N;
        segments[2][3] = S_T;
        segments[2][4] = S_E;
        segments[2][5] = S_R;
        segments[2][6] = S_N;
      }
      break;

    case menuMultitrack:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11101101;
      segments[1][0] = S_T;
      segments[1][1] = S_R;
      segments[1][2] = S_A;
      segments[1][3] = S_K;

      segments[1][5] = S_R;
      segments[1][6] = S_E;
      segments[1][7] = S_C;

      if (globalData.multitrackRec) {

        segments[2][1] = S_N;
        segments[2][2] = S_U;
        segments[2][3] = S_L;
        segments[2][4] = S_T;
        segments[2][5] = S_I;

      } else {
        segments[2][1] = S_S;
        segments[2][2] = S_I;
        segments[2][3] = S_N;
        segments[2][4] = S_G;
        segments[2][5] = S_L;
        segments[2][6] = S_E;
      }

      break;

    case menuMidiThru:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11100110;
      segments[1][3] = S_N;
      segments[1][4] = S_I;
      segments[1][5] = S_d;
      segments[1][6] = S_I;

      segments[2][0] = S_T;
      segments[2][1] = S_H;
      segments[2][2] = S_R;
      segments[2][3] = S_U;

      showOnOrOff(globalData.midiThru);
      break;

    case menuInit:
      segments[0][0] = S_G_DOT;

      segments[0][1] = B11110111;  // menuInit
      if (initMode == 0) {
        segments[1][0] = S_d;
        segments[1][1] = B01011100;

        segments[1][3] = S_R;
        segments[1][4] = S_E;
        segments[1][5] = S_S;
        segments[1][6] = S_E;
        segments[1][7] = S_T;

        segments[2][2] = S_N;
        segments[2][3] = S_O;
        segments[2][4] = S_N;
        segments[2][5] = S_E;
      } else if (initMode == 1) {
        segments[1][0] = S_d;
        segments[1][1] = B01011100;

        segments[1][3] = S_S;
        segments[1][4] = S_E;
        segments[1][5] = S_T;
        segments[1][6] = S_U;
        segments[1][7] = S_P;

        segments[2][0] = S_P;
        segments[2][1] = S_A;
        segments[2][2] = S_T;
        segments[2][2] |= B10000000;
        segments[2][3] = B01011011;
        segments[2][4] = S_b;
        segments[2][5] = S_A;
        segments[2][6] = S_N;
        segments[2][7] = S_K;
      } else if (initMode == 2) {
        segments[1][0] = S_d;
        segments[1][1] = B01011100;

        segments[1][3] = S_C;
        segments[1][4] = S_L;
        segments[1][5] = S_O;
        segments[1][6] = S_N;
        segments[1][7] = S_E;

        segments[2][0] = S_P;
        segments[2][1] = S_A;
        segments[2][2] = S_T;
        segments[2][2] |= B10000000;
        segments[2][3] = B01011011;
        segments[2][4] = S_b;
        segments[2][5] = S_A;
        segments[2][6] = S_N;
        segments[2][7] = S_K;
      } else if (initMode == 3) {
        segments[1][0] = S_d;
        segments[1][1] = B01011100;
        segments[1][2] = B00000000;
        segments[1][3] = S_R;
        segments[1][4] = S_E;
        segments[1][5] = S_S;
        segments[1][6] = S_E;
        segments[1][7] = S_T;

        segments[2][2] = S_b;
        segments[2][3] = S_A;
        segments[2][4] = S_N;
        segments[2][5] = S_K;
      } else if (initMode == 4) {
        segments[1][0] = S_d;
        segments[1][1] = B01011100;
        segments[1][2] = B00000000;
        segments[1][3] = S_R;
        segments[1][4] = S_E;
        segments[1][5] = S_S;
        segments[1][6] = S_E;
        segments[1][7] = S_T;

        segments[2][1] = S_F;
        segments[2][2] = S_A;
        segments[2][3] = S_C;
        segments[2][4] = S_T;
        segments[2][5] = S_O;
        segments[2][6] = S_R;
        segments[2][7] = S_Y;
      }
      break;
  }
}

void processMenuFORMAT() {
  switch (menuPosition) {
    case menuInit:
      if (seqPlaying) ShowTemporaryMessage(kStopSequencer);
      else {
        if (initMode == 1 || initMode == 2 || initMode == 3) {
          stopSequencer();
          byte backToPattern = currentPattern;
          reset();
          if (initMode == 3) resetPatternBank();  // sets back all steps to init
          if (initMode == 3) patternData.init();
          int porc = 0;
          initPatternBank(currentPatternBank, true, porc, true, initMode);
          if (initMode == 3) loadPatternBank(currentPatternBank);
          if (initMode == 1 || initMode == 2) loadPattern(backToPattern);
          if (curRightScreen == kRightMenu) curRightScreen = kRightSteps;
        } else if (initMode == 4) {
          stopSequencer();
          reset();
          resetPatternBank();  // sets back all steps to init
          patternData.init();
          globalData.init();
          currentPatternBank = 0;
          flashInit(true);
          if (curRightScreen == kRightMenu) curRightScreen = kRightSteps;
        } else if (curRightScreen == kRightMenu) curRightScreen = kRightSteps;
      }
      break;
  }
}

void processMenu(char value) {
  switch (menuPosition) {

    case menuProgramChange:
      if (value > 0 && patternData.programChange[curTrack] < 128) patternData.programChange[curTrack]++;
      else if (value < 0 && patternData.programChange[curTrack] > 0) patternData.programChange[curTrack]--;
      sendMIDIProgramChange(curTrack);

      somethingChangedPattern = true;
      break;

    case menuMIDIChannel:
      if (value > 0 && patternData.trackMidiCH[curTrack] < 15) patternData.trackMidiCH[curTrack]++;
      else if (value < 0 && patternData.trackMidiCH[curTrack] > 0) patternData.trackMidiCH[curTrack]--;
      somethingChangedPattern = true;
      break;

    case menuTrackLen:
      if (value > 0 && patternData.trackLen[curTrack] < 16) patternData.trackLen[curTrack]++;
      else if (value < 0 && patternData.trackLen[curTrack] > 1) patternData.trackLen[curTrack]--;
      somethingChangedPattern = true;
      break;

    case menuTrackProbability:  // Step playback probability
      if (value > 0 && patternData.trackProbability[curTrack] < 100) {
        patternData.trackProbability[curTrack] += 10;
      } else if (value < 0 && patternData.trackProbability[curTrack] > 0) {
        patternData.trackProbability[curTrack] -= 10;
      }
      somethingChangedPattern = true;
      break;

    case menuShuffle:
      if (value > 0 && patternData.shuffleDelay < 5) patternData.shuffleDelay++;
      else if (value < 0 && patternData.shuffleDelay > 0) patternData.shuffleDelay--;

      somethingChangedPattern = true;
      break;

    case menuClockDivider:
      if (value > 0 && globalData.midiClockDivide < 16) globalData.midiClockDivide++;
      else if (value < 0 && globalData.midiClockDivide > 1) globalData.midiClockDivide--;

      somethingChangedGlobal = true;
      break;

    case menuNote:
      if (curTrack < DRUM_TRACKS) {
        if (value > 0 && patternData.trackNote[curTrack] < 127) patternData.trackNote[curTrack]++;
        else if (value < 0 && patternData.trackNote[curTrack] > 0) patternData.trackNote[curTrack]--;
        somethingChangedPattern = true;
      }
      break;

    case menuPulseOut:
      if (value > 0 && globalData.tickOut < 16) globalData.tickOut++;
      else if (value < 0 && globalData.tickOut > 0) globalData.tickOut--;
      if (globalData.tickOut > 0) {
        tickOutCounterLen = globalData.tickOut;
      } else {
        pulseOut(false);
        tickOutPinState = false;
      }
      somethingChangedGlobal = true;
      break;

    case menuPulseOutLen:
      if (value > 0 && globalData.tickOutLen < 16) globalData.tickOutLen++;
      else if (value < 0 && globalData.tickOutLen > 1) globalData.tickOutLen--;
      somethingChangedGlobal = true;
      break;

    case menuBrightness:
      if (value > 0 && globalData.Brightness < 2) globalData.Brightness++;       // Increase brightness
      else if (value < 0 && globalData.Brightness > 0) globalData.Brightness--;  // Decrease brightness

      somethingChangedGlobal = true;
      break;


    case menuNoteLen:
      if (value > 0 && patternData.drumNoteLen[curTrack] < 129) patternData.drumNoteLen[curTrack]++;
      else if (value < 0 && patternData.drumNoteLen[curTrack] > 1) patternData.drumNoteLen[curTrack]--;
      somethingChangedPattern = true;
      break;

    case menuAccent1:
    case menuAccent2:
    case menuAccent3:
      {
        byte i = menuPosition - menuAccent1;

        if (value > 0 && patternData.accentValues[i] < 127) {
          patternData.accentValues[i]++;
          // Enforce accent1 < accent2 < accent3
          if (i == 0 && patternData.accentValues[0] >= patternData.accentValues[1]) patternData.accentValues[1] = patternData.accentValues[0] + 1;
          if (i == 1 && patternData.accentValues[1] >= patternData.accentValues[2]) patternData.accentValues[2] = patternData.accentValues[1] + 1;
        } else if (value < 1 && patternData.accentValues[i] > 0) {
          patternData.accentValues[i]--;
          // Enforce accent1 < accent2 < accent3
          if (i == 2 && patternData.accentValues[2] <= patternData.accentValues[1]) patternData.accentValues[1] = patternData.accentValues[2] - 1;
          if (i == 1 && patternData.accentValues[1] <= patternData.accentValues[0]) patternData.accentValues[0] = patternData.accentValues[1] - 1;
        }

        // Clamp to safe bounds
        patternData.accentValues[0] = constrain(patternData.accentValues[0], 0, 125);
        patternData.accentValues[1] = constrain(patternData.accentValues[1], patternData.accentValues[0] + 1, 126);
        patternData.accentValues[2] = constrain(patternData.accentValues[2], patternData.accentValues[1] + 1, 127);

        somethingChangedPattern = true;
        break;
      }

    case menuProtect:
      globalData.writeProtectFlash = !globalData.writeProtectFlash;
      saveGlobalData(true);
      break;

    case menuPtNext:
      if (value > 0 && patternData.nextPattern < 256) patternData.nextPattern++;
      else if (value < 1 && patternData.nextPattern > 0) patternData.nextPattern--;

      somethingChangedPattern = true;
      break;

    case menuPtPlays:
      if (value > 0 && patternData.playsPattern < 255) patternData.playsPattern++;
      else if (value < 1 && patternData.playsPattern > 0) patternData.playsPattern--;

      somethingChangedPattern = true;
      break;

    case menuPtPlaysChain:
      if (value > 0 && patternData.playsChain < 255) patternData.playsChain++;
      else if (value < 1 && patternData.playsChain > 0) patternData.playsChain--;

      somethingChangedPattern = true;
      break;

    case menuMIDIinPattern:
      if (value > 0 && globalData.midiInputToPatternChannel < 16) globalData.midiInputToPatternChannel++;
      else if (value < 1 && globalData.midiInputToPatternChannel > 0) globalData.midiInputToPatternChannel--;
      somethingChangedGlobal = true;
      break;

    case menuClockType:
      globalData.midiClockAuto = !globalData.midiClockAuto;

      somethingChangedGlobal = true;
      break;

    case menuVariationsABCD:
      if (value > 0 && patternData.totalVariations < 4) patternData.totalVariations++;
      else if (value < 1 && patternData.totalVariations > 1) patternData.totalVariations--;
      somethingChangedPattern = true;
      break;

    case menuSyncOut:
      globalData.seqSyncOut = !globalData.seqSyncOut;
      somethingChangedGlobal = true;
      break;

    case menuMultitrack:
      globalData.multitrackRec = !globalData.multitrackRec;
      somethingChangedGlobal = true;
      break;

    case menuMidiThru:
      globalData.midiThru = !globalData.midiThru;
      somethingChangedGlobal = true;
      break;


    case menuInit:
      if (value > 0 && initMode < 4) initMode++;
      else if (value < 1 && initMode > 0) initMode--;
      break;
  }
}
//END OF LINE