
#include "application.h"
SYSTEM_MODE(MANUAL);

#include "rgbled.h"

#include "common.h"
#include "Cellar.h"

Cellar *cellar;

void setup() {
#ifdef PUFFIN_DEBUG
  Serial.begin(9600);
#endif

  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);

  RGB.control(true);
  RGB.color(0, 0, 0);

  cellar = new Cellar();
}

void loop() {
  cellar->loop();
}
