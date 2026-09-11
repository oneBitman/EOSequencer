/*

   Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com

*/
#include <util/atomic.h>
static inline uint8_t read_curTrack_volatile_once() {
  return *(volatile const uint8_t *)&curTrack;
}

#define PPQ_TICK_DOUBLE_NOTE 3
#define PPQ_TICK_END 6
#define PPQ 24

ISR(TIMER1_COMPA_vect) {
  //if (inISR) return;
  //inISR = true;

  if (midiClockInternal && sysexActive == false) {

    if (globalData.seqSyncOut && seqPlaying) {
      midiClockCounter--;
      if (midiClockCounter <= 0) {  //clock divider for internal clock
        Serial.write(0xF8);         // Send MIDI clock
        eventsSent++;
        midiClockCounter = globalData.midiClockDivide;
      }
    }

    outputMIDIBuffer();
    pulseOut(tickOutPinState);
    ledsBufferFlip();
    calculateSequencer++;
    doTickSequencer();

  } else {
    if (!midiClockInternal) handleMIDIInput();
  }
  //inISR = false;
}

void outputMIDIBuffer() {
  while (midiOutputBufferPosition > 0) {
    Serial.write(midiOutputBuffer[0][midiOutputBufferPosition - 1]);
    Serial.write(midiOutputBuffer[1][midiOutputBufferPosition - 1]);
    if (midiOutputBuffer[2][midiOutputBufferPosition - 1] != 0xFF) Serial.write(midiOutputBuffer[2][midiOutputBufferPosition - 1]);
    midiOutputBufferPosition--;
    eventsSent += 3;
  }
}


