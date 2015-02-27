
#include "application.h"
SYSTEM_MODE(MANUAL);

#include "rgbled.h"

#include "RHT03.h"
#include "TMP3x.h"
#include "OledDisplay.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"
#include "font_12x16_bold.h"
#include "common.h"

// function prototypes
void drawPattern();
void drawText();
void drawTemp();
void getTemp();

int btnUp = D6;
int btnDn = D5;
int btnHome = D3;

bool btnUpLatch = false;
bool btnDnLatch = false;
bool btnHomeLatch = false;

double avgTemp = 0.0;

uint drawMode = 0;
uint textMode = 0;

OledDisplay display = OledDisplay(D1, D2, D0);;
RHT03 rht = RHT03(D4, D7);
TMP3x tmp36 = TMP3x(A0, 10, 1000);

const font_t* font_lcdSm = parseFont(FONT_LCD6X8);
const font_t* font_lcdLg = parseFont(FONT_LCD11X16);
const font_t* font_bold  = parseFont(FONT_12X16_BOLD);

void setup() {

  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDn, INPUT_PULLUP);
  pinMode(btnHome, INPUT_PULLUP);

  // Spark.variable("reading", &reading, DOUBLE);
  // Spark.variable("volts", &volts, DOUBLE);
  // Spark.variable("temp", &avgTemp, DOUBLE);

  RGB.control(true);
  RGB.color(0, 0, 0);

  display.begin();
}

void loop() {
  tmp36.poll();
  if (rht.poll()) {
    drawTemp();
  }

  // curTime = millis();
  // // get temp once per second
  // if (lastTime + 3000 < curTime) {
  //   lastTime = curTime;
  //   display.clear(CLEAR_OLED);
  //   drawTemp();
  // }

  if (digitalRead(btnHome) == LOW) {
    if (btnHomeLatch) { // only draw once per press
      drawPattern();
      btnHomeLatch = false;
    }
  } else {
    btnHomeLatch = true;
  }

  if (digitalRead(btnUp) == LOW) {
    RGB.color(0,0,255);
    if (btnUpLatch) { // only draw once per press
      drawTemp();
      btnUpLatch = false;
    }
  } else {
    btnUpLatch = true;
  }

  if (digitalRead(btnDn) == LOW) {
    RGB.color(255,0,0);
    if (btnDnLatch) { // only draw once per press
      drawText();
      btnDnLatch = false;
    }
  } else {
    btnDnLatch = true;
  }
}

void initStat() {
}

void drawTemp() {
  display.clear(CLEAR_OLED);
  display.setFont(font_lcdLg);
  char tempStr[8];

  double ftemp = tmp36.getTempF();
  ftemp = (ftemp / 10);

  sprintf(tempStr, "%.1f\x7f", ftemp);
  display.writeText(0, 0, tempStr);

  ftemp = rht.getTempF();
  ftemp = ftemp / 10;
  sprintf(tempStr, "%.1f\x7f", ftemp);
  display.writeText(0, 1, tempStr);

  ftemp = rht.getRH();
  ftemp = ftemp / 10;
  sprintf(tempStr, "%.1f%%", ftemp);
  display.writeText(0, 2, tempStr);

  // sprintf(tempStr, "%d.%d  ", rht.getIntCount(), rht.getIgnCount());
  // display.writeText(1, 4, tempStr);
  display.display();
}

void drawPattern() {
  display.resetPage();

  switch (drawMode) {
    case 0: // blank page
      RGB.color(255,0,0);
      display.clear(CLEAR_BUFF);
      break;
    case 1: // horizontal lines
      RGB.color(0,255,0);
      display.fill(0xaa);
      break;
    case 2: // vertical lines
      RGB.color(0,0,255);
      for (int i=0; i<6; i++) {
        for (int j=0; j<32; j++) {
          int c = j*2;
          display.setByte(i, c, 0xff);
          display.setByte(i, c+1, 0x00);
        }
      }
      break;
    case 3: {// count
      RGB.color(255,255,0);
      byte val = 0x00;
      for (int i=0; i<6; i++) {
        for (int j=0; j<64; j++) {
          display.setByte(i, j, val++);
          if (val > 0xff) val = 0x00;
        }
      }
      break;
    }
    case 4: // diagnal lines
      RGB.color(255,0,255);
      for (int i=0; i<6; i++) {
        for (int j=0; j<8; j++) {
          int c = j*8;
          display.setByte(i, c,   0x01);
          display.setByte(i, c+1, 0x02);
          display.setByte(i, c+2, 0x04);
          display.setByte(i, c+3, 0x08);
          display.setByte(i, c+4, 0x10);
          display.setByte(i, c+5, 0x20);
          display.setByte(i, c+6, 0x40);
          display.setByte(i, c+7, 0x80);
        }
      }
      break;
    case 5: // progression
      RGB.color(0,255,255);
      for (int i=0; i<6; i++) {
        for (int j=0; j<8; j++) {
          int c = j*8;
          display.setByte(i, c,   0x01);
          display.setByte(i, c+1, 0x03);
          display.setByte(i, c+2, 0x07);
          display.setByte(i, c+3, 0x0f);
          display.setByte(i, c+4, 0x1f);
          display.setByte(i, c+5, 0x3f);
          display.setByte(i, c+6, 0x7f);
          display.setByte(i, c+7, 0xff);
        }
      }
      break;
  }
  display.display();

  drawMode += 1;
  if (drawMode > 5) {
      drawMode = 0;
  }
}

void drawText() {
  display.clear(CLEAR_OLED);
  // display.setFont(textMode);
  switch (textMode) {
    case 0: // small
    case 1: // med
    case 2: // bold
      // RGB.color(textMode==0?255:0, textMode==0?0:255, 0);
      display.writeText(0, 0, "12345");
      display.writeText(0, 1, "Hello");
      display.writeText(0, 2, "World");
      break;
    case 3: // large
    case 4:
    case 5:
      // RGB.color(0,0,255);
      display.writeText(0, 0, "12345");
      if (textMode < 5) {
        display.writeText(0, 1, "67890");
      }
      break;
  }

  textMode += 1;
  if (textMode > 5) {
    textMode = 0;
  }
}
