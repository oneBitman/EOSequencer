/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 */
#include "Lib_Flash.h"

void checkIfDataNeedsSaving() {
  if (somethingChangedPattern) {
    somethingChangedPattern = false;
    if (!globalData.writeProtectFlash) {

      savePatternData(true);
      //saveStepsData();
    }
  }

  if (somethingChangedGlobal) {
    somethingChangedGlobal = false;
    if (!globalData.writeProtectFlash) saveGlobalData(true);
  }
}

void eraseSector(uint16_t _pagePos) {

  if (!flash.eraseSector(_pagePos, 0)) showErrorMsg(flash.error());
}

void saveGlobalData(byte _eraseSector) {
  if (_eraseSector) {
    cli();

    eraseSector(32640 + (masterOffset * 16));
    
  }

  
  if (!flash.writeAnything(32640 + (masterOffset * 16), (uint8_t)0, globalData)) {
    showErrorMsg(flash.error());
  }
  sei();
}

void savePatternData(byte _eraseSector) {
  uint16_t base = (uint16_t)(masterOffset * 4112)
                + 16
                + (uint16_t)(currentPatternBank * (16 * 16))
                + (uint16_t)(currentPattern * 16);

  cli();
  if (_eraseSector) eraseSector(base);                     // same sector for both
  bool ok = flash.writeAnything(base,   (uint8_t)0, patternData)
         && flash.writeAnything(base+1, (uint8_t)0, stepsData);
  sei();

  if (!ok) showErrorMsg(flash.error());
}

void loadPattern(byte pattern, bool force) {
  checkIfDataNeedsSaving();

  if (!seqPlaying || force) {
    ignoreButtons = true;

    patternPagePos = ((masterOffset * 4112) + 16 + nextPatternBank * (16 * 16)) + (pattern * 16);  // Use nextPatternBank, this was a legacy bug
    if (!flash.readAnything(patternPagePos, (uint8_t)0, patternData)) showErrorMsg(flash.error());
    patternPagePos++;
    if (!flash.readAnything(patternPagePos, (uint8_t)0, stepsData)) showErrorMsg(flash.error());

    currentPattern = pattern;

    if (!flash.readAnything(32640 + (masterOffset * 16), (uint8_t)0, globalData)) showErrorMsg(flash.error());

    currentPatternBank = nextPatternBank;
    patternBitsSelector = 0;

    if (isSelectingBank) bitSet(patternBitsSelector, nextPatternBank);
    else bitSet(patternBitsSelector, currentPattern);


    ignoreButtons = false;
  } else {
    streamNextPattern = true;
  }
}

/*
 * 
 * Created by William Kalfelz @ Beat707 (c) 2018 - http://www.Beat707.com
 * 
 * Flash Page Size = 256 bytes
 * Capacity =  8388608 bytes
 * Number of Pages = 32768
 * 
 * Config: 1 page
 * PatternBank: 1 page
 * Pattern Size: 2 pages
 * 
 * Page 0 is just the B707 header
 * Page 1 starts the data
 *                      Pg1    Config
 *                      Pg2    PatternBank
 *                      Pg3    Pattern 01
 *                      Pg4    Pattern 01 (continued)
 *                                  ~
 *                          
 * Total of .... pages per patternBank -  1 PATTERN BANK IS 16 patterns now
 * The problem is that the W25Q64 IC only erases pages in groups of 16. So we end up with only 2048 pages.
 * So we need a full 16 pages for the patternBank data + config, them another 16 pages per pattern. Total of .... pages per patternBank.
 * 
 * ----------------------------------------------------------------------
 * New format
 * We have 32768 pages, so with 2 bytes (16 bits) we can store a full page address
 * 
 * For storing global setup values, Pages 32752–32767 provide a safe, 
 * isolated block that meets the 16-page erase requirement and remains well out of the range of PatternBank data. 
 *
 */