void doTickSequencer() {
  const uint8_t curTrack_s = read_curTrack_volatile_once();
  uint16_t muteTrack_s;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    muteTrack_s = muteTrack;
  }
  while (calculateSequencer > 0) {  //flag for executing the tick, 24 pulse per quarter note based
    if (seqPlaying) {
      // Track Mute Logic
      if (prevMuteTrack != muteTrack_s) {
        for (byte x = 0; x < (NOTE_TRACKS + DRUM_TRACKS); x++) {
          const bool nowMuted = bitRead(muteTrack_s, x);
          if (bitRead(prevMuteTrack, x) != nowMuted && nowMuted) {
            if (x >= DRUM_TRACKS) {
              stopDrumTrackPrevNote(x - DRUM_TRACKS, false);  // Adjust global index to local index (0–5 for note tracks)
            } else {
              stopDrumTrackPrevNote(x, true);  // No adjustment needed for drum tracks
            }
          }
        }
        prevMuteTrack = muteTrack_s;
      }
      // Determine if the current step is even or uneven
      isEvenStep = (trackPosition[longestTrackIndex] + 1) % 2 == 0;

      if (PPQcounter == 0) {
        if (!isEvenStep) {                                 //count ticks from uneven steps, 1 3 5 etc.
          shuffleCounter = patternData.shuffleDelay + 11;  //LDA shuffle delay, full step offset (6 tics), 32nd offset (3 tics), +1 tic since we decrease NOW in the same tick - to align PPQ 0 with shuffleCounter 9, +1 offset to have zero as a neutral value
          alignedToPlay16 = false;                         // Reset alignments
          alignedToPlay32 = false;
          alignedToDisplay16 = false;
        }
      }

      if (shuffleCounter > 0) {
        shuffleCounter--;  // Decrement every PPQ tick
      }

      if (shuffleCounter == 5) {
        alignedToDisplay16 = true;
      }

      if (shuffleCounter == 4) {
        alignedToPlay16 = true;
      }
      if (shuffleCounter == 1) {
        alignedToPlay32 = true;
      }

      if (((PPQcounter == 0 || PPQcounter == PPQ_TICK_DOUBLE_NOTE) && (!isEvenStep)) || (alignedToPlay16) || (alignedToPlay32)) {



        for (byte i = 0; i < 16; i++) {
          byte x = trackSequence[i];  // Get the current track from the sequence, optimized playback order of tracks
          // Drum Track Handling
          if (x < DRUM_TRACKS) {  // Drum tracks are indexed as 0–9
            bool trackIsMuted = bitRead(muteTrack_s, x) == 1;
            byte xvel = bitRead(stepsData[trackPosition[x]].steps[x], 1 + (variation * 2)) << 1 | bitRead(stepsData[trackPosition[x]].steps[x], 0 + (variation * 2));
            bool isDouble = bitRead(stepsData[trackPosition[x]].stepsDouble[variation], x);
            if (xvel > 0) {
              // now check the time conditions
              if (((PPQcounter == 0) && (!isEvenStep)) || ((PPQcounter == PPQ_TICK_DOUBLE_NOTE) && isDouble && (!isEvenStep)) || alignedToPlay16 || (alignedToPlay32 && isDouble)) {
                if (!trackIsMuted) {
                  // Playback Probability - step skip
                  byte randomChance;
                  if (patternData.trackProbability[x] != 0) {
                    // Only generate random if needed
                    randomChance = random(100);  // 0–99
                  } else {
                    // Always play
                    randomChance = 0;
                  }
                  // Check if the step should be played
                  if (randomChance >= patternData.trackProbability[x]) {
                    byte theVelocity = patternData.accentValues[xvel - 1];
                    trackNoteOn(x, patternData.trackNote[x], theVelocity);
                  }
                } else {
                  noteLenCountersLED[x] = patternData.drumNoteLen[x] + 1;
                }
              }
            }
          }
          // Note Track Handling
          else {

            // Note tracks are indexed as 10–15
            bool NtrackIsMuted = bitRead(muteTrack_s, x) == 1;
            byte Nxvel = bitRead(stepsData[trackPosition[x]].noteStepsExtras[x - DRUM_TRACKS][0], 1 + (variation * 2)) << 1 | bitRead(stepsData[trackPosition[x]].noteStepsExtras[x - DRUM_TRACKS][0], 0 + (variation * 2));
            byte extra = bitRead(stepsData[trackPosition[x]].noteStepsExtras[x - DRUM_TRACKS][1], 1 + (variation * 2)) << 1 | bitRead(stepsData[trackPosition[x]].noteStepsExtras[x - DRUM_TRACKS][1], 0 + (variation * 2));
            bool isSlide = (extra == 1);
            bool isNoteOff = (extra == 3);
            byte xnote = stepsData[trackPosition[x]].noteSteps[x - DRUM_TRACKS][variation];
            if ((xnote > 0 && PPQcounter == 0) || (xnote > 0 && PPQcounter == 0 && alignedToPlay16)) {
              if (isNoteOff) {
                noteLenCounters[x] = 1;
              }
            }
            if ((xnote > 0 && Nxvel > 0 && PPQcounter == 0) || (xnote > 0 && Nxvel > 0 && alignedToPlay16)) {
              if (!NtrackIsMuted) {
                // Playback Probability - step skip
                byte randomChance;

                if (patternData.trackProbability[x] != 0) {
                  // Only generate random if needed
                  randomChance = random(100);  // 0–99
                } else {
                  // Always play
                  randomChance = 0;
                }

                // Check if the step should be played
                if (randomChance >= patternData.trackProbability[x]) {
                  byte theVelocity = patternData.accentValues[Nxvel - 1];
                  noteTrackNoteOn(x, xnote, theVelocity, isSlide);
                }

              } else {
                noteLenCountersLED[x] = patternData.drumNoteLen[x] + 1;
              }
            }
          }
        }
        alignedToPlay32 = false;
      }

      // Recording Check

      if (recordEnabled && PPQcounter == 4) {

        if (!globalData.multitrackRec) {  //only current track
          for (byte xm = 0; xm < recordBufferPosition; xm++) {
            recordInputCheck(recordBuffer[0][xm], recordBuffer[1][xm], recordBuffer[2][xm], curTrack_s);
          }
          recordBufferPosition = 0;
        } else  //alltracks
        {
          // --- Array for sorted note track candidates ---
          struct NoteEvent {
            byte note;
            byte velocity;
            byte channel;
          };

          NoteEvent noteTrackCandidates[6];  // max 6 poly notes
          byte noteTrackCount = 0;

          for (byte xm = 0; xm < recordBufferPosition; xm++) {
            byte incomingNote = recordBuffer[0][xm];
            byte incomingVelocity = recordBuffer[1][xm];
            byte incomingChannel = recordBuffer[2][xm];

            // --- Check for DRUM TRACKS first ---
            bool isDrum = false;
            for (byte t = 0; t < DRUM_TRACKS; t++) {
              if (patternData.trackMidiCH[t] == incomingChannel && patternData.trackNote[t] == incomingNote) {
                recordInputCheck(incomingNote, incomingVelocity, incomingChannel, t);
                isDrum = true;
                break;
              }
            }

            // --- If not a drum note, store for note track processing ---
            if (!isDrum && noteTrackCount < 6) {
              noteTrackCandidates[noteTrackCount++] = { incomingNote, incomingVelocity, incomingChannel };
            }
          }

          // --- Sort only the note track candidates by pitch (ascending) ---
          for (byte i = 1; i < noteTrackCount; i++) {
            NoteEvent key = noteTrackCandidates[i];
            int j = i - 1;
            while (j >= 0 && noteTrackCandidates[j].note > key.note) {
              noteTrackCandidates[j + 1] = noteTrackCandidates[j];
              j--;
            }
            noteTrackCandidates[j + 1] = key;
          }

          // --- Now assign sorted note candidates to matching note tracks ---
          for (byte i = 0; i < noteTrackCount; i++) {
            byte incomingNote = noteTrackCandidates[i].note;
            byte incomingVelocity = noteTrackCandidates[i].velocity;
            byte incomingChannel = noteTrackCandidates[i].channel;

            // Build list of matching note tracks
            byte matchingTracks[NOTE_TRACKS];
            byte matchingTrackCount = 0;

            for (byte t = DRUM_TRACKS; t < (DRUM_TRACKS + NOTE_TRACKS); t++) {
              if (patternData.trackMidiCH[t] == incomingChannel) {
                matchingTracks[matchingTrackCount++] = t;
              }
            }

            if (matchingTrackCount == 0) continue;

            // Pitch-based assignment: lowest note → lowest track
            if (i < matchingTrackCount) {
              byte selectedTrack = matchingTracks[i];
              recordInputCheck(incomingNote, incomingVelocity, incomingChannel, selectedTrack);
            }
          }

          recordBufferPosition = 0;
        }
      }

      //pocket operator/volca pulse output
      if (globalData.tickOut > 0 && PPQcounter == 0) {
        if (tickOutCounter == globalData.tickOut) {
          tickOutCounterLen = (globalData.tickOutLen + 1);
          tickOutPinState = true;
        }
        tickOutCounter--;
        if (tickOutCounter == 0) {
          tickOutCounter = globalData.tickOut;
        }
      }

      if (tickOutCounterLen > 0) {  //every tick is counted
        tickOutCounterLen--;
        if (tickOutCounterLen == 0) tickOutPinState = false;
      }
      // note length counter
      for (byte i = 0; i < 16; i++) {
        byte x = trackSequence[i];  // Process tracks in the correct priority order
        if (noteLenCounters[x] > 0) {
          if (--noteLenCounters[x] == 0) {
            // Only send Note Off for the corresponding note
            if (x < DRUM_TRACKS) {
              sendMidiEvent(midiNoteOff, patternData.trackNote[x], 0, patternData.trackMidiCH[x]);
            } else if (prevPlayedNote[x - DRUM_TRACKS] > 0) {
              sendMidiEvent(midiNoteOff, prevPlayedNote[x - DRUM_TRACKS], 0, patternData.trackMidiCH[x]);
              prevPlayedNote[x - DRUM_TRACKS] = 0;  // Reset the previous note
            }
          }
        }


        if (noteLenCountersLED[x] > 0) {
          if (--noteLenCountersLED[x] == 0) {
          }
        }
      }
    }

    allMidiActivityFlags = 0;

    // Build final activity flags for all tracks - merge midi input and playback data to display on segment dots
    for (byte t = 0; t < 16; t++) {
      if ((midiInputActivityFlags & (1 << t)) || (noteLenCounters[t] > 0)) {
        bitSet(allMidiActivityFlags, t);
      }
    }

    // Pattern Stream Handling
    if (PPQcounter == PPQ_TICK_DOUBLE_NOTE) {
      longestTRACK = 0;
      longestTrackIndex = 0;

      for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {


        if (patternData.trackLen[x] > longestTRACK) {
          // Update longest track
          longestTRACK = patternData.trackLen[x];
          longestTrackIndex = x;
        }
      }

      if (seqPlaying) {

        // trigger pattern switching based longest track's position
        if ((trackPosition[longestTrackIndex] + 1) >= patternData.trackLen[longestTrackIndex] && (variation + 1) >= prevPatternTotalVariations) {
          //last 32nd step before wrapping, +3 tics from here and we are at back at first PPQ tick at first step
          //once per polyMetric or once per 16/32/48/64 steps A/B/C/D wrap

          if (streamNextPattern) {
            //next pattern is chained, in 3 tics we do the switch

            streamNextPattern = false;
            loadPatternNow = true;
            currentPlaysPattern = currentPlaysChain = 0;
            for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {

              if (patternData.trackLen[x] != 16) {  //set all steps to their last position if we were in polyMetric mode
                trackPosition[x] = 15;

              } else doAlign = true;  //set flag to ensure first step alignment
            }
          }


          // Handle Playlist/Chaining
          else {  //we are now still at the end of the pattern, 3PPQ before wrap, now to check which pattern is played next
            if (repeatMode == kRepeatModePattern) currentPlaysPattern = 0;
            if (patternData.playsPattern != 0 && (repeatMode == kRepeatModeNormal || repeatMode == kRepeatModeChain)) {  //if repeat one pattern with counter
              currentPlaysPattern++;
              if ((currentPlaysPattern - 1) == (patternData.playsPattern - 1)) {
                currentPlaysPattern = 0;
                //
                if (patternData.playsChain != 1) {  //if pattern chain mode true
                  currentPlaysChain++;
                  if (patternData.playsChain == 0 || repeatMode == kRepeatModeChain) currentPlaysChain = 0;  //play current chain infinite
                  if (patternData.playsChain != 0 && currentPlaysChain >= (patternData.playsChain - 1)) {    //current chain is finished
                    currentPlaysChain = 0;
                    nextPattern = currentPattern + 1;  //exit chain to next pattern +1
                    if (nextPattern >= 16) {
                      nextPattern = 0;
                      nextPatternBank = (currentPatternBank + 1) % PT_BANKS;
                    }
                    loadPatternNow = true;
                    for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {  //logic for aligning tracks when pattern was polymetric and next pattern is not

                      if (patternData.trackLen[x] != 16) {  //set all steps to their last position if we were in polyMetric mode
                        trackPosition[x] = 15;

                      } else doAlign = true;  //set flag to ensure first step alignment
                    }
                  } else {  //chain not finsihed, next pattern
                    if (patternData.nextPattern == 0) {
                      nextPattern = currentPattern + 1;
                      if (nextPattern >= 16) {
                        nextPattern = 0;
                        nextPatternBank = (currentPatternBank + 1) % PT_BANKS;
                      }
                    } else {
                      nextPattern = ((patternData.nextPattern - 1) % 16);
                      nextPatternBank = (patternData.nextPattern - 1) / 16;
                    }
                    loadPatternNow = true;

                    for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {

                      if (patternData.trackLen[x] != 16) {  //set all steps to their last position if we were in polyMetric mode
                        trackPosition[x] = 15;

                      } else doAlign = true;  //set flag to ensure first step alignment
                    }
                  }
                } else {  //not chain mode
                  if (patternData.nextPattern == 0) {
                    nextPattern = currentPattern + 1;
                    if (nextPattern >= 16) {
                      nextPattern = 0;
                      nextPatternBank = (currentPatternBank + 1) % PT_BANKS;
                    }
                  } else {
                    nextPattern = ((patternData.nextPattern - 1) % 16);  //go to specific pattern as defined in setup
                    nextPatternBank = (patternData.nextPattern - 1) / 16;
                  }
                  loadPatternNow = true;
                  for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {

                    if (patternData.trackLen[x] != 16) {  //set all steps to their last position if we were in polyMetric mode
                      trackPosition[x] = 15;

                    } else doAlign = true;  //set flag to ensure first step alignment
                  }
                }
              }
            }
          }
        }

        //visual feedback for last pattern or chain repeat
        if (
          repeatMode == kRepeatModeNormal) {

          // PATTERN REPEAT active only if no chain
          if (patternData.playsPattern > 0) {  // && patternData.playsChain <= 1) {
            isLastPatternRepeat = (patternData.playsPattern - currentPlaysPattern == 1);
          } else {
            isLastPatternRepeat = false;
          }

          // CHAIN REPEAT active only if chaining is active
          if (patternData.playsChain > 1) {
            isLastChainRepeat = ((patternData.playsChain - 1) - currentPlaysChain == 1);
          } else {
            isLastChainRepeat = false;
          }

        } else {
          // Reset both flags if not in normal mode
          isLastPatternRepeat = false;
          isLastChainRepeat = false;
        }
      }
    }

    PPQcounter++;
    if (PPQcounter >= PPQ_TICK_END) {
      PPQcounter = 0;

      bool disableVariations = false;
      for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {
        // Align tracks when coming from a full 16 step pattern
        if (doAlign == true) {
          trackPosition[x] = (patternData.trackLen[x] - 1);
        }
        // Increment and wrap position based on track length
        trackPosition[x] = (trackPosition[x] + 1) % patternData.trackLen[x];

        // Polymetric mode - Disable variations if any track is shorter than 16 steps
        if (patternData.trackLen[x] < 16) {
          disableVariations = true;
        }
      }

      if (disableVariations) {
        prevPatternTotalVariations = patternData.totalVariations = 1;
        if (variation != 0 || forceVariation > 0) {
          variation = 0;
          forceVariation = -1;
          mirror = false;
        }
      }

      doAlign = false;
      seqPosition = trackPosition[longestTrackIndex];

      if (trackPosition[longestTrackIndex] == 0 && seqPlaying == true) {
        //we are at position 0 now
        //wrapping of longest track if polyMetric, else wrapping of 16step pattern

        variation = (variation + 1) % prevPatternTotalVariations;

        prevPatternTotalVariations = patternData.totalVariations;
      }
    }

    // Reset LED buffers
    chaseLEDsBuffer[0] = 0;
    chaseLEDsBuffer[1] = 0;
    // Track LED update only for the currently selected track

    isEvenStep = (trackPosition[longestTrackIndex] + 1) % 2 == 0;
    if ((PPQcounter == 0 && !isEvenStep) || alignedToDisplay16) {

      ledPosition = trackPosition[curTrack_s];  // Current step for the selected track
    }
    if (ledPosition < patternData.trackLen[curTrack_s]) {
      if (ledPosition < 8) {
        bitSet(chaseLEDsBuffer[0], ledPosition);
      } else {
        bitSet(chaseLEDsBuffer[1], ledPosition - 8);
      }
    }
    alignedToPlay16 = false;
    alignedToDisplay16 = false;
    // Update LEDs on screen
    ledsVarSeqUpdate();
    screenUpdate = true;  //flag for updating screen in main void
    calculateSequencer = 0;
  }
}

