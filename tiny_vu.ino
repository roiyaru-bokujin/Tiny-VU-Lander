// --- Version 1.1 ---
// - Configured differential ADC with 20x hardware gain
// - Changed ADC prescaler to 125kHz for ultra-low noise reads
// - Replaced blocking sleep with 10ms peak sampling window
// - Implemented Smooth Gate subtraction for signal spikes
// - Mapped raw mic signals to an adaptive VU scale
// - Swapped differential signal pins
// - Switched to internal 1.1v reference
//
// --- Version 1.2 ---
// - Locked DEAD_BAND to 150 to perfectly bury the 1.5V DC offset
// - Lowered BRIGHTNESS to 10 to protect coin cell power supply
// - Lowered SAMPLES to 20 for shorter volume memory window
// - Sped up EMA math (>> 4) for 4x faster live responsiveness
// - Removed minimum window constraints for maximum whisper-sensitivity

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define LED_PIN      1
#define NUM_LEDS     7
#define TOP          (NUM_LEDS + 2)
#define SAMPLES      20  

#define BRIGHTNESS   10  
#define DEAD_BAND    150 
#define INPUT_DIV    2   

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t volCount = 0;
unsigned int lvl = 20;
unsigned int minLvlAvg = 0;
unsigned int maxLvlAvg = 512;
unsigned int vol[SAMPLES];

uint32_t VUColor(byte i) {
  switch (i) {
    case 0: return strip.Color(90, 165, 0);    // green-yellow
    case 1: return strip.Color(150, 105, 0);   // amber
    case 2: return strip.Color(210, 45, 0);    // orange-red
    case 3: return strip.Color(240, 0, 15);    // red
    case 4: return strip.Color(180, 0, 75);    // magenta-red
    case 5: return strip.Color(120, 0, 135);   // purple
    default:return strip.Color(60, 0, 195);    // blue-violet
  }
}

void setupADC_Differential20x() {
  ADMUX =
    (1 << REFS1) |   
    (0 << REFS0) |
    (0 << ADLAR) |   
    (0 << REFS2) |
    (0 << MUX3)  |
    (1 << MUX2)  |
    (1 << MUX1)  |
    (1 << MUX0);     

  ADCSRA =
    (1 << ADEN)  |
    (1 << ADPS2) |   
    (1 << ADPS1) |   
    (0 << ADPS0);    
}

int readMicRaw() {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  uint8_t low = ADCL;        
  int8_t high = (int8_t)ADCH;
  int value = ((int)high << 8) | low;

  return value;
}

unsigned int readMicPeak() {
  unsigned int maxPeak = 0;
  unsigned long startMillis = millis();
  
  while (millis() - startMillis < 10) {
    int raw = readMicRaw();
    unsigned int mag = abs(raw);

    if (mag > DEAD_BAND) {
      mag = mag - DEAD_BAND; 
    } else {
      mag = 0; 
    }

    if (mag > maxPeak) {
      maxPeak = mag;
    }
  }
  return maxPeak;
}

unsigned int readMicHeldMagnitude() {
  static unsigned int held = 0;

  unsigned int mag = readMicPeak();

  if (mag > held) {
    held = mag;
  } else {
    held = held >> 1; 
  }

  return held;
}

void setup() {
  clock_prescale_set(clock_div_1);

  memset(vol, 0, sizeof(vol));

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  setupADC_Differential20x();
  delay(1);   
}

void loop() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int height;

  unsigned int n = readMicHeldMagnitude() / INPUT_DIV;

  lvl = ((lvl * 7) + n) >> 3;

  if (maxLvlAvg <= minLvlAvg) {
    maxLvlAvg = minLvlAvg + TOP + 1;
  }

  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0) height = 0;
  if (height > TOP) height = TOP;

  strip.clear();
  for (i = 0; i < height && i < NUM_LEDS; i++) {
    strip.setPixelColor(i, VUColor(i));
  }
  strip.show();

  vol[volCount] = n;
  if (++volCount >= SAMPLES) volCount = 0;

  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl) minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }

  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;

  // Faster auto-scaling EMA: Adapts 4x quicker to changes in the song's volume
  minLvlAvg = (minLvlAvg * 15 + minLvl) >> 4;
  maxLvlAvg = (maxLvlAvg * 15 + maxLvl) >> 4;
}