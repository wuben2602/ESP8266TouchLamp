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
const String toAdafruitColor = "#00FFFC"; // color for sending to Adafruit
const long userColor = 0x00fffc; // userColor 
long ledColor = userColor; // color in hex

uint8_t state = 0; // [sensorVal - prevSensVal]
long sensorVal = 0; // current sensor value
long prevSensVal = 0; // previous sensor value to compare
uint8_t state_buffer[3] = {0,0,0}; // buffer for states

uint16_t seconds = 0; // time to tell when to turn lamp off again
bool isOn = true;

void setup() {

  Serial.begin(115200);

  // wait for serial monitor to open
  while (!Serial);

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

  /**** GET INFORMATION ****/

  io.run();
  sensorVal =  cs.capacitiveSensor(250);
  //capsense->save(sensorVal); //  *** uncomment to debug touch sensor***
  
  /**** DETERMINE STATE ****/
  
  long changed = sensorVal - prevSensVal;

  if (changed < -SENSOR_THRESHOLD){
    state = 1; // went low 
  }
  else if (changed > SENSOR_THRESHOLD){
    state = 2; // went high
  }
  else{
    state = 0; // stayed same
  }

  // update array with state
  for(int i = 0; i < 2; i++){
    state_buffer[i] = state_buffer[i + 1];
  }
  state_buffer[2] = state;

  Serial.print("\n|");
  if (isOn){Serial.print("ON|");}
  else {Serial.print("OFF|");}
  for(int i = 0; i < 3; i++){
    Serial.print(state_buffer[i]);
    Serial.print("|");
  }
  Serial.print(prevSensVal);
  Serial.print("|");
  Serial.print(sensorVal);
  Serial.print("|");
  Serial.print(changed);
  Serial.print("|");
  Serial.print(seconds);
  Serial.print("|");
  
  /**** TOGGLE FRIENDSHIP LIGHTS ****/

  // [0/1 2 1] should toggle friendship lights with 1.5 sec delay
  if (isOn && state_buffer[0] != 2 && state_buffer[1] == 2 && state_buffer[2] == 1) {
    toggle->save(1); // if touched, send 1 to toggle feed
    color->save(toAdafruitColor); // if touched, send personal color to the color feed
    Serial.print("\n");
    Serial.print("Sensor Value: ");
    Serial.print(sensorVal);
    Serial.print("\n");
  }

  /**** TOGGLE LAMP ON/OFF ****/

  // covers constantly rising sensor values to constant sensor values
  if (state_buffer[0] == 2 && state_buffer[1] != 1 && state_buffer[2] != 1) {
    isOn = !isOn;
    if (!isOn){
      pixels.clear();
      pixels.show();
    }
    if (isOn){
      toggle->save(1);
      color->save(toAdafruitColor);
    }
  }

  /**** HANDLE OTHER ****/

  if (isOn) {
    if (seconds > 300) {
      toggle->save(0); // after TIME seconds, send 0 to toggle feed
    }
    seconds++;   // increment seconds in order to inaccurately measure time
  }

  prevSensVal = sensorVal;
  delay(750); // wait .75 seconds between each loop
}

/**** FUNCTIONS AND HANDLERS ****/

// turns on the LEDs
void toggleLED() {
  Serial.print("\nSetting all pixels to ledColor");
  Serial.print("\n");
  //set all pixels to color
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    pixels.setPixelColor(i, ledColor);
  }
  pixels.show();
}

// used to switch colors if the color wheel is changed in ADAFRUIT IO
void handleColor(AdafruitIO_Data *hex) {
//  Serial.print("Setting color to: ");
//  Serial.print(hex->value());
//  Serial.print("\n");
  ledColor = hex->toNeoPixel();
}

// used to handle changes to toggle feed
void handleToggle(AdafruitIO_Data *IO) {
  if (isOn) {
    if (IO->toInt() == 0) {
      Serial.print("toggle is off");
      Serial.print("\n");
      pixels.clear();
      pixels.show();
      seconds = 0; // reset timer
    }
    else if (IO->toInt() == 1) {
      Serial.print("toggle is on");
      Serial.print("\n");
      toggleLED();
      seconds = 0; // reset timer
    }
    else {
      Serial.print(IO->value());
      Serial.print("\n");
    }
  }
  else {
    Serial.print("Lamp is off!");
  }
}