void recordInputCheck(byte data1, byte data2, byte channel, byte track) {

  byte xvariation = variation;
  byte theStep = trackPosition[track];

  byte xVar = xvariation;
  if (forceVariation >= 0) xVar = forceVariation;
  if (mirror) xVar = 0;

  if (data2 == 0) return;  //cancel the check in case of zero velocity

  if (track < DRUM_TRACKS)  //LOGIC FOR RECORDING DRUM TRACKS
  {
    if (
      (!globalData.multitrackRec) || (globalData.multitrackRec && patternData.trackMidiCH[track] == channel && patternData.trackNote[track] == data1)) {

      bitClear(stepsData[theStep].steps[track], (xVar * 2));
      bitClear(stepsData[theStep].steps[track], (xVar * 2) + 1);

      // Quantize velocity for drum tracks
      lastVelocityMIDI = 1;                                                 // Default
      if (data2 >= patternData.accentValues[2]) lastVelocityMIDI = 3;       // Strong accent
      else if (data2 >= patternData.accentValues[1]) lastVelocityMIDI = 2;  // Mild accent

      // Apply quantized velocity to the drum step
      somethingChangedPattern = true;
      stepsData[theStep].steps[track] |= lastVelocityMIDI << (xVar * 2);

      // Apply the mirror logic across all variations
      if (mirror) {
        // Clear the step for all variations (set it to 0x00)
        stepsData[theStep].steps[track] = 0x00;

        // Apply mirrored velocity (stepVelocity can be 1, 2, or 3)
        stepsData[theStep].steps[track] |= lastVelocityMIDI;       // Set first position
        stepsData[theStep].steps[track] |= lastVelocityMIDI << 2;  // Set second position
        stepsData[theStep].steps[track] |= lastVelocityMIDI << 4;  // Set third position
        stepsData[theStep].steps[track] |= lastVelocityMIDI << 6;  // Set fourth position
      }
    }

  } else  //LOGIC FOR RECORDING NOTE TRACKS
  {
    if (
      (!globalData.multitrackRec) || (globalData.multitrackRec && patternData.trackMidiCH[track] == channel)) {

      patternData.lastNote[track - DRUM_TRACKS] = data1;

      lastVelocityMIDI = 1;
      if (data2 >= patternData.accentValues[2]) lastVelocityMIDI = 3;
      else if (data2 >= patternData.accentValues[1]) lastVelocityMIDI = 2;

      stepsData[theStep].noteSteps[track - DRUM_TRACKS][xVar] = patternData.lastNote[track - DRUM_TRACKS];
      // Clear the specific bits before setting the velocity for the given variation
      stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0] &= ~(0b11 << (xVar * 2));           // Clear two bits at the target position
      stepsData[theStep].noteStepsExtras[track - DRUM_TRACKS][0] |= lastVelocityMIDI << (xVar * 2);  // Set new velocity bits

      checkIfMirrorAndCopy(theStep, track - DRUM_TRACKS);
      somethingChangedPattern = true;
    }
  }
}

