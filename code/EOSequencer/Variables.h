/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */

#define DEFAULT_NOTE 36     // Default note when adding steps to a new pattern, else the last used note number is carried on.
#define DEFAULT_MIDI_CH 10  // MIDI Default Channel for all 10 drum tracks - track 1 - 10
#define BUFFER_SIZE 64      // UART software buffer size, can go as low as 64
#define SERIAL_RX_BUFFER_SIZE 64
#define MIDI_OVER_USB 0  // When set will use 38400 bauds for the Serial interface - EOSequencer will not be recognized as a midi device, needs special COM drivers to use it via USB with a DAW.

#define DRUM_TRACKS 10  // This can't go above 16 - not recommended to change
#define NOTE_TRACKS 6   // This can't go above 8 - not recommended to change
#define PATTERNS 16     // This can't go above 64 - not recommended to change
#define PT_BANKS 16     // # of Pattern Banks - not recommended to change
#define STEPS 16        // This can't go above 16 - not recommended to change

#define TEMPORARY_MESSAGE_TIME 100           //Time for error messages and playback mode splashscreen
#define EXTERNAL_CLOCK_TIMER (F_CPU / 2560)  // Since a midi byte is 10 bits, 2560 is the 100% correct divider
#define MIDI_INPUT_BUFFER (18)               // (4 * 2) - The size of the internal buffer used for recording, do not change
#define FLASH_CHIPSIZE MB64
#define FLASH_VERSION '2'

enum {
  kButtonNone = 0,
  kButtonClicked,
  kButtonHold,
  kButtonRelease,
  kButtonReleaseNothingClicked,
  kRightSteps = 0,
  kRightTrackSelection,
  kRightPatternSelection,
  kRightMenuCopyPaste,
  kRightMenu,
  kMuteMenu,
  midiNoteOn = 0x90,
  midiNoteOff = 0x80,
  midiProgramChange = 0xC0,
  midiChannels = 16,

  menuFirst = 0,
  menuPtPlays = 0,
  menuPtNext,
  menuPtPlaysChain,
  menuMIDIChannel,
  menuNote,
  menuNoteLen,
  menuAccent1,
  menuAccent2,
  menuAccent3,
  menuProgramChange,
  menuVariationsABCD,
  menuTrackLen,
  menuShuffle,
  menuTrackProbability,
  menuClockType,
  menuSyncOut,
  menuClockDivider,
  menuMidiThru,
  menuMultitrack,
  menuPulseOut,
  menuPulseOutLen,
  menuMIDIinPattern,
  menuProtect,
  menuInit,
  menuBrightness,
  lastMenu = menuBrightness,

  kLeftMain = 0,
  kRandom,
  kErase,
  kMemoryProtectMessage = 0,
  kPatternRepeatMessage,
  kRepeatModeNormal = 0,
  kRepeatModeChain,
  kRepeatModePattern,
  kSysexDump,
  kStopSequencer
};

