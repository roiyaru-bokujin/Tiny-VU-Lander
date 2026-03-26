# Micro NeoPixel VU Meter (ATtiny85)

A highly responsive, auto-scaling audio visualizer designed for miniature hardware sculptures. Powered by an ATtiny85, a single CR2032 coin cell, and a custom 4x-speed Exponential Moving Average (EMA) algorithm to keep the visuals punchy and "alive" without getting stuck on loud noises.

## Hardware Components
* **Microcontroller:** ATtiny85 (Running at 8MHz)
* **Display:** 7x WS2812B NeoPixels
* **Audio Input:** Analog 9.7mm 2-Pin Electret Microphone Module
* **Power Supply:** CR2032 Coin Cell Battery (3V)
* **Boost Converter:** Pololu NCP1402 3.3V Step-Up (Essential for preventing coin-cell brownouts)
* **Capacitor:** 100µF across power lines to smooth peak current draws

## Wiring & Power Architecture


Driving NeoPixels and an active microphone from a CR2032 is an extreme edge case. To prevent the ATtiny from browning out and constantly resetting (Pin 1/8 voltage collapse), the power must be actively boosted and smoothed.

* **Battery:** Connects to Pololu `VIN` and `GND`.
* **Pololu `VOUT` (3.3V):** Connects to ATtiny VCC (Pin 8), Microphone VCC, and NeoPixel 5V/VCC.
* **ATtiny Pin 2 (PB3) & Pin 3 (PB4):** Microphone Signal and Reference lines (Differential ADC).
* **ATtiny Pin 6 (PB1):** NeoPixel Data Line.
* **Grounds:** All components must share a common ground.

## How the Code Works
* **20x Hardware Gain:** Uses the ATtiny85's differential ADC to heavily amplify quiet ambient audio directly at the hardware level.
* **DC Offset Rejection:** A strict dead band mathematically subtracts the 1.5V baseline noise floor so the meter stays dark in a silent room.
* **Fast Auto-Scaling:** A 4x-speed Exponential Moving Average (EMA) tracks the room's volume to dynamically adjust the visual ceiling and floor.

---
*Adapted from the TinyVU lander by Mohit Bhoite.*
