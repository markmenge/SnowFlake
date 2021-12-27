// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library
#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <Adafruit_NeoPixel.h>

extern void loopOTA();

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN D4
#define NUMPIXELS 150

// How many NeoPixels are attached to the Arduino?
int brightness = 30;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 500; // delay for half a second

// UDP stuff
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
WiFiUDP Udp;
extern void setupOTA();
struct Spark
{
  int led, led_brightness, bRampingUp = true;
};
int lasttime_sparkle;
#define NUM_SPARKS 10
Spark sparkles[NUM_SPARKS];

void setup()
{
  Serial.begin(115200);
  setupOTA();
  WiFi.printDiag(Serial);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Udp.begin(8888);
  strip.begin(); // This initializes the NeoPixel library.
  changeColor(strip.Color(brightness, brightness, brightness));
}

int count;
int red, green, blue = 50;

int current_state = 's';
int packetSize;
void loop()
{
  int i;
  packetSize = Udp.parsePacket();
  if (packetSize > 0)
  {
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[packetSize] = 0;
    uint8_t character = packetBuffer[0];
    //  Serial.println("Contents:");
    //  Serial.println(character);

    switch (character)
    {
    case 'a':
      colorWipe(strip.Color(brightness, 0, 0), 20); // Red
      break;
    case 'b':
      colorWipe(strip.Color(0, brightness, 0), 20); // Green
      break;
    case 'c':
      colorWipe(strip.Color(0, 0, brightness), 20); // Blue
      break;
    case 'd': // theaterChase red
    case 'e': // theaterChase green
    case 'f': // theaterChase yellow
    case 'g': // rainbowCycle
    case 'h': // rainbow
    case 'i': // theaterChaseRainbow long

    case 'm': // maize and blue fade
    case '2': // two step maize and blue
    case '4': // four step maize and blue
      break;
    case 's': // sparkle command just received
      brightness = 3;
      changeColor(strip.Color(brightness, brightness, brightness));
      break;
    case '0': // black
      colorWipe(strip.Color(0, 0, 0), 0);

      break;
    case 'l':
      brightness = atoi(packetBuffer + 1);
      character = current_state;
      break;
    case '6':
      fill();
      break;
    case 'u':
      for (i = 0; i < 300; i++)
        strip.setPixelColor(i, packetBuffer[i * 3 + 1], packetBuffer[i * 3 + 2], packetBuffer[i * 3 + 3]);
      strip.show();
    }
    current_state = character;
  }
  switch (current_state)
  {
  case 'd':
    theaterChase(strip.Color(brightness, 0, 0), 40); // Red
    break;
  case 'e':
    theaterChase(strip.Color(0, brightness, 0), 40); // Green
    break;
  case 'f':
    theaterChase(strip.Color(brightness, brightness, 0), 40); // yellow
    break;
  case 'g':
    rainbowCycle(2);
    break;
  case 'h':
    rainbow(20);
    break;
  case 'i':
    theaterChaseRainbow(20);
    break;
  case 'm':
    fade();
    break;
  case '2':
    twostep(strip.Color(0, 0, brightness), strip.Color(brightness, brightness * 2 / 3, 0), 999);
    break;
  case 'r': // red and green
    twostep(strip.Color(brightness, 0, 0), strip.Color(0, brightness, 0), 999);
    break;
  case 't': // red and white
    twostep(strip.Color(brightness, 0, 0), strip.Color(brightness, brightness, brightness), 999);
    break;
  case '4':
    colorstep(strip.Color(0, 0, brightness), strip.Color(brightness, brightness * 2 / 3, 0), 4, 999);
    break;
  case '5':
  {
    static int stepsize;
    stepsize++;
    if (stepsize > 100)
      stepsize = 1;
    colorstep(strip.Color(brightness, 0, 0), strip.Color(0, brightness, 0), stepsize, 999);
  }
  break;
  case '6':
    fill();
    break;
  case 's': // sparkle
    sparkle();
    break;
  default:
    break;
  }
  loopOTA();
}