void ledsVarSeqUpdate() {
  leftLEDsVarSeqBuffer = 0;
  bitSet(leftLEDsVarSeqBuffer, (seqPosition / 4));
  byte xVar = variation;
  if (forceVariation >= 0) xVar = forceVariation;
  if (mirror) leftLEDsVarSeqBuffer |= B11110000;
  else {
    if (forceVariation >= 0) {
      leftLEDsVarSeqBuffer |= B11110000;
      bitClear(leftLEDsVarSeqBuffer, xVar + 4);
    } else bitSet(leftLEDsVarSeqBuffer, xVar + 4);
  }
}

void ledsBufferFlip() {
  leftLEDsVarSeq = leftLEDsVarSeqBuffer;
  chaseLEDs[0] = chaseLEDsBuffer[0];
  chaseLEDs[1] = chaseLEDsBuffer[1];
}

void trackNoteOn(byte xtrack, byte xnote, byte xvelocity)  //drum track
{
  sendMidiEvent(midiNoteOn, xnote, xvelocity, patternData.trackMidiCH[xtrack]);
  if (noteLenCounters[xtrack] > 0) {
    sendMidiEvent(midiNoteOff, xnote, 0, patternData.trackMidiCH[xtrack]);
    noteLenCounters[xtrack] = 0;
  }

  noteLenCounters[xtrack] = patternData.drumNoteLen[xtrack] + 1;
}

