
#include "application.h"
SYSTEM_MODE(MANUAL);

#include <spark_wiring_i2c.h>
#include <rgbled.h>

#include "common.h"
#include "Cellar.h"

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
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);

  // Take over the RGB lED
  RGB.control(true);
  RGB.color(0, 0, 0);

  // Launch the cellar
  cellar = new Cellar();
}

void loop() {
  cellar->loop();
}
