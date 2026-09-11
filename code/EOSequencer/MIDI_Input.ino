/*

   Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com

*/

void handleMIDIInput() {

  if (Serial.available() > 0 && sysexActive == false) {
    byte inputByte = Serial.peek();

    if (inputByte == 0xF0) {
      if (Serial.available() < 3) return;  // Wait until full header is available

      Serial.read();  // Consume F0

      byte sig1 = Serial.peek();  // Check next byte without removing yet
      if (sig1 != 0x7D) {
        // Reject dump, consume until F7 or buffer empty
        while (Serial.available()) {
          if (Serial.read() == 0xF7) break;
        }
        return;
      }

      Serial.read();  // Consume 7D

      byte sig2 = Serial.peek();
      if (sig2 != 0x45) {
        while (Serial.available()) {
          if (Serial.read() == 0xF7) break;
        }
        return;
      }

      Serial.read();  // Consume 45

      if (!seqPlaying) {
        receivingSysEx = true;
        sysexActive = true;
        byteIndex = 0;
        highNibble = false;
        lastStatusByte = 0;
        allowRunningStatus = false;
        return;
      } else {
        // Dump not allowed during playback
        while (Serial.available()) {
          if (Serial.read() == 0xF7) break;
        }
        return;
      }
    }

    // Handle real-time messages (F8–FF)
    if (inputByte >= 0xF8) {
      inputByte = Serial.read();  // consume it

      switch (inputByte) {
        case 0xF8:  // MIDI Clock Tick


          if (!midiClockInternal && globalData.seqSyncOut) {
            Serial.write(0xF8);

            eventsSent++;
          }

          if (!midiClockInternal) {
            outputMIDIBuffer();
            pulseOut(tickOutPinState);
            ledsBufferFlip();
            calculateSequencer++;
          }

          break;
        case 0xFA:  // Start
        case 0xFB:  // Continue
          if (globalData.midiClockAuto && !seqPlaying) {
            midiClockInternal = false;
            setupTimerForExternalMIDISync(true);
            startSequencer();
          }
          break;
        case 0xFC:  // Stop
          if (!midiClockInternal) {
            if (globalData.seqSyncOut) Serial.write(0xFC);
            midiClockInternal = true;
            stopTimer(true);
            updateSequencerSpeed(true);
            startTimer(true);
            stopSequencer();
          }
          midiClockInternal = true;
          break;
          // FE, FF, etc. are ignored
      }
    }
    byte peekByte = Serial.peek();
    // Consume any FE (Active Sensing) messages before continuing
    while (Serial.available() > 0 && Serial.peek() == 0xFE) {
      Serial.read();
    }
    if (peekByte >= 0xF8 || peekByte == 0xF0) return;  //more realtime bytes coming in
    if (midiInputStage == 0) {
      midiInputBuffer[0] = 0;
      midiInputBuffer[1] = 0;

      byte peekByte = Serial.peek();

      if ((peekByte & 0x80) == 0) {  // Data byte
        if (allowRunningStatus && lastStatusByte >= 0x80 && lastStatusByte < 0xF0) {
          if (Serial.available() < 2) return;  // Don't enter running status flow unless both data bytes are there

          midiInputBuffer[0] = lastStatusByte;
          midiInputStage = 1;
          allowRunningStatus = false;
          return;
        }
        // Otherwise reset
        lastStatusByte = 0;
        allowRunningStatus = false;
      }

      // Byte is a status byte and should be consumed
      byte status = Serial.read();
      allowRunningStatus = false;

      // Accept Channel Voice (0x80–0xEF) and System Common (0xF1, F2, F3)
      if ((status >= 0x80 && status <= 0xEF) || status == 0xF1 || status == 0xF2 || status == 0xF3) {
        lastStatusByte = status;
      } else {
        lastStatusByte = 0;  // Don't allow running status for anything else
      }

      // Handle special system messages
      if (status == 0xF6 || status == 0xF7 || status == 0xF4 || status == 0xF5) {
        lastStatusByte = 0;  // Reset running status
        midiInputStage = 0;  // Abort
        return;
      }

      midiInputBuffer[0] = status;
      midiInputStage = 1;
      return;
    }

    // Stage 1: Second byte
    else if (midiInputStage == 1) {
      midiInputBuffer[1] = Serial.read();
      if (
        midiInputBuffer[0] == 0xF2 ||           // Song Position Pointer (3-byte)
        (midiInputBuffer[0] & 0xF0) == 0xA0 ||  // Poly AT
        (midiInputBuffer[0] & 0xF0) == 0xB0 ||  // CC
        (midiInputBuffer[0] & 0xF0) == 0xE0     // Pitch Bend
      ) {
        midiInputStage = 2;
        return;
      } else if (
        midiInputBuffer[0] == 0xF1 ||  // MTC QF (System Common)
        midiInputBuffer[0] == 0xF3     // Song Select (System Common)
      ) {
        lastStatusByte = 0;  // ⬅ reset running status
        midiInputStage = 0;
        return;
      } else if (
        (midiInputBuffer[0] & 0xF0) == 0xC0 ||  // Program Change
        (midiInputBuffer[0] & 0xF0) == 0xD0     // Channel Pressure
      ) {
        // unsupported, but valid for running status — do NOT reset it
        allowRunningStatus = true;
        midiInputStage = 0;
        return;
      }

      switch (midiInputBuffer[0] & 0xF0) {
        case 0x80:
        case 0x90:
          midiInputStage = 2;
          return;
        default:
          allowRunningStatus = false;  // is this needed here?
          midiInputStage = 0;
          return;
      }
    }

    // Stage 2: Third byte
    else if (midiInputStage == 2) {
      byte lastData = Serial.read();
      midiInputStage = 0;
      allowRunningStatus = true;
      byte channel = midiInputBuffer[0] & 0xF;
      byte noteP = midiInputBuffer[1];
      byte velocityP = lastData;

      switch (midiInputBuffer[0] & 0xF0) {
        case 0x80:  // Note Off
          {
            // if (recordEnabled && seqPlaying) //addRecordNotes(midiInputBuffer[1], 0, channel); // we do not want to record note offs
            if (channel == (globalData.midiInputToPatternChannel - 1)) break;  //do not process note to pattern switching note off
            if (globalData.multitrackRec) {

              bool matchesTrack = false;  //only midi thru notes that are defined in setup

              for (byte t = 0; t < (DRUM_TRACKS + NOTE_TRACKS); t++) {
                if (t < DRUM_TRACKS) {
                  if ((patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) && patternData.trackNote[t] == midiInputBuffer[1]) {
                    matchesTrack = true;
                    midiInputActivityFlags &= ~(1 << t);  // clear led bit for track
                  }
                } else {
                  if (patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) {
                    matchesTrack = true;
                    midiInputActivityFlags &= ~(1 << t);  // clear led bit for track
                  }
                }
              }

              if (matchesTrack && globalData.midiThru) {
                // Send MIDI thru
                Serial.write(midiInputBuffer[0]);  //MSG type & Channel
                Serial.write(midiInputBuffer[1]);  //Note number
                Serial.write(lastData);            // Velocity
              }
            }

            else {  //single track rec
              if (curTrack < DRUM_TRACKS) {
                // Drum Tracks - Modify Channel and Note - quantizing ANY midi input channel & note to current track setup
                byte newChannel = patternData.trackMidiCH[curTrack] & 0x0F;  // Mask lower 4 bits for the channel
                byte newNote = patternData.trackNote[curTrack] & 0x7F;       // Mask the note to keep it within valid range
                midiInputActivityFlags &= ~(1 << curTrack);

                // Replace the channel in midiInputBuffer[0] and the note in midiInputBuffer[1]
                if (globalData.midiThru) {
                  Serial.write(0x80 | newChannel);  // Note Off + new channel
                  Serial.write(newNote);            // New note
                  Serial.write(lastData);           // Velocity (lastData stays the same)
                }
              } else {
                // Note Tracks - Modify only the Channel - quantizing ANY midi input channel to current track setup
                byte newChannel = patternData.trackMidiCH[curTrack] & 0x0F;
                midiInputActivityFlags &= ~(1 << curTrack);
                if (globalData.midiThru) {
                  Serial.write(0x80 | newChannel);   // Note Off + new channel
                  Serial.write(midiInputBuffer[1]);  // Original note
                  Serial.write(lastData);            // Velocity
                }
              }
            }
            break;
          }

        case 0x90:  // Note On
          {
            if (channel == globalData.midiInputToPatternChannel - 1) {  //midi note pattern changing
              if (lastData == 0) break;                                 // Ignore Note On with vel 0 (used as Note Off in running status)
              byte noteChange = midiInputBuffer[1];
              int noteOffset = (noteChange - 36) % 16;  // Offset from note 36, limited to 16 keys
              if (noteOffset < 0) noteOffset += 16;     // Ensure positive wrap-around if below range
              byte noteValue = noteOffset;              // Map to 1-16

              // Toggle between bank and pattern selection
              if (midiToBank) {
                nextPatternBank = noteValue;
              } else {
                nextPattern = noteValue;
              }

              midiToBank = !midiToBank;  // Toggle for next press

              // Only call loadPattern if there is a change
              if (nextPatternBank != currentPatternBank || nextPattern != currentPattern) {
                loadPattern(nextPattern);
              }
              break;
            }

            if (recordEnabled && seqPlaying && lastData > 0)  //do not record note on zero velocity as note off steps
            {
              if (noteP <= 127 && velocityP <= 127 && velocityP > 0) {
                addRecordNotes(noteP, velocityP, channel);
              }
            }

            lastIncomingMIDINote = midiInputBuffer[1];            // store the note for midi drum note learn in setup
            lastIncomingMIDIChannel = midiInputBuffer[0] & 0x0F;  // store the channel for midi drum note learn in setup

            //piggybacked midi drum note learn function
            if ((midiLearnActive && lastIncomingMIDINote >= 0) && (lastIncomingMIDIChannel == patternData.trackMidiCH[curTrack])) {
              patternData.trackNote[curTrack] = lastIncomingMIDINote;
              lastIncomingMIDINote = lastIncomingMIDIChannel = -1;  // reset for preventing double learn
              somethingChangedPattern = true;
              somethingHappened = true;
            }

            if (globalData.midiThru) {
              if (globalData.multitrackRec) {

                bool matchesTrack = false;  //only midi thru notes that are defined in setup

                for (byte t = 0; t < (DRUM_TRACKS + NOTE_TRACKS); t++) {
                  if (t < DRUM_TRACKS) {
                    if ((patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) && patternData.trackNote[t] == midiInputBuffer[1]) {
                      matchesTrack = true;
                      bitWrite(midiInputActivityFlags, t, lastData != 0);
                    }
                  } else {
                    if (patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) {
                      matchesTrack = true;
                      bitWrite(midiInputActivityFlags, t, lastData != 0);
                    }
                  }
                }
                if (matchesTrack) {
                  // Send MIDI thru
                  Serial.write(midiInputBuffer[0]);  //MSG type & Channel
                  Serial.write(midiInputBuffer[1]);  //Note number
                  Serial.write(lastData);            // Velocity
                }
              }

              else {  //single track rec
                if (curTrack < DRUM_TRACKS) {
                  // Drum Tracks - Modify Channel and Note  - quantizing ANY midi input channel & note to current track setup
                  byte newChannel = patternData.trackMidiCH[curTrack] & 0x0F;  // Mask lower 4 bits for the channel
                  byte newNote = patternData.trackNote[curTrack] & 0x7F;       // Mask the note to keep it within valid range

                  Serial.write(0x90 | newChannel);  // Note On + new channel
                  Serial.write(newNote);            // New note
                  Serial.write(lastData);           // Velocity (lastData stays the same)
                  bitWrite(midiInputActivityFlags, curTrack, lastData != 0);

                } else {
                  // Note Tracks - Modify only the Channel  - quantizing ANY midi input channel to current track setup
                  byte newChannel = patternData.trackMidiCH[curTrack] & 0x0F;

                  Serial.write(0x90 | newChannel);   // Note On + new channel
                  Serial.write(midiInputBuffer[1]);  // Original note
                  Serial.write(lastData);            // Velocity
                  bitWrite(midiInputActivityFlags, curTrack, lastData != 0);
                }
              }
            } else {
              if (globalData.multitrackRec) {  // no midi thru, only show leds
                for (byte t = 0; t < (DRUM_TRACKS + NOTE_TRACKS); t++) {
                  if (t < DRUM_TRACKS) {
                    if ((patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) && patternData.trackNote[t] == midiInputBuffer[1]) {
                      bitWrite(midiInputActivityFlags, t, lastData != 0);
                    }
                  } else {
                    if (patternData.trackMidiCH[t] == (midiInputBuffer[0] & 0x0F)) {
                      bitWrite(midiInputActivityFlags, t, lastData != 0);
                    }
                  }
                }
              } else {  // single track rec
                bitWrite(midiInputActivityFlags, curTrack, lastData != 0);
              }
            }
            break;
          }
      }
    }
  }
}

