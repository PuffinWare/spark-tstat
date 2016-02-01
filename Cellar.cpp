
#include "Cellar.h"
#include <spark_wiring.h>
#include "oled_font.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"
#include <functional>

#define DRAW_CUR_TEMP 1
#define DRAW_SET_TEMP 1
#define DRAW_MENU 2

Cellar::Cellar() {

  this->curTemp = 0;
  this->curRH = 0;
  this->setTemp = 72;
  this->setRH = 30;
  this->btnHomeToggle = false;
  this->drawMode = DRAW_CUR_TEMP;

  this->font_lcdSm = parseFont(FONT_LCD6X8);
  this->font_lcdLg = parseFont(FONT_LCD11X16);

  this->display = new OledDisplay(D1, D2, D0);
  this->display->begin();

  this->rht = new RHT03(D3);

  using namespace std::placeholders;
  this->btnHome = new ButtonInterrupt(D4, 100, std::bind(&Cellar::handleButtonHome, this, _1), 2000, 250);
  this->btnUp = new ButtonInterrupt(D6, 100, std::bind(&Cellar::handleButtonUp, this, _1), 2000, 250);
  this->btnDn = new ButtonInterrupt(D5, 100, std::bind(&Cellar::handleButtonDn, this, _1), 2000, 250);
}

void Cellar::loop() {
  if (rht->poll()) {
    getTemp();
    if (drawMode == DRAW_CUR_TEMP) {
      drawTemp();
    }
  }

  btnHome->poll();
  btnUp->poll();
  btnDn->poll();
}

void Cellar::getTemp() {
  curTemp = rht->getTempF();
  curRH = rht->getRH();
}


void Cellar::drawTemp() {
  display->clear(CLEAR_BUFF);

  display->setFont(font_lcdLg);
  char tempStr[8];

  int ftemp = curTemp;
  int fdec = ftemp % 10;
  ftemp = ftemp / 10;

  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d.", ftemp);
  display->writeText(0, 0, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", fdec);
  display->writeText(5, 1, tempStr);

  int rh = curRH;
  int rhdec = rh % 10;
  rh = rh / 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d.", rh);
  display->writeText(0, 1, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d%%", rhdec);
  display->writeText(5, 3, tempStr);

  display->setFont(font_lcdSm);
  const char *curDate = "10/14";
  const char *curTime = "12:34";
  display->writeText(0, 5, curDate);
  display->writeText(6, 5, curTime, -3);

  sprintf(tempStr, "%d", setTemp);
  display->writeText(8, 0, tempStr, 4);
  sprintf(tempStr, "%d", setRH);
  display->writeText(8, 2, tempStr, 4);

  display->display();
}

void Cellar::drawSetTemp() {
  char tempStr[8];

  sprintf(tempStr, "%d", setTemp);
  display->writeText(8, 0, tempStr, 4);
  sprintf(tempStr, "%d", setRH);
  display->writeText(8, 2, tempStr, 4);

  display->display();
}

void Cellar::handleButtonHome(int mode) {
  switch(mode) {
    case UP:
      digitalWrite(D7, LOW);
      break;

    case FIRST:
      digitalWrite(D7, HIGH);
      btnHomeToggle = true;
      break;

    case REPEAT:
      digitalWrite(D7, btnHomeToggle ? HIGH : LOW);
      btnHomeToggle = !btnHomeToggle;
      break;

    default:
      break;
  }
}

void Cellar::handleButtonUp(int mode) {
  switch(mode) {
    case FIRST:
    case REPEAT:
      setTemp++;
      drawSetTemp();
      break;

    default:
      break;
  }
}

void Cellar::handleButtonDn(int mode) {
  switch(mode) {
    case FIRST:
    case REPEAT:
      setTemp--;
      drawSetTemp();
      break;

    default:
      break;
  }
}