void noteTrackNoteOn(byte xtrack, byte xnote, byte xvelocity, bool slide)  //note track
{

  // Handle previous note logic
  if ((prevPlayedNote[xtrack - DRUM_TRACKS] > 0) || prevPlayedNote[xtrack - DRUM_TRACKS] == xnote) {
    if (noteLenCounters[xtrack] > 130) {
      // Glide Mode: Play new note while holding previous note
      sendMidiEvent(midiNoteOff, prevPlayedNote[xtrack - DRUM_TRACKS], 0, patternData.trackMidiCH[xtrack]);  // Add Old Note Off
    }
  }
  sendMidiEvent(midiNoteOn, xnote, xvelocity, patternData.trackMidiCH[xtrack]);  // Add New Note On
  if ((prevPlayedNote[xtrack - DRUM_TRACKS] > 0) || prevPlayedNote[xtrack - DRUM_TRACKS] == xnote) {
    if (noteLenCounters[xtrack] < 130) {

      sendMidiEvent(midiNoteOff, prevPlayedNote[xtrack - DRUM_TRACKS], 0, patternData.trackMidiCH[xtrack]);  // Add Old Note Off
    }
  }

  // Update previous note to the current note
  prevPlayedNote[xtrack - DRUM_TRACKS] = xnote;
  // Set note length based on slide mode
  if (slide) {
    noteLenCounters[xtrack] = 255;  // Max length for glide
  } else {
    noteLenCounters[xtrack] = patternData.drumNoteLen[xtrack] + 1;  // Standard length
  }
}

