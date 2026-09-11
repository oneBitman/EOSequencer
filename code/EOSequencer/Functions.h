/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */

void reset();
void resetPatternBank();
void waitMs(int mstime);
void checkIfDataNeedsSaving();
void eraseSector(uint16_t _pagePos);
void savePatternData(byte _eraseSector);
void flashInit(bool force);
void saveHeader(bool sectorErase);
void initPatternBank(byte patternBank, bool sectorErase, int& porc, bool patternBankOnly, byte initMode);
void loadPattern(byte pattern, bool force = false);
void checkPatternStream();
void loadPatternBank(byte patternBank);
void initTM1638();
void sendData(byte data);
void sendDataConst(byte data1, byte data2, byte data3);
void sendScreen();
void readButtons(void);
void printNumber(byte segment, byte offset, int number);
void showErrorMsg(byte error);
void showErrorMsg(byte error, bool errors);
void showWaitMsg(char porcentage);
void printDashDash(byte segment, byte offset);
void printMIDInote(byte note, byte segment, byte offset, byte offsetOctave);
//void splashScreen();
void createScreen();
void checkInterface();
void checkIfMirrorAndCopy(byte thestep, byte track);
void clearStepsExtrasBits(byte thestep, byte xVar, byte track);
void resetSegments(byte xs, byte xe);
void getStepVelocity(byte theStep, byte track, byte variation, bool cleanBits);
void getNoteStepGlideDoubleOff(byte theStep, byte track, byte variation, bool cleanBits);
void eraseTrack();
void randomTrack();
void showMenuCopyPaste();
void processMenuCopyPaste(byte button);
void showMenu();
void processMenuFORMAT();
void processMenu(char value);
void showOnOrOff(bool showOn);
void showMemoryProtected();
void handleMIDIInput();
void createScreenMute();
void checkInterfaceMute();
void outputMIDIBuffer();
void doTickSequencer();
void ledsVarSeqUpdate();
void ledsBufferFlip();
void trackNoteOn(byte xtrack, byte xnote, byte xvelocity);
void noteTrackNoteOn(byte xtrack, byte xnote, byte xvelocity);  //, bool slide);
void stopDrumTrackPrevNote(byte track, bool isDrumTrack);
void startSequencer();
void startTimer(bool force);
void stopTimer(bool force);
void updateSequencerSpeed(bool force);
void setupTimerForExternalMIDISync(bool active);
void stopSequencer(void);
void sendMidiEvent(byte type, byte byte1, byte byte2, byte channel);
void startMIDIinterface();
void updatePorc(byte value);
void ShowTemporaryMessage(byte message);
char stepBeenHold();
void addRecordNotes(byte data1, byte data2, byte channel);
void recordInputCheck(byte data1, byte data2, byte channel, byte track);
void sendMIDIProgramChange(byte track);
void resetProgramChangeAndCC();
void pulseOut(bool enable);
void sendSysExPatternData();
void handleSYSEXInput();
char getBankLetter(byte value);
//END OF LINE