SPIFlash flash;
byte segments[3][8];
byte leds[3];
byte buttons[3];  // raw button values
byte buttonEvent[3][8];
byte buttonEventWasHolding[3] = { 0, 0, 0 };
byte buttonIsHolding[8];
volatile byte buttonDownTime[3][8];  // time of button press
volatile byte variation = 0;         // ABCD Variations
volatile char forceVariation = -1;
byte curTrack = 0;
byte curLeftScreen = 0;   //main interface
byte curRightScreen = 0;  //step interface
byte currentPattern = 0;
byte nextPattern = 0;
byte currentPatternBank = 0;
byte nextPatternBank = 0;
uint16_t patternBitsSelector;
volatile bool mirror = false;
bool somethingClicked = false;
bool somethingHappened = false;  // a button press was registered
bool forceAccent = false;
volatile bool seqPlaying = false;
volatile byte seqPosition = 0;
volatile byte PPQcounter = 0;
bool somethingChangedPattern = false;  // if true saves the pattern (steps, double steps variations...)
bool somethingChangedGlobal = false;   // if true saves global parameters (clock int/ext, sync out, write protect...)
volatile bool streamNextPattern = false;
volatile bool loadPatternNow = false;
byte ignoreButtons = false;
byte lastVelocity = B00000011;
byte lastVelocityMIDI = B00000011;
bool editingNote = false;
byte editStep = 0;
byte editVariation = 0;
byte prevPlayedNote[NOTE_TRACKS];  //
byte menuPosition = 0;
byte initMode = 0;
char flashHeader[8];
uint16_t prevMuteTrack = 0;
byte totalFlashErrors = 0;
byte prevPatternTotalVariations = 4;
volatile byte midiInputStage = 0;
volatile byte midiInputBuffer[2] = { 0, 0 };  // buffer for analyzing current midi input
byte midiOutputBuffer[3][48];                 //144 bytes - 48 3byte midi events
volatile byte midiOutputBufferPosition = 0;
volatile byte calculateSequencer = 0;
uint16_t patternPagePos = 0;
uint16_t pagePos = 0;
char editingNoteTranspose = -127;
bool noteTransposeWasChanged = false;
bool noteTransposeEditAllSteps = false;
byte leftLEDsVarSeq = 0;
byte leftLEDsVarSeqBuffer = 0;
uint16_t chaseLEDs[2] = { 0, 0 };
uint16_t chaseLEDsBuffer[2] = { 0, 0 };
byte stepVelocity = 0;
byte drumStepLastVelocity = B00000011;
byte noteStepGlideDoubleOff = 0;
bool ignoreNextButton = false;
volatile byte currentPlaysPattern = 0;
volatile byte currentPlaysChain = 0;
byte repeatMode = 0;
byte showTemporaryMessage = 0;
byte temporaryMessageCounter = 0;
byte xm = 0;
byte inputByte = 0;
volatile byte noteLenCounters[DRUM_TRACKS + NOTE_TRACKS];
volatile byte noteLenCountersLED[DRUM_TRACKS + NOTE_TRACKS];
volatile byte tickOutCounter = 0;
volatile byte tickOutCounterLen = 0;
volatile bool tickOutPinState = false;
bool recordEnabled = false;
byte recordBuffer[3][MIDI_INPUT_BUFFER];  //= one full midi event like note on
volatile byte recordBufferPosition = 0;
bool isSelectingBank = false;
byte preventABCD = 0;  //flag for preventing ABCD mode change when button [0][7] is pushed
uint16_t realBPM = 120;
uint8_t quantizedValue = 0;  // Global quantized timer value (0-15) for char animation counter
byte trackPosition[DRUM_TRACKS + NOTE_TRACKS] = {};
const byte trackSequence[16] = { 9, 8, 7, 6, 15, 5, 14, 4, 13, 3, 12, 2, 11, 1, 10, 0 };  // playback priority of tracks, reverse order! 0 is drum track 1, 10 is note track 1
volatile byte longestTRACK = 0;                                                           // amount of steps in longest track, polymetrics
volatile byte longestTrackIndex = 0;                                                      //for polymetrics, to determine when to switch to next pattern, index is pointing to patternData.trackLength
bool doAlign = false;                                                                     //alignment of polymetric tracks
volatile uint16_t muteTrack = 0x00;                                                       //all channels unmuted;
volatile byte shuffleCounter = 0;                                                         //each even 16th and 32nd step is generated by shufflecounter and value in setup
volatile bool alignedToPlay16 = false;                                                    //shuffled or straight playback flag for 16th step
volatile bool alignedToPlay32 = false;                                                    //same for 32nd step
volatile bool isEvenStep = false;                                                         //flag to determine even steps when to start shuffle counter
volatile byte ledPosition = 0;                                                            //position memory to show shuffle correctly on playback leds
volatile byte midiClockCounter = 0;                                                       // Counter for midiclock divide down division
volatile bool midiToBank = true;                                                          // Flag to interpret first midi note on external pattern switching control as bank number (second as patttern)
volatile bool sysexActive = false;
volatile bool receivingSysEx = false;
volatile uint16_t byteIndex = 0;
volatile uint8_t tempByte = 0;
volatile bool highNibble = false;
volatile uint8_t syxinputByte;
volatile bool startup = true;
volatile bool midiPanic = false;
const byte pwmBrightness[] = {
  0b10001000,  // 1/16 brightness
  0b10001001,  // 2/16 brightness
  0b10001010,  // 4/16 brightness
  0b10001011,  // 10/16 brightness
               // 0b10001100,  // 11/16 brightness
               // 0b10001101   // 12/16 brightness
};

uint16_t midiInputActivityFlags = 0;  // 16 bit value for displaying midi IN activity
uint16_t allMidiActivityFlags = 0;    // 16 bit value for displaying ALL midi activity
volatile bool midiLearnActive = false;         //flag for learning midi drum notes in setup
volatile bool trackChanged = true;             //flag for debouncing midi learn
int8_t lastIncomingMIDINote = -1;     // holds latest Note On note
int8_t lastIncomingMIDIChannel = -1;  // holds latest Note On channel
volatile bool leButtonHeld = 0;                //for detecting button hold in trackselector/midi learn
volatile bool screenUpdate = false;            //for syncing screen updates with PPQ tick
volatile bool midiClockInternal = true;
unsigned long previousFrameTime = 0;          //for interface and buttons refresh
unsigned long accumulatedInterfaceTime = 12;  //for interface and buttons refresh
volatile bool isLastPatternRepeat = false;    //flag for indicating last pattern repeat visual
volatile bool isLastChainRepeat = false;      //flag for indicating last pattern chain repeat visual
volatile byte lastStatusByte = 0;
volatile bool allowRunningStatus = false;
volatile bool dumpReceived = false;
volatile uint16_t eventsSent = 0;
uint16_t eventsPrevious = 0;
unsigned long accumulatedInterfaceTimeAlltime = 0;
byte visualPattern = 0;
volatile bool patCopy = true;
byte clipboardSlot = 0;                      // Current write slot for pattern clipboard
const byte CLIPBOARD_SLOTS = 8;              // Number of wear-leveling slots
const uint32_t CLIPBOARD_BASE_ADDR = 32480;
volatile bool alignedToDisplay16 = false;
volatile byte masterOffset = 0;

