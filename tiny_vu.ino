// --- Version 1.2.1 (Breadboard Final) ---
// - Audio Processing: Max-Min peak-to-peak to ignore DC offset.
// - Palette UI: Added 4 distinct color schemes (Classic, Synthwave, Deep Cyan, All Red).
// - Palette Control: Double-clap state machine to cycle through color schemes.
// - Boot Stability: 500ms setup() delay ignores power-on transients.

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define LED_PIN      1
#define NUM_LEDS     7
#define TOP          (NUM_LEDS + 2)
#define SAMPLES      20  

#define BRIGHTNESS   15  
#define DEAD_BAND    48  
#define INPUT_DIV    1   

// --- CLAP DETECTION KNOBS ---
#define CLAP_THRESHOLD     450  // Requires a very loud clap or a physical tap
#define CLAP_QUIET_DROP    100  
#define CLAP_MIN_INTERVAL  100  
#define CLAP_MAX_INTERVAL  500  

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t volCount = 0;
unsigned int lvl = 20;
unsigned int minLvlAvg = 0;
unsigned int maxLvlAvg = 512;
unsigned int vol[SAMPLES];

uint8_t currentScheme = 0;

uint32_t VUColor(byte i) {
  if (currentScheme == 0) {
    // --- 1. Classic Color Scheme ---
    switch (i) {
      case 0: return strip.Color(90, 165, 0);    // Green-yellow
      case 1: return strip.Color(150, 105, 0);   // Amber
      case 2: return strip.Color(210, 45, 0);    // Orange-red
      case 3: return strip.Color(240, 0, 15);    // Red
      case 4: return strip.Color(220, 0, 40);    // Ruby Pink
      case 5: return strip.Color(80, 0, 200);    // Deep Violet
      default:return strip.Color(60, 0, 195);    // Blue-violet
    }
  } else if (currentScheme == 1) {
    // --- 2. Smoothed Synthwave Base with Raspberry Peak ---
    switch (i) {
      case 0: return strip.Color(0, 255, 255);   // Pure Cyan
      case 1: return strip.Color(0, 150, 255);   // Sky Blue 
      case 2: return strip.Color(20, 50, 255);   // True Blue 
      case 3: return strip.Color(80, 0, 255);    // Indigo 
      case 4: return strip.Color(140, 0, 220);   // Violet 
      case 5: return strip.Color(255, 0, 255);   // Pure Magenta (User Tweak)
      default:return strip.Color(255, 0, 125);   // Raspberry (User Tweak)
    }
  } else if (currentScheme == 2) {
    // --- 3. Deep Cyan Base, Red Peaks Color Scheme ---
    if (i < 5) return strip.Color(0, 130, 210);  // Deep Cyan
    else return strip.Color(240, 0, 0);          // Red
  } else {
    // --- 4. All Red Color Scheme ---
    return strip.Color(240, 0, 0);               // Red
  }
}

void setupADC_Differential20x() {
  ADMUX = (1 << REFS1) | (0 << REFS0) | (0 << ADLAR) | (0 << REFS2) | (0 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);     
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);    
}

int readMicRaw() {
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  uint8_t low = ADCL;        
  int8_t high = (int8_t)ADCH;
  return ((int)high << 8) | low;
}

unsigned int readMicPeak() {
  int signalMax = -32768; 
  int signalMin = 32767;  
  unsigned long startMillis = millis();
  
  while (millis() - startMillis < 10) {
    int raw = readMicRaw();
    if (raw > signalMax) signalMax = raw;
    if (raw < signalMin) signalMin = raw;
  }

  unsigned int peakToPeak = 0;
  if (signalMax > signalMin) peakToPeak = signalMax - signalMin;
  
  unsigned int mag = peakToPeak / 2;

  // --- DOUBLE CLAP STATE MACHINE ---
  static uint8_t clapState = 0; 
  static unsigned long lastClapTime = 0;
  unsigned long now = millis();

  if (clapState > 0 && (now - lastClapTime > CLAP_MAX_INTERVAL)) {
    clapState = 0; 
  }

  if (mag > CLAP_THRESHOLD) {
    if (clapState == 0) {
      clapState = 1;
      lastClapTime = now;
    } else if (clapState == 2 && (now - lastClapTime > CLAP_MIN_INTERVAL)) {
      currentScheme++;
      if (currentScheme > 3) currentScheme = 0;
      clapState = 0; 
      lastClapTime = now; 
    }
  } else if (mag < CLAP_QUIET_DROP) {
    if (clapState == 1 && (now - lastClapTime > 50)) {
      clapState = 2; 
    }
  }
  // ---------------------------------

  if (mag > DEAD_BAND) return mag - DEAD_BAND; 
  return 0;
}

unsigned int readMicHeldMagnitude() {
  static unsigned int held = 0;
  unsigned int mag = readMicPeak();
  if (mag > held) held = mag;
  else held = (held * 3) >> 2; 
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
  
  delay(500);   
}

void loop() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int height;

  unsigned int n = readMicHeldMagnitude() / INPUT_DIV;

  lvl = ((lvl * 7) + n) >> 3;

  if (maxLvlAvg <= minLvlAvg) maxLvlAvg = minLvlAvg + TOP + 1;

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

  minLvlAvg = (minLvlAvg * 15 + minLvl) >> 4;
  maxLvlAvg = (maxLvlAvg * 15 + maxLvl) >> 4;
}
