
#include "application.h"
SYSTEM_MODE(MANUAL);

#include "rgbled.h"

#include "common.h"
#include "RHT03.h"
#include "ButtonInterrupt.h"
#include "OledDisplay.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"

// function prototypes
void drawPattern();
void drawText();
void drawTemp();
void drawSetTemp();
void getTemp();
void handleButtonHome(int mode);
void handleButtonUp(int mode);
void handleButtonDn(int mode);

#define DRAW_CUR_TEMP 1
#define DRAW_SET_TEMP 1
#define DRAW_MENU 2

#define PUFFIN_DEBUG true

bool btnHomeToggle = false;

int curTemp = 0;
int curRH = 0;
int setTemp = 72;
int setRH = 30;

uint drawMode = DRAW_CUR_TEMP;
uint textMode = 0;

OledDisplay *display;
RHT03 *rht;
ButtonInterrupt  *btnHome;
ButtonInterrupt  *btnUp;
ButtonInterrupt  *btnDn;

const font_t* font_lcdSm = parseFont(FONT_LCD6X8);
const font_t* font_lcdLg = parseFont(FONT_LCD11X16);

void setup() {
#ifdef PUFFIN_DEBUG
  Serial.begin(9600);
#endif

  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);

  RGB.control(true);
  RGB.color(0, 0, 0);

  display = new OledDisplay(D1, D2, D0);
  display->begin();

  rht = new RHT03(D3);

  btnHome = new ButtonInterrupt(D4, 100, &handleButtonHome, 2000, 250);
  btnUp = new ButtonInterrupt(D6, 100, &handleButtonUp, 2000, 250);
  btnDn = new ButtonInterrupt(D5, 100, &handleButtonDn, 2000, 250);
}

void loop() {
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

void initStat() {
}

void handleButtonHome(int mode) {
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

void handleButtonUp(int mode) {
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

void handleButtonDn(int mode) {
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

void getTemp() {
  curTemp = rht->getTempF();
  curRH = rht->getRH();
  // TODO: Send to server
}

void drawTemp() {
  // todo: do this only once
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

void drawSetTemp() {
  char tempStr[8];

  sprintf(tempStr, "%d", setTemp);
  display->writeText(8, 0, tempStr, 4);
  sprintf(tempStr, "%d", setRH);
  display->writeText(8, 2, tempStr, 4);

  display->display();
}
