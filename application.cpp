
#include "application.h"
SYSTEM_MODE(MANUAL);

#include "OledDisplay.h"
#include "rgbled.h"

// function prototypes
void drawPattern();
void drawText();
void drawTemp();
void getTemp();

// Define the pins we're going to call pinMode on
int ledRed = D6;
int ledBlue = D7;
int btnUp = D5;
int btnDn = D4;
int btnPad = D3;

bool btnUpLatch = false;
bool btnDnLatch = false;
bool btnPadLatch = false;

int tmpSensor = A0;
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

uint drawMode = 0;
uint textMode = 0;

unsigned long curTime;
unsigned long lastTime = 0;

OledDisplay display = OledDisplay(reset, dc, cs);;

void setup() {
  pinMode(ledRed, OUTPUT);
  pinMode(ledBlue, OUTPUT);

  digitalWrite(ledRed, LOW);
  digitalWrite(ledBlue, LOW);

  pinMode(reset, OUTPUT);
  pinMode(dc, OUTPUT);
  pinMode(cs, OUTPUT);

  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDn, INPUT_PULLUP);
  pinMode(btnPad, INPUT_PULLUP);

  Spark.variable("reading", &reading, DOUBLE);
  Spark.variable("volts", &volts, DOUBLE);
  Spark.variable("temp", &avgTemp, DOUBLE);

  RGB.control(true);
  RGB.color(0, 0, 0);

  display.begin();
}

void loop() {

  curTime = millis();
  // get temp once per second
  if (lastTime + 1000 < curTime) {
    lastTime = curTime;
    getTemp();
    if (tempReady) { // dont draw until we average a few readings
      RGB.color(255,0,50*tempIdx);
      drawTemp();
    }
  }

  if (digitalRead(btnPad) == LOW) {
    if (btnPadLatch) { // only draw once per press
      drawPattern();
      btnPadLatch = false;
    }
  } else {
      btnPadLatch = true;
  }

  if (digitalRead(btnUp) == LOW) {
    digitalWrite(ledBlue, HIGH);
    if (btnUpLatch) { // only draw once per press
      drawTemp();
      btnUpLatch = false;
    }
  } else {
    btnUpLatch = true;
    digitalWrite(ledBlue, LOW);
  }

  if (digitalRead(btnDn) == LOW) {
    digitalWrite(ledRed, HIGH);
    if (btnDnLatch) { // only draw once per press
      drawText();
      btnDnLatch = false;
    }
  } else {
    btnDnLatch = true;
    digitalWrite(ledRed, LOW);
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
  reading = analogRead(tmpSensor);
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
  display.clear(CLEAR_OLED);
  display.setFont(1);
  double ftemp = avgTemp * 1.8 + 32;
  char tempStr[8];
  sprintf(tempStr, "%.1ff", ftemp);
  display.writeText(1, 1, tempStr);
  sprintf(tempStr, "%.1fc", avgTemp);
  display.writeText(1, 2, tempStr);
  return;
}