void addRecordNotes(byte data1, byte data2, byte channel) {
  if (recordBufferPosition < MIDI_INPUT_BUFFER) {
    recordBuffer[0][recordBufferPosition] = data1;
    recordBuffer[1][recordBufferPosition] = data2;
    recordBuffer[2][recordBufferPosition] = channel;
    recordBufferPosition++;
  }
}

void sendSysExPatternData() {
  if (seqPlaying) {
    ShowTemporaryMessage(kStopSequencer);  // Stop sequencer message if playing
    return;
  }
  ShowTemporaryMessage(kSysexDump);
  sysexActive = true;
  (midiClockInternal) = true;
  // Start SysEx Message
  Serial.write(0xF0);  // Start SysEx
  Serial.write(0x7D);  // Non-commercial Manufacturer ID
  Serial.write(0x45);  // Project Identifier "E"

  // Send Pattern Data (Config)
  uint8_t *ptr = (uint8_t *)&patternData;
  for (uint16_t i = 0; i < sizeof(WPATTERN); i++) {
    Serial.write(ptr[i] & 0x7F);         // Low 7 bits
    Serial.write((ptr[i] >> 7) & 0x01);  // MSB
  }

  // Send Step Data
  ptr = (uint8_t *)&stepsData;
  for (uint16_t i = 0; i < sizeof(WSTEPS) * STEPS; i++) {
    Serial.write(ptr[i] & 0x7F);         // Low 7 bits
    Serial.write((ptr[i] >> 7) & 0x01);  // MSB
  }

  // End SysEx Message
  Serial.write(0xF7);
  sysexActive = false;
}