void flashInit(bool force) {

  totalFlashErrors = 0;
  if (!flash.begin(FLASH_CHIPSIZE)) { showErrorMsg(127); }
  waitMs(120);
  if (!flash.readAnything((masterOffset * 4112) + 0, (uint8_t)0, flashHeader)) showErrorMsg(flash.error());

  if (flashHeader[0] != 'E' || flashHeader[1] != '0' || flashHeader[2] != 'S' || flashHeader[3] != 'E' || flashHeader[4] != 'Q')  //check for header, if no header force flash init
  { force = true; }                                                                                                               //flash init for new chips

  if (force) {
    stopTimer(false);
    showWaitMsg(-1);
    waitMs(2000);
    bool sectorErase = true;
    int porc = 0;

    for (byte x = 0; x < PT_BANKS; x++) {
      initPatternBank(x, sectorErase, porc, false, 3);
    }

    saveHeader(sectorErase);
    globalData.init();
    saveGlobalData(true);

    if (!flash.readAnything((masterOffset * 4112) + 0, (uint8_t)0, flashHeader)) showErrorMsg(flash.error());
    if (flashHeader[0] != 'E' || flashHeader[1] != '0' || flashHeader[2] != 'S' || flashHeader[3] != 'E' || flashHeader[4] != 'Q' || flashHeader[5] != FLASH_VERSION) showErrorMsg(98);

    if (totalFlashErrors > 0) {
      showErrorMsg(totalFlashErrors, true);
      waitMs(2000);
      totalFlashErrors = 0;
    }

    startTimer(false);
  }
}

void saveHeader(bool sectorErase) {
  uint16_t base = (uint16_t)masterOffset * 4112;

  uint8_t hdr[6];
  hdr[0]='E'; hdr[1]='0'; hdr[2]='S'; hdr[3]='E'; hdr[4]='Q'; hdr[5]=FLASH_VERSION;

  if (sectorErase) eraseSector(base);
  if (!flash.writeAnything(base, (uint8_t)0, hdr)) showErrorMsg(flash.error());
}

void initPatternBank(byte patternBank, bool sectorErase, int &porc, bool patternBankOnly, byte initMode) {
  pagePos = (masterOffset * 4112) + 16 + (patternBank * (16 * 16));  // correct base
  //pagePos += 16;

  stopTimer(false);
  for (byte p = 0; p < PATTERNS; p++) {
    if (initMode == 1) {
      // MODE 1: Clone setup only, preserve steps
      // Read target pattern’s steps from flash into stepsData
      if (!flash.readAnything(pagePos + 1, (uint8_t)0, stepsData)) showErrorMsg(flash.error());
      if (sectorErase) eraseSector(pagePos);
      // Write current patternData (setup)
      if (!flash.writeAnything(pagePos, (uint8_t)0, patternData)) showErrorMsg(flash.error());
      pagePos++;
      // Write original steps back
      if (!flash.writeAnything(pagePos, (uint8_t)0, stepsData)) showErrorMsg(flash.error());
      pagePos += 15;
    }

    else if (initMode == 2 || initMode == 3) {
      // MODE 2 and 3: Overwrite with current setup + stepsData
      if (sectorErase) eraseSector(pagePos);
      if (!flash.writeAnything(pagePos, (uint8_t)0, patternData)) showErrorMsg(flash.error());
      pagePos++;
      if (!flash.writeAnything(pagePos, (uint8_t)0, stepsData)) showErrorMsg(flash.error());
      pagePos += 15;
    }

    porc++;
    if (patternBankOnly) showWaitMsg(porc * 2);
    else showWaitMsg(byte(porc / 20));
  }
  startTimer(false);
  initMode = 0;
}

void checkPatternStream() {
  if (loadPatternNow) {
    loadPattern(nextPattern, true);
    loadPatternNow = (streamNextPattern = (ignoreButtons = false));
    resetProgramChangeAndCC();
  }
}

void loadPatternBank(byte patternBank) {
  currentPattern = nextPattern = 0;
  currentPatternBank = nextPatternBank = patternBank;
  loadPattern(0);

  if (!flash.readAnything(32640 + (masterOffset * 16), (uint8_t)0, globalData)) showErrorMsg(flash.error());
}

// END OF LINE