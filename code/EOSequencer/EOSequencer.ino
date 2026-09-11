/*
 * EOSequencer — fork of Beat707NXT
 *
 * Copyright (c) 2018–2025 Gert Borovcak
 * Original Beat707 portions Copyright (c) 2011–2018 William Kalfelz (Beat707 / Wusik.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Project: https://www.eoseq.com
 * Support William: https://ko-fi.com/williamkalfelz  •  https://www.patreon.com/williamkwusik
 */

#include "Lib_Flash.h"
#include "Functions.h"
#include "Variables.h"
#include <util/atomic.h>

void setup() {
  DDRC = 0xFF;  // For the Pulse-Out (analog pins, A54 Header)
  pulseOut(false);
  realBPM = 120;  //the BPM set at startup
  
  startMIDIinterface();
  initTM1638();
  reset();
  resetPatternBank();  //sets back all steps to init
  patternData.init();
  globalData.init();
  //splashScreen();  //removed to save memory

  readButtons();
  masterOffset = 0;

  if (buttons[2] & _BV(0)) {
    for (;;) {
      readButtons();

      if (buttons[1] & _BV(6)) masterOffset = 6;
      else if (buttons[1] & _BV(5)) masterOffset = 5;
      else if (buttons[1] & _BV(4)) masterOffset = 4;
      else if (buttons[1] & _BV(3)) masterOffset = 3;
      else if (buttons[1] & _BV(2)) masterOffset = 2;
      else if (buttons[1] & _BV(1)) masterOffset = 1;
      else if (buttons[1] & _BV(0)) masterOffset = 0;

      segments[1][0] = S_S;
      segments[1][1] = S_L;
      segments[1][2] = S_O;
      segments[1][3] = S_T;
      printNumber(1, 5, masterOffset + 1);
      sendScreen();

      // keep halting while ANY of these are held
      if (!(buttons[2] & _BV(0))) break;

      _delay_ms(10);
    }
  }
  //sendScreen();
  waitMs(800);

  buttons[0] = buttons[1] = buttons[2] = 0;

  flashInit(false);
  loadPatternBank(0);

  calculateSequencer++;
  startup = false;
  setupTimerForExternalMIDISync(!midiClockInternal);
}

void loop() {


  if (sysexActive) {
    handleSYSEXInput();
  } else {  // Only run these sections when sysex is not active

    if (dumpReceived) {
      ShowTemporaryMessage(kSysexDump);
      dumpReceived = false;
    }

    if (!midiClockInternal) {
      doTickSequencer();  //
    }

    checkPatternStream();

    unsigned long currentTime = millis();
    unsigned long frameDuration = currentTime - previousFrameTime;
    previousFrameTime = currentTime;

    accumulatedInterfaceTime += frameDuration;
    accumulatedInterfaceTimeAlltime += frameDuration;

    if (accumulatedInterfaceTimeAlltime >= 400) {  //400mS throughput check
      eventsPrevious = eventsSent / 10;
      eventsSent = accumulatedInterfaceTimeAlltime = 0;
    }

    // Check interface if enough time has passed
    const byte interfaceCheckThreshold = 12;  // 12mS

    if (accumulatedInterfaceTime >= interfaceCheckThreshold) {
      readButtons();
    }

    if (midiClockInternal) {
      handleMIDIInput();
      if (sysexActive) return;
    }

    if (accumulatedInterfaceTime >= interfaceCheckThreshold) {
      checkInterface();
      accumulatedInterfaceTime = 0;
    }

    // Frame skip logic
    static uint8_t framePattern[6];

    static const uint8_t pattern_111111[6] = { 1, 1, 1, 1, 1, 1 };
    static const uint8_t pattern_111110[6] = { 1, 1, 1, 0, 1, 1 };
    static const uint8_t pattern_111100[6] = { 1, 1, 0, 0, 1, 1 };
    static const uint8_t pattern_110100[6] = { 1, 1, 0, 0, 0, 1 };
    static const uint8_t pattern_100100[6] = { 1, 0, 0, 0, 0, 1 };
    static const uint8_t pattern_100000[6] = { 1, 0, 0, 0, 0, 0 };

    const uint8_t* patternEvents;
    const uint8_t* patternBPM;

    // Choose pattern based on event load
    if (eventsPrevious <= 0x05) {
      patternEvents = pattern_111111;
      visualPattern = B00000000;
    } else if (eventsPrevious <= 0x20) {
      patternEvents = pattern_111111;
      visualPattern = B00001000;
    } else if (eventsPrevious <= 0x2D) {
      patternEvents = pattern_111110;
      visualPattern = B00001000;
    } else if (eventsPrevious <= 0x3A) {
      patternEvents = pattern_111100;
      visualPattern = B01001000;
    } else if (eventsPrevious <= 0x47) {
      patternEvents = pattern_110100;
      visualPattern = B01001000;
    } else if (eventsPrevious <= 0x54) {
      patternEvents = pattern_100100;
      visualPattern = B01001000;
    } else {
      patternEvents = pattern_100000;
      visualPattern = B01001001;
    }

    // Choose pattern based on BPM (high BPM = reduce frames)
    if (realBPM < 200) {
      patternBPM = pattern_111111;
    } else if (realBPM < 300) {
      patternBPM = pattern_111110;
    } else if (realBPM < 400) {
      patternBPM = pattern_111100;
    } else if (realBPM < 500) {
      patternBPM = pattern_110100;
    } else if (realBPM < 800) {
      patternBPM = pattern_100100;
    } else {
      patternBPM = pattern_100000;
    }

    // Final pattern = pattern with most zeroes
    for (byte i = 0; i < 6; i++) {
      framePattern[i] = patternEvents[i] && patternBPM[i];
    }

    // Screen update check
    if (screenUpdate) {
      screenUpdate = false;

      if (framePattern[(PPQcounter + 5) % 6]) {
        createScreen();
        sendScreen();
      }
    }

    if (editingNoteTranspose != -127 && stepBeenHold() == -2) {
      editingNoteTranspose = -127;
    }

    // Animation timer, not synced to BPM
    const unsigned long animationCycleTime = 6000;  // 6 seconds
    const byte animationSteps = 16;

    unsigned long ms = millis() % animationCycleTime;
    quantizedValue = ms / (animationCycleTime / animationSteps);
    //}
  }
}
//END OF LINE