void stopDrumTrackPrevNote(byte track, bool isDrumTrack) {
  if (isDrumTrack) {
    sendMidiEvent(midiNoteOff, patternData.trackNote[track], 0, patternData.trackMidiCH[track]);
  } else {
    if (prevPlayedNote[track] > 0) {
      sendMidiEvent(midiNoteOff, prevPlayedNote[track], 0, patternData.trackMidiCH[DRUM_TRACKS + track]);
      prevPlayedNote[track] = 0;
    }
  }
}

void startSequencer() {
  stopTimer(false);
  seqPlaying = true;
  seqPosition = PPQcounter = variation = 0;
  memset((void *)trackPosition, 0, sizeof(trackPosition));
  currentPlaysPattern = currentPlaysChain = 0;
  recordBufferPosition = 0;
  memset(midiOutputBuffer, 0, sizeof(midiOutputBuffer));
  midiOutputBufferPosition = 0;
  memset((void *)noteLenCounters, 0, sizeof(noteLenCounters));
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    prevMuteTrack = muteTrack;
  }

  if (globalData.writeProtectFlash) ShowTemporaryMessage(kMemoryProtectMessage);

  tickOutCounter = globalData.tickOut;
  prevPatternTotalVariations = patternData.totalVariations;

  eventsSent = eventsPrevious = 0;

  ledsVarSeqUpdate();

  chaseLEDs[0] = chaseLEDsBuffer[0] = 0;
  chaseLEDs[1] = chaseLEDsBuffer[1] = 0;

  ledPosition = 0;
  alignedToDisplay16 = true;

  if (globalData.seqSyncOut) {
    Serial.write(0xFA);  // MIDI Start
  }

  delay(1);  // Wait 1 millisecond between FA and F8 as per midi spec
  calculateSequencer = 1; //prepare PPQ0 data so it can be sent immediate
  doTickSequencer();
  resetProgramChangeAndCC();

  if (midiClockInternal) {  //send PPQ0 clock immediately not to wait for timer
    if (globalData.seqSyncOut && seqPlaying) {
      Serial.write(0xF8);  // Send MIDI clock
      midiClockCounter = globalData.midiClockDivide;
    }

    outputMIDIBuffer();  //send PPQ0 data
    pulseOut(tickOutPinState);
    ledsBufferFlip();
    calculateSequencer++;
    doTickSequencer();  //prepare PPQ1
  }

  startTimer(true);  //timer started, ISR takes over MIDI buffer and tick sequencer
  leftLEDsVarSeq = leftLEDsVarSeqBuffer;
  midiPanic = true;  //enable once to execute midi panic only once when sequencer is stopped
}

