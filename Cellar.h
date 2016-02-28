#ifndef STAT_CELLAR_H
#define STAT_CELLAR_H

#include "common.h"
#include "HIH6130.h"
#include "DS2482.h"
#include "DS18B20.h"
#include "ButtonInterrupt.h"
#include "OledDisplay.h"

class Cellar {

public:
  Cellar();
  void loop();

private:
  int curTemp;
  int curRH;
  int setTemp;
  int setRH;
  uint drawMode;
  bool btnHomeToggle;

  font_t* font_lcdSm;
  font_t* font_lcdLg;
  OledDisplay *display;
  HIH6130 *hih;
  DS2482 *ds2482;
  DS18B20 *ds18b20;
  ButtonInterrupt *btnHome;
  ButtonInterrupt *btnUp;
  ButtonInterrupt *btnDn;

  void drawPattern();
  void drawText();
  void drawTemp();
  void drawSetTemp();
  void drawHihTemp();
  void drawAltTemp();
  void getTemp();
  void handleButtonHome(int mode);
  void handleButtonUp(int mode);
  void handleButtonDn(int mode);
};

#endif //STAT_CELLAR_H
