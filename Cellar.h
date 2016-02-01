#ifndef STAT_CELLAR_H
#define STAT_CELLAR_H

#include "common.h"
#include "RHT03.h"
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
  RHT03 *rht;
  ButtonInterrupt *btnHome;
  ButtonInterrupt *btnUp;
  ButtonInterrupt *btnDn;

  void drawPattern();
  void drawText();
  void drawTemp();
  void drawSetTemp();
  void getTemp();
  void handleButtonHome(int mode);
  void handleButtonUp(int mode);
  void handleButtonDn(int mode);
};

#endif //STAT_CELLAR_H