void startTimer(bool force) {
  if (midiClockInternal || force) {
    TCCR1A = TCCR1B = 0;
    TCNT1 = 0;  // Reset counter

    // Set prescaler depending on clock mode
    if (midiClockInternal) {
      bitWrite(TCCR1B, CS11, 1);  // Prescaler = 8
    } else {
      bitWrite(TCCR1B, CS10, 1);  // Prescaler = 1
    }

    bitWrite(TCCR1B, WGM12, 1);  // CTC Mode
    updateSequencerSpeed(false);
    bitWrite(TIMSK1, OCIE1A, 1);  // Enable compare interrupt
  }
}

//     Mode	    Prescaler	OCR1A calculation	    Timer speed	        Purpose
//     Internal	8	        Based on BPM & PPQ	  Sequencer timing	  Master Clock
//     External	1	        2560                  MIDI byte polling 	Slave Sync

void stopTimer(bool force) {
  if (midiClockInternal || force) {
    bitWrite(TIMSK1, OCIE1A, 0);
    TCCR1A = TCCR1B = OCR1A = 0;
  }
}

void updateSequencerSpeed(bool force) {
  // Calculates the Frequency for the Timer, used by the PPQ clock (Pulses Per Quarter Note) //
  // This uses the 16-bit Timer1, unused by the Arduino, unless you use the analogWrite or Tone functions //

  if (midiClockInternal || force) OCR1A = (F_CPU / 8) / ((((realBPM) * (PPQ)) / 60)) - 1;
}

