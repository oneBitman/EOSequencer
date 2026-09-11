# EOSequencer

**EOSequencer** is an open-source hardware MIDI step sequencer firmware for Arduino and compatible microcontrollers. It is built as an enhanced fork of the classic **Beat707NXT** engine by William Kalfelz, featuring updated workflow options, refined timing stability, and customized interface controls.

🌐 **Website (buy a kit):** [https://dsmcz.com/prestashop/en/home/23957-eoseq-full-kit-with-eu-psu-yellow-purple-enclousure-v10-may-2026.html](https://dsmcz.com/prestashop/en/home/23957-eoseq-full-kit-with-eu-psu-yellow-purple-enclousure-v10-may-2026.html)

---

## Features

- **Multi-track Step Sequencing:** Intuitive step entry for rhythm and melodic tracks.
- **Hardware Integration:** Direct support for MIDI In/Out, TM1638 led & key board based interface.
- **Enhanced Engine:** Modifications built over Beat707NXT for improved stability and workflow options.
- **Arduino Compatible:** Designed to compile cleanly using the standard Arduino IDE.

---

## Hardware Requirements

To build and run EOSequencer, you will need:

1. **Microcontroller Board:**
   - Only for Arduino Pro Mini 328P, 3.3V, 8Mhz
   - Eosequencer was developed for ATmega328P but code will run flawless on 328PB variant
2. **User Interface Components:**
   - 3x TM1638 board
3. **MIDI Hardware Interface:**
   - Standard DIN-5 or TRS MIDI input and output circuits (optocoupler-driven MIDI IN, standard MIDI OUT driver).

---

## Software & Build Setup

### Option A: Using Arduino IDE

1. **Download & Install Arduino IDE** (v1.8.x or v2.x).
2. **Clone the Repository:**
   ```bash
   git clone https://github.com/oneBitman/EOSequencer.git
   ```
3. **Install Required Libraries:**
   Ensure the following libraries are installed in your Arduino library manager:
   - `MIDI` (by Francois Best)
   - `Wire` & `SPI` (Built-in)
4. **Open Project:**
   Open `EOSequencer.ino` inside the `EOSequencer/` folder.
5. **Compile & Upload:**
   You MUST install MiniCore board library for gaining more programming space, code exceeds 30720 bytes!
   Recommended bootloader (512 bytes): Optiboot/Urboot --- urboot_m328p_1s_autobaud_uart0_rxd0_txd1_led+b5_ee_ce_hw_stk500

   How to install MiniCore board library (Arduino IDE 2.x on Windows 11)

   File → Preferences → Additional Boards Manager URLs
   1.) → paste: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
   2.) Tools → Board → Boards Manager… → search MiniCore → Install.
   Choose Tools > Board > MiniCore > Atmega328
    Select your target board and port under **Tools**, then click **Upload**.

