/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */
void reset() {
  resetSegments(0, 2);
  memset((void*)trackPosition, 0, sizeof(trackPosition));
  memset(midiOutputBuffer, 0, sizeof(midiOutputBuffer));
  midiOutputBufferPosition = 0;
  memset(leds, 0, sizeof(leds));
  memset(buttons, 0, sizeof(buttons));
  memset(buttonEvent, 0, sizeof(buttonEvent));
  memset((void*)buttonDownTime, 0, sizeof(buttonDownTime));
  memset(prevPlayedNote, 0, sizeof(prevPlayedNote));
  bitSet(patternBitsSelector, 0);
  memset(recordBuffer, 0, sizeof(recordBuffer));
  recordBufferPosition = 0;
  memset((void*)noteLenCounters, 0, sizeof(noteLenCounters));
}

void resetPatternBank() {
  for (byte x = 0; x < STEPS; x++) { stepsData[x].init(); }
}

void waitMs(int mstime) {
  for (int xx = 0; xx < mstime; xx++) {
    for (int xms = 0; xms < 1000; xms++) {
      __asm__("nop");  //burn some cycles
    }
  }
}