#pragma pack(push, 1)
//pragma pack is needed to ensure all variables take exact placement in the byte space without unnecessary padding,
//so array based syx dumps can be read nicer and also be dumped back and be saved to flash memory.

// Structuring of STEP data in arrays:
//
//Drum Tracks:
//Compressed format: Each step uses 2 bits for velocity 00 01 10 11
//For drum tracks only velocity is saved, step note is defined by patternData.trackNote[0-9]
//A single byte can hold the velocity for 4 steps in a single variation in AABBCCDD format.
//Double step byte, one bit per step - only a one bit flag per step, packed as AABBCCDD.

//Note Tracks:
//Full format:
//1 byte per step for the note value 0-127.
//2 extra byte per step for velocity and additional flags (noteStepsExtras).

////    Bits in noteStepsExtras

//noteStepsExtras[x][0] and noteStepsExtras[x][1] are used to store additional step data.
//Each variation occupies 2 bits, packed into nibbles (4 bits) per byte.

// noteStepsExtras[x][0] - Velocity (Lower Nybble)
// ------------------------------------------------
// - Stores velocity information for each variation:
//   - 00: Velocity 0 (step off)
//   - 01: Velocity 1 (low)
//   - 10: Velocity 2 (medium)
//   - 11: Velocity 3 (high)
// - Each variation's 2 bits are mapped sequentially:
//   - Variation A: Bits 0–1
//   - Variation B: Bits 2–3
//   - Variation C: Bits 4–5
//   - Variation D: Bits 6–7

// noteStepsExtras[x][1] - Attributes (Upper Byte)
// ------------------------------------------------
// - Stores step attributes for each variation:
//   - 00: No attribute
//   - 01: Slide (legato enabled)
//   - 10: Double Note (triggers double note event) //not used
//   - 11: Note Off (explicit note-off for this step)
// - Each variation's 2 bits are mapped sequentially:
//   - Variation A: Bits 0–1
//   - Variation B: Bits 2–3
//   - Variation C: Bits 4–5
//   - Variation D: Bits 6–7


// Compact Encoding:
// -----------------
// Each note step uses 2 bytes (16 bits) to store velocity and attributes for all 4 variations.

struct WSTEPS  // Actual STEP data
{
  byte steps[DRUM_TRACKS];  // steps are stored as 8 bits variations in AABBCCDD format. 2 bits per step per variation. 0~3
  uint16_t stepsDouble[4];  // drum tracks, 4 bits each for the ABCD variations
  byte noteSteps[NOTE_TRACKS][4];
  byte noteStepsExtras[NOTE_TRACKS][2];  // 2 bytes packed as ABCD: [0] = Velocity (2 bits) ABCD, [1] = Glide/Double ABCD (2 bits)
  //
  void init() {
    memset(steps, 0, sizeof(steps));
    memset(stepsDouble, 0, sizeof(stepsDouble));
    memset(noteSteps, 0, sizeof(noteSteps));
    memset(noteStepsExtras, 0, sizeof(noteStepsExtras));
  }
};

struct WPATTERN  // Config per PATTERN
{
  byte totalVariations;
  byte lastNote[NOTE_TRACKS];
  volatile uint16_t nextPattern;
  volatile byte playsPattern;
  volatile byte playsChain;
  byte programChange[DRUM_TRACKS + NOTE_TRACKS];
  volatile byte shuffleDelay;
  volatile byte trackLen[DRUM_TRACKS + NOTE_TRACKS];
  volatile byte trackProbability[DRUM_TRACKS + NOTE_TRACKS];
  byte drumNoteLen[DRUM_TRACKS + NOTE_TRACKS];
  byte trackNote[DRUM_TRACKS];
  byte trackMidiCH[DRUM_TRACKS + NOTE_TRACKS];  // 0-15
  byte accentValues[3];


