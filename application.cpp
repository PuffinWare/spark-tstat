
#include "application.h"
SYSTEM_MODE(MANUAL);

#include "OledDisplay.h"
#include "RHT03.h"
#include "common.h"
#include "rgbled.h"

// function prototypes
void drawPattern();
void drawText();
void drawTemp();
void getTemp();

int btnUp = D6;
int btnDn = D5;
int btnHome = D4;

bool btnUpLatch = false;
bool btnDnLatch = false;
bool btnHomeLatch = false;

int tempSensor = A0;
int tempRhSensor = D3;
bool startup = true;

int reset = D1;
int cs = D0;
int dc = D2;

double reading = 0.0;
double volts = 0.0;
uint tempIdx = 0;
bool tempReady = false;
double temps[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
double avgTemp = 0.0;

int rhtTemp = 0;
int rhtRH = 0;

uint drawMode = 0;
uint textMode = 0;

unsigned long curTime;
unsigned long lastTime = 0;

OledDisplay display = OledDisplay(reset, dc, cs);;
RHT03 rht = RHT03(tempRhSensor, D7);

void setup() {
  pinMode(reset, OUTPUT);
  pinMode(dc, OUTPUT);
  pinMode(cs, OUTPUT);

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

  curTime = millis();
  // get temp once per second
  if (lastTime + 3000 < curTime) {
    lastTime = curTime;
    getTemp();
    if (tempReady) { // dont draw until we average a few readings
      drawTemp();
    }
  }

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
    // RGB.color(0,0,0);
  }

  if (digitalRead(btnDn) == LOW) {
    RGB.color(255,0,0);
    if (btnDnLatch) { // only draw once per press
      drawText();
      btnDnLatch = false;
    }
  } else {
    btnDnLatch = true;
    // RGB.color(0,0,0);
  }
}

void initStat() {
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
  display.setFont(textMode);
  switch (textMode) {
    case 0: // small
    case 1: // med
      RGB.color(textMode==0?255:0, textMode==0?0:255, 0);
      display.writeText(0, 0, "12345");
      display.writeText(1, 1, "67890");
      display.writeText(2, 2, "Hello");
      if (textMode == 0) display.writeText(3, 3, "World");
      break;
    case 2: // large
      RGB.color(0,0,255);
      display.writeText(0, 0, "12345");
      break;
  }

  textMode += 1;
  if (textMode > 2) {
    textMode = 0;
  }
}

void getTemp() {
  rht.update();
  RGB.color(255,255,0);

  reading = analogRead(tempSensor);
  volts = (reading * 3.3) / 4095;
  temps[tempIdx] = (volts - 0.5) * 100;
  tempIdx += 1;

  if (tempIdx == 5) {
    tempIdx = 0;
    tempReady = true;
  }

  if (!tempReady) return;

  double totTemp = 0.0;
  for(uint i=0; i<5; i++) {
    totTemp += temps[i];
  }
  avgTemp = totTemp/5;
}

void drawTemp() {
  //display.clear(CLEAR_OLED);
  display.setFont(0);

  double ftemp = avgTemp * 1.8 + 32;
  char tempStr[8];

  sprintf(tempStr, "%.1ff a", ftemp);
  display.writeText(1, 1, tempStr);

  ftemp = rht.getTemp();
  ftemp = ftemp * 0.18 + 32;
  sprintf(tempStr, "%.1ff d", ftemp);
  display.writeText(1, 2, tempStr);

  ftemp = rht.getRH();
  ftemp = ftemp / 10;
  sprintf(tempStr, "%.1f%%rh", ftemp);
  display.writeText(1, 3, tempStr);

  sprintf(tempStr, "%d cnt", rht.getIntCount());
  display.writeText(1, 4, tempStr);
  return;
}
