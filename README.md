# Project-01

# Dual-Radio 2.4 GHz RF Spectrum Scanner & Channel-Activity Monitor

## Project Overview
This project builds a dual-radio 2.4 GHz RF spectrum scanner and channel-activity monitor on an ESP32 DevKit paired with two nRF24L01+ PA/LNA transceivers and a momentary push button. Although the initial request referenced ESP32 Marauder, Marauder targets the ESP32's internal Wi-Fi and Bluetooth radio rather than external nRF24L01 modules—so this implementation uses the RF24 library to sweep the 2.400-2.525 GHz band (channels 0-125) and report peak RF energy over serial.

## Hardware Architecture & Signal Flow
- Microcontroller & SPI Bus: The ESP32 DevKit acts as the bus master using a shared SPI bus across both transceivers so they can communicate with the microcontroller without needing separate data lines per radio.
- Independent Radio Control: Each nRF24L01+ module has its own enable and chip-select controls so SPI transfers and listening/standby states stay isolated between the two radios.
- Power & Decoupling: PA/LNA modules run on 3.3V (never 5V) and draw current spikes during RF operation. Decoupling capacitors (10 uF-100 uF) across each module's power and ground stabilize the supply rail and help prevent ESP32 brownouts.
- User Input: A single push button wired to ground uses an internal pull-up and a debounced hardware interrupt to toggle scan modes without blocking the sweep loop.

## Firmware Behavior & Scan Modes
1. Initialization (setup()): Starts serial logging at 115200 baud, configures the button interrupt, initializes the SPI bus, and configures both radios with auto-acknowledgment disabled, 1 Mbps data rate, and conservative PA output (RF24_PA_LOW) for bench stability.
2. Channel Energy Sweeps (scanEnergy()): Steps through channels 0 to 125 (2.400 GHz to 2.525 GHz), briefly listens (130 us), and increments an activity counter when carrier/RPD activity is detected across 64 samples per channel.
3. Mode 1 - Single-Radio Scan (Default): Sweeps the first radio across all 126 channels, finds the highest-activity channel, and prints the channel number, frequency (2.400 GHz + channel), and hit count out of 64.
4. Mode 2 - Dual-Radio Scan (Button Toggle): Pressing the push button flips dualSweepMode, sweeping the second radio after the first and printing peak channel and activity readings for both radios side by side.

## Practical Use & Limitations
- What it shows well: Relative 2.4 GHz channel occupancy and noisy frequencies near Wi-Fi routers, Bluetooth peripherals, cordless/wireless links, or other 2.4 GHz emitters in a lab or authorized test setup.
- What it does not do: It does not run ESP32 Marauder Wi-Fi/BLE firmware, decode full Wi-Fi/Bluetooth frames, or act as a calibrated lab spectrum analyzer—carrier/RPD flags indicate presence/energy rather than exact dBm power levels.
- Responsible operation: Keep testing confined to your own hardware or authorized environments, and avoid transmitting high-power interference onto shared or unauthorized wireless channels.
