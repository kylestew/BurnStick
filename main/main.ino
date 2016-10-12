/*
 * Burn
 * =============
 * LED based #IXArt platform
 */
#define ARM_MATH_CM4
#include <arm_math.h>
#include <FastLED.h>
#include "RingCoder.h"

#define SERIAL_DEBUG    true

/* Status LED */
const int STATUS_LED = 13;

/* Pixels */
const int PIXEL_PIN = 6;
const int PIXEL_COUNT = 1;
CRGB pixels[PIXEL_COUNT];
int brightness = 32;

/* Ring Encoder */
// (pins fill left
const int ENCB = 0; // All pins interrupt on Teensy
const int ENCA = 1;
const int ENC_SW = 2;
const int LEDR = 3; // (not enabled)
const int LEDB = 3; // PWM enabled LED driver
const int LEDG = 4; // PWM enabled LED driver
// ring led shift register
const int DAT = 5;
const int CLR = 9;
const int CLK = 10;
const int LATCH = 11;
const int EN = 12;
RingCoder ringcoder = RingCoder(ENCB, ENCA, LEDR, LEDB, LEDG, ENC_SW, DAT, CLR, CLK, LATCH, EN);

/* Mode */
int mode = 0;   // 0 == set animation program
                // 1 == set LED brightness
const int ANIMATION_COUNT = 3; // number of animations implemented
int animationProgram = 0;

/* Audio FFT Sampling */
const int AUDIO_INPUT_PIN = 14;
const int ANALOG_READ_RESOLUTION = 10;   // Bits of resolution for the ADC.
const int ANALOG_READ_AVERAGING = 16;    // Number of samples to average with each ADC reading.
const int FFT_SIZE = 256;                // Size of the FFT.  Realistically can only be at most 256
const int SAMPLE_RATE_HZ = 9000;         // Sample rate of the audio in hertz
IntervalTimer samplingTimer;
int sampleCounter = 0;
float samples[FFT_SIZE*2];

/* FFT Processing */
float magnitudes[FFT_SIZE];
// const int BAND_START = 40.0;             // start of freq sample band
// const int BAND_END = 920.0;              // end of freq sample band
// float GAIN = 3.0;                        // increase the color volume
// float LERP_DOWN = 0.09;                       // smooth out responsiveness
// float LERP_UP = 0.7;                       // smooth out responsiveness
// float HUE_SHIFT = 40.0;                  // shift hue based on intensity
// float SPECTRUM_MIN_DB = 45.0;            // Audio intensity (in decibels) that maps to low LED brightness.
// float SPECTRUM_MAX_DB = 95.0;            // Audio intensity (in decibels) that maps to high LED brightness.


// === LIFECYCLE ===
void setup() {
  if (SERIAL_DEBUG) {
    Serial.begin(115200);
  }

  // status LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // initialze Pixels
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(pixels, PIXEL_COUNT);
  FastLED.setBrightness(brightness);
  clearPixels();

  // boot animation on ring coder
  ringcoder.setKnobRgb(255, 255, 255);
  ringcoder.spin();
  ringcoder.reverse_spin();
  setMode(0);


  // ADC -> FFT input
  // pinMode(AUDIO_INPUT_PIN, INPUT);
  // analogReadResolution(ANALOG_READ_RESOLUTION);
  // analogReadAveraging(ANALOG_READ_AVERAGING);
  // samplingBegin();
}

void loop() {
  // DON'T BLOCK MAIN LOOP
  // ring coder is a polling input

  // watch for button changes
  if (ringcoder.update() && ringcoder.button() == HIGH) {
    // toggle mode
    setMode(mode == 0 ? 1 : 0);
  }

  // update ring coder display
  updateModeDisplay();




  // TEMP output
  setPixel(0, 255, 0, 255);
  FastLED.show();





  /*
  // Calculate FFT if a full sample is available.
  if (samplingIsDone()) {
    // Run FFT on sample data.
    arm_cfft_radix4_instance_f32 fft_inst;
    arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
    arm_cfft_radix4_f32(&fft_inst, samples);
    // Calculate magnitude of complex numbers output by the FFT.
    arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);

    // display
    sampleIntensity();

    // Restart audio sampling.
    samplingBegin();
  }

  // rotate hue
  // hue += 0.008;
  // if (hue > 360) hue -= 360;

  // updatePixleStates();
  */
}

// === MODES ===
void setMode(int newMode) {
  mode = newMode;
  if (mode == 0) {
    // BLUE mode - select animation
    ringcoder.setEncoderRange(ANIMATION_COUNT);
    ringcoder.writeEncoder(animationProgram / 2);
    ringcoder.setKnobRgb(0, 0, 255);
  }
  else if (mode == 1) {
    // GREEN mode - select brightness
    ringcoder.setEncoderRange(128);
    ringcoder.writeEncoder(brightness * 2);
    ringcoder.setKnobRgb(0, 255, 0);
  }
}

void updateModeDisplay() {
  int pos = ringcoder.readEncoder();
  if (mode == 0) {
    animationProgram = pos;
  } else {
    brightness = pos;
    FastLED.setBrightness(brightness);
  }
  ringcoder.ledRingFollower();
}