  void init() {

    memset(lastNote, DEFAULT_NOTE, sizeof(lastNote));
    memset(programChange, 0, sizeof(programChange));
    memset((void *)trackLen, 16, sizeof(trackLen));
    memset((void *)trackProbability, 0, sizeof(trackProbability));
    for (byte i = 0; i < DRUM_TRACKS + NOTE_TRACKS; i++) {
      drumNoteLen[i] = (i < DRUM_TRACKS) ? 1 : 6;
    }
    memset(trackMidiCH, (DEFAULT_MIDI_CH - 1), sizeof(trackMidiCH));
    for (xm = 0; xm < NOTE_TRACKS; xm++) {
      trackMidiCH[DRUM_TRACKS + xm] = xm;
    }

    totalVariations = 4;
    nextPattern = 0;
    playsPattern = 0;
    playsChain = 1;
    shuffleDelay = 0;

    accentValues[0] = 63;
    accentValues[1] = 93;
    accentValues[2] = 127;

    trackNote[0] = 36;  //default drum notes can be changed here
    trackNote[1] = 38;
    trackNote[2] = 42;
    trackNote[3] = 46;

    trackNote[4] = 37;
    trackNote[5] = 39;
    trackNote[6] = 44;
    trackNote[7] = 51;

    trackNote[8] = 50;
    trackNote[9] = 57;
  }
};
//

struct GCONFIG  // GLOBAL config
{
  bool seqSyncOut;
  byte midiInputToPatternChannel;
  bool midiClockAuto;
  bool writeProtectFlash;
  volatile byte tickOut;
  volatile byte tickOutLen;
  volatile byte midiClockDivide;
  volatile bool multitrackRec;
  volatile bool midiThru;
  byte Brightness;

  void init() {

    tickOut = 2;
    tickOutLen = 1;
    seqSyncOut = true;
    midiClockAuto = true;
    midiInputToPatternChannel = 0;
    writeProtectFlash = false;
    midiClockDivide = 1;
    Brightness = 1;
    multitrackRec = false;
    midiThru = true;
  }
};

#pragma pack(pop)


//

WSTEPS stepsData[STEPS];  //actual step data, velocity, 32nd note drumstep, note track pitch/velocity/glide/noteoff...
WPATTERN patternData;     //config data, unique saved for each pattern
GCONFIG globalData;       //global data, saved in a permanent resident spot in memory, valid for any pattern

//clipboard for track copy operations
struct TrackClipboard {
  byte noteSteps[STEPS][4];        // Used for NOTE tracks or DRUM (constant value)
  byte noteStepsExtras[STEPS][2];  // [0] velocity packed (AABBCCDD), [1] extras packed (AABBCCDD)
};


// ---7 SEGMENT DISPLAY DEFINITIONS
/*
 *    A     --
 *   F B   |  |
 *    G     --
 *   E C   |  |
 *    D     --
 *       X      .
 *       
 *       
 *  XGFEDCBA
 *  01011000
 *  *  B00001000
 *  *  B01001000
 *  *  B01001001
 *  *  B00010000
 *  *  B00001000
 *  *  B00000100
 *  *  B01000000
 *  *  B00100000
 */


#define S_MUTE B00001000    //CHAR SIGN FOR MUTED
#define S_UNMUTE B00110111  //CHAR SIGN FOR UNMUTED

//Below: Alphabet and custom number definition
#define S_1 B00000110
#define S_1_ B00110000
#define S_W1 B01100100
#define S_W2 B01010010
#define S_u B00011100
#define S_n B01010100
#define S_d B01011110
#define S_d_DOT B11011110
#define S_V B00111100
#define S_X B01000100
#define S_Z B01101100
#define S_R B01010111
#define S_DASH B01000000
#define S_S B01101101
#define S_Y B01101110
#define S_L B00111000
#define S_O B00111111
#define S_U B00111110
#define S_G B00111101
#define S_G_DOT B10111101
#define S_F B01110001
#define S_o B01011100
#define S_C B00111001
#define S_C_DOT B10111001
#define S_N B00110111
#define S_E B01111001
#define S_E_DOT B11111001
#define S_t B01111000
#define S_n B01010100
#define S_K B01111010
#define S_r B01010000
#define S_T B00110001
#define S_d B01011110
#define S_c B01011000
#define S_A B01110111
#define S_A_DOT B11110111
#define S_b B01111100
#define S_b_DOT B11111100
#define S_P B01110011
#define S_P_DOT B11110011
#define S_I B00110000
#define S_H B01110110
#define S__ B00001000
#define S_DOT B10000000

//Below: Char representation for empty step, step velocities ----- 32nd note, legato and note off display chars definitions are embedded in code as where needed
const byte stepChars[4] PROGMEM = { B00000000, B01000000, B01100010, B01100011 };

//Below: Standardized number font
const byte numbers[10] PROGMEM = { B00111111, B00000110, B01011011, B01001111, B01100110, B01101101, B01111101, B00000111, B01111111, B01101111 };

//Below: Animation chars for exit
const byte exitKeyMapping[8] = {
  S_R,
  S_E,
  S_T,
  S_U,
  S_R,
  S_N,
  B00000000,
  B00000000,
};

//END OF LINE