void handleSYSEXInput() {
  // Only process if we're currently receiving SysEx data
  if (!receivingSysEx) {
    return;  // Exit if no SysEx message is being received
  }

  while (Serial.available() > 0) {
    syxinputByte = Serial.read();

    if (receivingSysEx) {
      // End of SysEx message
      if (syxinputByte == 0xF7) {
        midiInputStage = 0;
        receivingSysEx = false;
        sysexActive = false;
        byteIndex = 0;  // Reset for next SysEx message
        highNibble = false;
        pulseOut(false);
        tickOutCounter = 0;
        tickOutCounterLen = 0;
        tickOutPinState = false;

        currentPlaysPattern = currentPlaysChain = 0;
        recordBufferPosition = 0;

        memset(midiOutputBuffer, 0, sizeof(midiOutputBuffer));
        midiOutputBufferPosition = 0;
        memset((void *)noteLenCounters, 0, sizeof(noteLenCounters));

        seqPosition = PPQcounter = variation = 0;
        seqPlaying = false;
        stopTimer(false);
        if (!globalData.writeProtectFlash) {  //write pattern
          stopTimer(true);
          savePatternData(true);
          //saveStepsData();
          startTimer(true);
        }
        startTimer(false);
        calculateSequencer = 1;
        dumpReceived = true;
        return;
      }

      // Process incoming SysEx data (on-the-fly decoding)
      if (!highNibble) {
        tempByte = syxinputByte;  // Store low 7-bit part
        highNibble = true;
      } else {
        uint8_t fullByte = (syxinputByte << 7) | tempByte;  // Reconstruct full byte
        highNibble = false;

        // Directly store into patternData and stepsData as bytes arrive
        if (byteIndex < sizeof(WPATTERN)) {
          ((uint8_t *)&patternData)[byteIndex] = fullByte;
        } else {
          ((uint8_t *)&stepsData)[byteIndex - sizeof(WPATTERN)] = fullByte;
        }

        byteIndex++;  // Move to the next byte
      }
    }
  }
}

//END OF LINE