// === PIXELS ===
void clearPixels() {
  // clear out all buffers
  fill_solid(&(pixels[0]), PIXEL_COUNT, CRGB(0, 0, 0));
  FastLED.clear();
}

// skips fade - sets pixel buffer directly
void setPixel(int i, int r, int g, int b) {
  pixels[i].r = r;
  pixels[i].g = g;
  pixels[i].b = b;
}

/*
// hue is angle from 0 to 365
void setPixelHSV(int i, int h, byte s, byte v) {
  pixBuffer[i] = pixels[i] = CHSV(h, s, v);
}

// maps an angle (0 to inf) to a color using SIN and offsets
void setPixelAngle(int i, float angle) {
  // angles are in rads - distribute around 2*PI
  float red = 128.0 + 128.0 * sin(angle);
  float green = 128.0 + 128.0 * sin(angle + 2.1);
  float blue = 128.0 + 128.0 * sin(angle + 4.2);
  setPixel(i, red, green, blue);
}

void showStatus(int r, int g, int b) {
  // status is always 1/3 brightness level (255/3)
  float ang = 42 + (42 * sin(millis() / 1000.0));
  setAllPixels(r * ang, g * ang, b * ang);
}
*/






// === Audio FFT Sampling ===
void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate
  sampleCounter = 0;
  samplingTimer.begin(samplingCallback, 1000000/SAMPLE_RATE_HZ);
}

void samplingCallback() {
  // TODO: WTF is happening here!?!?!?

  // Interrupt everything to grab a sample - fill the buffer - and disable interrupts
  // Read from the ADC and store the sample data
  samples[sampleCounter] = (float32_t)analogRead(AUDIO_INPUT_PIN);
  // Complex FFT functions require a coefficient for the imaginary part of the input.
  // Since we only have real data, set this coefficient to zero.
  samples[sampleCounter+1] = 0.0;
  // Update sample buffer position and stop after the buffer is filled
  sampleCounter += 2;
  if (sampleCounter >= FFT_SIZE*2) {
    samplingTimer.end();
  }
}

boolean samplingIsDone() {
  return sampleCounter >= FFT_SIZE*2;
}

void sampleIntensity() {
  // TODO: WTF is happening here!?!?!?

/*
  float intensity, otherMean;
  // grab average of frequency band
  windowMean(magnitudes,
             frequencyToBin(BAND_START),
             frequencyToBin(BAND_END),
             &intensity,
             &otherMean);
  // convert intensity to decibels.
  intensity = 20.0*log10(intensity);
  // boost signals that stand out
  // otherMean = 20.0*log10(otherMean);
  // if (intensity > otherMean) {
  //   intensity += (intensity - otherMean) * BAND_ISOLATION;
  // }

  // map to intensity values (clamp 0-1)
  intensity = constrain(map(intensity, SPECTRUM_MIN_DB, SPECTRUM_MAX_DB, -0.2, 1.0*GAIN), 0.0, 1.0);
  // smooth out transitions (only in desc direction)
  value = intensity;
  // if (intensity < value)
  //   value = lerp(value, intensity, LERP_DOWN);
  // else
  //   value = lerp(value, intensity, LERP_UP);

  Serial.print(intensity);
  Serial.print(" :: ");
  Serial.println(value);

  //   // shift hue based on intensity (center on pure)
  //   float hue = hues[i];
  //   hue += (intensity-0.5)*HUE_SHIFT*2.0; // deg swing around center
  //   if (hue>360.0) hue -= 360.0;
  //   if (hue<0.0) hue += 360.0;
  //
  //   pixels.setPixelColor(i, pixelHSVtoRGBColor(hue, 1.0, intensities[i]));
  //   if (i == 0) {
  //     Serial.println(intensity);
  //   }
  //
  // }
  // pixels.show();
  */
}

// Compute the average magnitude of a target frequency window vs. all other frequencies.
void windowMean(float* magnitudes, int lowBin, int highBin, float* windowMean, float* otherMean) {
  *windowMean = 0;
  *otherMean = 0;
  // notice the first magnitude bin is skipped because it represents the
  // average power of the signal.
  for (int i = 1; i < FFT_SIZE/2; ++i) {
    if (i >= lowBin && i <= highBin) {
      *windowMean += magnitudes[i];
    } else {
      *otherMean += magnitudes[i];
    }
  }
  *windowMean /= (highBin - lowBin) + 1;
  *otherMean /= (FFT_SIZE / 2 - (highBin - lowBin));
}

// Convert a frequency to the appropriate FFT bin it will fall within.
int frequencyToBin(float frequency) {
  float binFrequency = float(SAMPLE_RATE_HZ) / float(FFT_SIZE);
  return int(frequency / binFrequency);
}


// === Utility Functions ===
float lerp(float start, float end, float percent) {
  return start + percent * (end - start);
}

// float capable MAP
float map(float value, float istart, float istop, float ostart, float ostop) {
  return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

int map(int value, int istart, int istop, int ostart, int ostop) {
  return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}