void changeColor(uint32_t c)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    delay(wait);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++)
  { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait)
{
  for (int j = 0; j < 10; j++)
  { // do 10 cycles of chasing
    for (int q = 0; q < 3; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, c); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait)
{
  for (int j = 0; j < 256; j++)
  { // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, Wheel((i + j) % 255)); // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3)
      {
        strip.setPixelColor(i + q, 0); // turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color((255 - WheelPos * 3) / 10, 0, (WheelPos * 3) / 10);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, (WheelPos * 3) / 10, (255 - WheelPos * 3) / 10);
  }
  WheelPos -= 170;
  return strip.Color((WheelPos * 3) / 10, (255 - WheelPos * 3) / 10, 0);
}

void fade()
{
  for (int i = 3; i < 18; i++)
  {
    for (uint16_t j = 0; j < strip.numPixels(); j++)
    {
      int blue = i < 10 ? i : 20 - i;
      strip.setPixelColor(j, strip.Color(0, 0, blue));
    }
    strip.show();
    delay(100);
  }
  // yellow
  for (int i = 3; i < 18; i++)
  {
    for (uint16_t j = 0; j < strip.numPixels(); j++)
    {
      int yellow = i < 10 ? i : 20 - i;
      strip.setPixelColor(j, strip.Color(yellow, yellow * 3 / 4, 0));
    }
    strip.show();
    delay(100);
  }
}

void twostep(uint32_t c1, uint32_t c2, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i = i + 2)
  {
    strip.setPixelColor(i, c1);
    strip.setPixelColor(i + 1, c2);
  }
  strip.show();
  delay(wait);
  for (uint16_t i = 0; i < strip.numPixels(); i = i + 2)
  {
    strip.setPixelColor(i, c2);
    strip.setPixelColor(i + 1, c1);
  }
  strip.show();
  delay(wait);
}

void colorstep(uint32_t c1, uint32_t c2, int groupsize, uint8_t wait)
{
  uint16_t i, j;
  for (i = 0; i < strip.numPixels(); i = i + groupsize * 2)
  {
    for (j = 0; j < groupsize; j++)
    {
      strip.setPixelColor(i + j, c1);
      strip.setPixelColor(i + groupsize + j, c2);
    }
  }
  strip.show();
  delay(wait);
  for (i = 0; i < strip.numPixels(); i = i + groupsize * 2)
  {
    for (j = 0; j < groupsize; j++)
    {
      strip.setPixelColor(i + j, c2);
      strip.setPixelColor(i + groupsize + j, c1);
    }
  }
  strip.show();
  delay(wait);
}
int ledcount[] = {32, 25, 24, 19};
int level;
int ledinring;
void fill()
{
  int i, j, ring;

  level -= 10;
  if (level < -50)
    level = 50;

  for (i = 0; i < strip.numPixels(); i++)
  {
    int ledinring = i;
    for (j = 0; j < 4; j++)
    {
      if (ledinring < ledcount[j])
        break;
      ledinring = ledinring - ledcount[j];
    }
    ring = 5 - j;
    long y = cos(ledinring * (6.28 / ledcount[j])) * 10 * ring;

    Serial.print(i);
    Serial.print(" ");
    Serial.print(ledinring);
    Serial.print(" ");
    Serial.println(y);

    if (y > level)
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    else
      strip.setPixelColor(i, strip.Color(0, 0, brightness));
  }
  strip.show();
  delay(100);
}

void sparkle()
{
  int i;
  static bool bFirstTime = true;
  if (labs(millis() - lasttime_sparkle) < 20)
    return;

  if (bFirstTime)
  {
    bFirstTime = false;

    for (i = 0; i < NUM_SPARKS; i++)
    {
      sparkles[i].led = rand() % 50;
      sparkles[i].led_brightness = rand() % 200;
      sparkles[i].bRampingUp = rand() % 2;
    }
  }

  lasttime_sparkle = millis();
  for (i = 0; i < NUM_SPARKS; i++)
  {
    if (sparkles[i].bRampingUp)
    {
      sparkles[i].led_brightness += 5;
      if (sparkles[i].led_brightness > 200)
        sparkles[i].bRampingUp = false;
      strip.setPixelColor(sparkles[i].led, strip.Color(sparkles[i].led_brightness, sparkles[i].led_brightness, sparkles[i].led_brightness));
    }
    else
    {
      sparkles[i].led_brightness -= 5;
      strip.setPixelColor(sparkles[i].led, strip.Color(sparkles[i].led_brightness, sparkles[i].led_brightness, sparkles[i].led_brightness));
      if (sparkles[i].led_brightness <= brightness)
      {
        strip.setPixelColor(sparkles[i].led, strip.Color(brightness, brightness, brightness));
        sparkles[i].led = rand() % 150; // start over
        sparkles[i].bRampingUp = true;
      }
    }
  }
  strip.show();
}
