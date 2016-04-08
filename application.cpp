
#include "application.h"
SYSTEM_MODE(MANUAL);

#include <spark_wiring_i2c.h>
#include <rgbled.h>

#include "common.h"
#include "cellar_util.h"
#include "Blinker.h"
#include "Cellar.h"

//#define PUFFIN_DEBUG true

Blinker *blinker;
Cellar *cellar;

void setup() {
#ifdef PUFFIN_DEBUG
  // Init USB Serial
  Serial.begin(9600);
#endif

  // Init SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);

  // Init I2C
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  // Testing LED
//  pinMode(D7, OUTPUT);
//  digitalWrite(D7, LOW);

  // Take over the RGB lED
  RGB.control(true);
  RGB.color(0, 0, 0);

  // Launch the cellar
  blinker = new Blinker(D7);
  cellar = new Cellar();

  blink(10, 200, 250);
}

void loop() {
  blinker->loop();
  cellar->loop();
}

void blink(int times, int onTime, int offTime) {
#ifdef PUFFIN_DEBUG
  Serial.println("Blink %d|%d|%d", times, onTime, offTime);
#endif
  blinker->trigger(times, onTime, offTime);
}