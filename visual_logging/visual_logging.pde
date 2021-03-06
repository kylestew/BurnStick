import processing.serial.*;

Serial serial;

int bins = 64;
int spectroHeight = bins*2;
float magnitudes[] = new float[bins];
PImage spectrogram;

int pixelCount = 32;
int colors[] = new int[pixelCount*3];

void setup () {
  size(412, 700);
  //size(900, 700);
  println(Serial.list()[1]);
  serial = new Serial (this, Serial.list()[1], 115200);
  serial.bufferUntil('\n');
  
  spectrogram = createImage(bins, spectroHeight, RGB);
  
  noLoop();
}

int highIdx = 0;
void draw() {
  background(22);

  /*
  // move the pixels down a row each frame
  spectrogram.copy(0, 0, bins, spectroHeight-1, 0, 1, bins, spectroHeight-1); 
  
  // update image
  spectrogram.loadPixels();
  colorMode(HSB, 360, 100, 100);
  for (int i = 0; i < magnitudes.length && i < bins; i++) {
    // draw into spectrogram - first row of pixels
    if (magnitudes[0] > 1.0)
      spectrogram.pixels[i] = color(0, 100, 100);
    else
      spectrogram.pixels[i] = color(map(magnitudes[i], 0, 1, 240, 0), 100, 100);
  }
  spectrogram.updatePixels();
  
  image(spectrogram, 80, 32, bins*4, bins*8);
  */
  
  // display strand colors
  translate(32, 32);
  stroke(32);
  //colorMode(RGB, 16, 16, 16); // pop colors - I think they are low due to calibration
  float pixelHeight = 18;
  for (int i = 0; i < pixelCount*3; i+=3) {
    fill(colors[i], colors[i+1], colors[i+2]);
    
    rect(0, 0, 30, pixelHeight);
    rect(320, 0, 30, pixelHeight);
    translate(0, pixelHeight+2);
  }
  colorMode(RGB, 255, 255, 255);
}


void serialEvent(Serial serial) {
  String rawSampleString = serial.readStringUntil('\n');
  String[] values = split(rawSampleString, ' ');
  
  println(values);
  
  // first 64*3 values are LED colors, rest is FFT magnitudes
//  for (int i = 0; i < values.length && i < 64*3 + bins; i++) {
  //for (int i = 0; i < bins; i++) {
  for (int i = 0; i < pixelCount*3; i++) {
    //if (i < 64*3) {
      colors[i] = int(values[i]);
    //} else {
      //magnitudes[i-64*3] = float(values[i]);
    //}
    //magnitudes[i] = float(values[i]);
  }
  redraw();
}