void setupTimerForExternalMIDISync(bool active) {

  if (active) {
    stopTimer(true);
    OCR1A = EXTERNAL_CLOCK_TIMER;

  } else {
    stopTimer(true);
    updateSequencerSpeed(true);
    startTimer(true);
  }
}

void stopSequencer(void) {
  recordEnabled = false;
  seqPlaying = false;
  stopTimer(false);

  if (midiPanic) {
    outputMIDIBuffer();
    Serial.flush();
  }

  delay(40);
  checkIfDataNeedsSaving();
  delay(40);

  if (midiPanic) {

    if (midiClockInternal && globalData.seqSyncOut) Serial.write(0xFC);  // MIDI Stop

    for (byte i = 0; i < 16; i++) {
      byte x = trackSequence[i];  // Process tracks in the correct priority order
      if (noteLenCounters[x] > 0) {

        // Only send Note Off for the corresponding note
        if (x < DRUM_TRACKS) {
          sendMidiEvent(midiNoteOff, patternData.trackNote[x], 0, patternData.trackMidiCH[x]);
        } else if (prevPlayedNote[x - DRUM_TRACKS] > 0) {
          sendMidiEvent(midiNoteOff, prevPlayedNote[x - DRUM_TRACKS], 0, patternData.trackMidiCH[x]);
          prevPlayedNote[x - DRUM_TRACKS] = 0;  // Reset the previous note
        }
        noteLenCounters[x] = 0;  // Reset counter either way
      }
    }
    Serial.flush();
    delay(20);
    isLastPatternRepeat = false;
    isLastChainRepeat = false;
    variation = 0;
    midiInputActivityFlags = 0;  //reset all midi input led flags
  }

  if (streamNextPattern) {
    streamNextPattern = false;
    loadPatternNow = true;
  }

  startTimer(false);
  calculateSequencer = 1;
  midiPanic = false;
}

void sendMidiEvent(byte type, byte byte1, byte byte2, byte channel) {

  midiOutputBuffer[0][midiOutputBufferPosition] = type | (channel & 0x0F);  // Mask channel to 4 bits
  midiOutputBuffer[1][midiOutputBufferPosition] = byte1;
  midiOutputBuffer[2][midiOutputBufferPosition] = byte2;
  midiOutputBufferPosition++;
}

void startMIDIinterface() {
  Serial.begin(31250);  // 31250 MIDI Interface //
}

void sendMIDIProgramChange(byte track) {
  if (patternData.programChange[track] > 0) sendMidiEvent(midiProgramChange, patternData.programChange[track] - 1, 0xFF, patternData.trackMidiCH[track]);
}

void resetProgramChangeAndCC() {

  for (byte x = 0; x < (DRUM_TRACKS + NOTE_TRACKS); x++) {
    sendMIDIProgramChange(x);
  }
}

void pulseOut(bool enable) {
  if (enable) PORTC = 0xFF;
  else PORTC = 0x00;
}

//END OF LINE