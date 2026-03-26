# Micro NeoPixel VU Meter (ATtiny85)

A highly responsive, auto-scaling audio visualizer circuit sculpture. Powered by an ATtiny85, a single CR2032 coin cell, and a custom 4x-speed Exponential Moving Average (EMA) algorithm to keep the visuals punchy and alive. It features an interactive, acoustic double-clap UI to switch between 4 custom color palettes.

## Hardware Components
* **Microcontroller:** ATtiny85 (Running at 8MHz)
* **LED Array:** 7x Adafruit NeoPixel SK6812 (3535) 
* **Audio Input:** 9.7mm 2-Pin Analog Electret Capsule
* **Power Supply:** CR2032 3V Coin Cell Battery
* **Boost Converter:** Pololu NCP1402 3.3V Step-Up
* **Passive Components:** * 1x 100µF electrolytic capacitor (bulk power smoothing)
  * 2x 0.1µF ceramic capacitors (one for ATtiny decoupling, one for raw VIN filtering)
  * 1x 1µF ceramic capacitor (audio AC coupling)
  * 4x 10kΩ resistors (Reset pull-up, mic bias, and voltage divider)

## Wiring & Power Architecture

Driving NeoPixels and an active microphone from a single CR2032 is an extreme edge case. To prevent the ATtiny from browning out and constantly resetting, the power must be actively boosted, smoothed, and strictly limited in software.

* **Power & Stability:** The battery feeds the Pololu boost converter. The Pololu's 3.3V `VOUT` powers the ATtiny VCC (Pin 8), the NeoPixels, and the microphone's 10kΩ bias resistor. To prevent massive current spikes from crashing the system, the code hard-caps LED brightness at a maximum of `15`.
* **Microphone & Audio (Pin 3):** The bare electret capsule's positive pin connects to the 3.3V bias resistor and routes the raw audio through a 1µF AC-coupling capacitor directly into ATtiny Pin 3.
* **The Reference Hub (Pin 2):** Acts as a 1.65V midpoint. This is built structurally on the IC socket itself using three 10kΩ resistors connecting to VCC (Pin 8), GND (Pin 4), and Pin 3.
* **Data & Structural Support:** Pin 6 drives the NeoPixel data line. For absolute electrical stability, a 10kΩ reset pull-up bridges Pin 1 to Pin 8, and a 0.1µF decoupling capacitor bridges Pin 8 to Pin 4, both soldered directly to the DIP8 socket.

## How the Code Works
* **20x Hardware Gain:** Uses the ATtiny85's differential ADC to heavily amplify quiet ambient audio directly at the hardware level.
* **Software Gain & Tuned Deadband:** Applies a 3x software multiplier to compensate for the relatively lower-sensitivity 9.7mm microphone. A strict deadband (35) mathematically swallows the Pololu's electrical switching noise before the multiplier is applied, ensuring the LEDs stay pitch black in a silent room.
* **Interactive Palette UI:** Features an acoustic state machine that detects double-claps (threshold: 150) to cycle the NeoPixels through 4 distinct color schemes (Classic, Synthwave, Deep Cyan with Red Peak, All Red).
* **Fast Auto-Scaling:** A 4x-speed Exponential Moving Average (EMA) tracks the room's volume to dynamically adjust the visual ceiling and floor.

---
*Adapted from the TinyVU lander by Mohit Bhoite.*
