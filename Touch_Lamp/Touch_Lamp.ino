#include "config.h"
#include "Adafruit_NeoPixel.h"
#include <CapacitiveSensor.h>

// pins for LED_OUTPUT
#define PIXEL_PIN     4 // D2
#define PIXEL_COUNT   24
#define PIXEL_TYPE    NEO_GRB + NEO_KHZ800

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// pins for CapSense
#define RECEIVE 12 //D6
#define SEND 0 //D3
#define ledPin 16 //on-board LED - low and high are switched
#define SENSOR_THRESHOLD 100 // sensor threshold

CapacitiveSensor cs = CapacitiveSensor(SEND, RECEIVE);

// set up feeds for color wheel and button
AdafruitIO_Feed *color = io.feed("color");
AdafruitIO_Feed *toggle = io.feed("toggle");
AdafruitIO_Feed *capsense = io.feed("capsense");
long ledColor = 0xFFFFFF; // color in hex
long prevSensVal = 0; // previous sensor value to compare
uint16_t seconds = 0; // time to tell when to turn lamp off again

void setup() {

  Serial.begin(115200);

  // wait for serial monitor to open
  while (! Serial);

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // set up a message handler for the 'color' feed.
  color->onMessage(handleColor);
  toggle->onMessage(handleToggle);

  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  color->get();
  toggle->get();
  capsense->get();

  // neopixel init
  pixels.begin();
  pixels.clear();

  // capSense init
  pinMode(16, OUTPUT);
  cs.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example

}

void loop() {

  // updates and gets values
  io.run();

  // if button is touched - send the result to 'toggle'
  long sensorVal =  cs.capacitiveSensor(250);
  //capsense->save(sensorVal); //  *** uncomment to debug touch sensor***
  
  if (sensorVal - prevSensVal > SENSOR_THRESHOLD) {
    toggle->save(1); // if touched, send 1 to toggle feed
    seconds = 0; // reset timer
    Serial.print("Sensor Value: ");
    Serial.print(sensorVal);
    Serial.print("\n");
  }
  
  prevSensVal = sensorVal;

  if (seconds > 300) {
    toggle->save(0); // after TIME seconds, send 0 to toggle feed
    seconds = 0;     // reset seconds
  }

  seconds++;   // increment seconds in order to inaccurately measure time
  delay(1000); // wait 1 seconds between each loop
}

// turns on the LEDs
void toggleLED() {
  Serial.print("Setting all pixels to ledColor");
  Serial.print("\n");
  //set all pixels to color
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    pixels.setPixelColor(i, ledColor);
  }
  pixels.show();
}

// used to switch colors if the color wheel is changed in ADAFRUIT IO
void handleColor(AdafruitIO_Data *hex) {
  Serial.print("Setting color to: ");
  Serial.print(hex->value());
  Serial.print("\n");
  ledColor = hex->toNeoPixel();
}

// used to handle changes to toggle feed
void handleToggle(AdafruitIO_Data *IO) {
  if (IO->toInt() == 0) {
    Serial.print("toggle is off");
    Serial.print("\n");
    pixels.clear();
    pixels.show();
  }
  else if (IO->toInt() == 1) {
    Serial.print("toggle is on");
    Serial.print("\n");
    toggleLED();
  }
  else {
    Serial.print(IO->value());
    Serial.print("\n");
  }
}
