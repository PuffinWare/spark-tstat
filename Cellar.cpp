
#include "Cellar.h"
#include <spark_wiring.h>
#include <spark_wiring_eeprom.h>
#include <spark_wiring_usbserial.h>

#include "oled_font.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"
#include <functional>

//#define CELLAR_DEBUG 1
#define VIEW_TIMEOUT 10000
#define AVERAGE_CNT     10
#define OUTPUT_LED      A0
#define OUTPUT_RELAY    D6
#define SPIN_DELAY     250 // 180deg spin per second
#define BREATHE_DELAY   32 // 32 ms, 8 point increment
#define BREATHE_STEP     8 // = ~32 updates per second

// Two magic bytes that would not be default values
static const byte CONFIG_MAGIC[] = { 0x1e, 0xe7 };
static char tempStr[8];
static char spinner[] = { '=', '*', '=', 0x00 };

Cellar::Cellar() {
  this->enabled = false;
  this->curRH = 0;
  memset(curTemp, 0, sizeof(int)*NUM_SENSORS);
  this->curDuration = 0;
  this->startTime = millis();
  this->waitTime = startTime + 1500;
  this->spinTime = startTime;
  this->breatheTime = startTime;
  this->breatheMode = 0;
  this->btnToggle = false;
  this->updateNeeded = false;
  this->configChanged = false;
  this->drawMode = CELLAR_MAIN;
  this->nextMode = CELLAR_MAIN;
  this->editMode = CELLAR_EDIT_NONE;
  this->histAvgTemp = new Averager(AVERAGE_CNT * 2);
  this->avgRunTime = new Averager(AVERAGE_CNT);
  this->avgIdleTime = new Averager(AVERAGE_CNT);

  this->font_lcdSm = parseFont(FONT_LCD6X8);
  this->font_lcdLg = parseFont(FONT_LCD11X16);

  this->display = new OledDisplay(A7, A6, A2);
  this->display->begin();

  // update every 500ms, adjust -0.5deg
  this->hih = new HIH6130(0x27, 500);

  this->ds2482 = new DS2482();
  this->ds18b20 = new DS18B20(this->ds2482, 500);

  using namespace std::placeholders;
  this->btnHome = new ButtonInterrupt(D4, 100, std::bind(&Cellar::handleButtonHome, this, _1), 2000, 250);
  this->btnUp = new ButtonInterrupt(D3, 100, std::bind(&Cellar::handleButtonUp, this, _1), 2000, 250);
  this->btnDn = new ButtonInterrupt(D2, 100, std::bind(&Cellar::handleButtonDn, this, _1), 2000, 250);

  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);
  pinMode(OUTPUT_LED, OUTPUT);
  digitalWrite(OUTPUT_LED, LOW);
  pinMode(OUTPUT_RELAY, OUTPUT);
  digitalWrite(OUTPUT_RELAY, LOW);

  // Setup config - check for magic bytes since it's not guaranteed to be zero at first
  if (EEPROM.read(0) == CONFIG_MAGIC[0] &&
      EEPROM.read(1) == CONFIG_MAGIC[1]) {
    // valid eeprom data
#ifdef CELLAR_DEBUG
    Serial.println("Load config");
#endif
    EEPROM.get(2, config);
  } else {
    // needs to be initialized
#ifdef CELLAR_DEBUG
    Serial.println("Create config");
#endif
    EEPROM.write(0, CONFIG_MAGIC[0]);
    EEPROM.write(1, CONFIG_MAGIC[1]);
    config.version = 1;
    config.setTemp = 600;
    config.hysteresis = 2;
    writeConfig();
  }
#ifdef CELLAR_DEBUG
  Serial.printlnf("config: %d|%d|%d", config.version, config.setTemp, config.hysteresis);
#endif
}

void Cellar::writeConfig() {
  EEPROM.put(2, config);
}

void Cellar::loop() {
  ulong now = millis();
  curDuration = (now - startTime) / 1000;

  btnHome->poll();
  btnUp->poll();
  btnDn->poll();
  hih->poll();
  if (ds18b20->poll()) {
    getTemp();
    checkTemp();
    updateNeeded = true;
  }

  if (waitTime > 0 && (now > waitTime)) {
    waitTime = 0;
    updateNeeded = true;
    drawMode = nextMode;
    editMode = CELLAR_EDIT_NONE;
    if (configChanged) {
      // Only want to write the config to EEPROM after all updates are done
      writeConfig();
      configChanged = false;
    }
  }

  if (updateNeeded) {
    updateNeeded = false;
    draw();
  }

  // These will update outside of the "updateNeeded" cycle
  if (enabled) {
    checkSpinner(now);
    checkBreatheLed(now);
  }
}

void Cellar::changeMode(ulong wait, CELLAR_DISPLAY_MODE next) {
  waitTime = millis() + wait;
  nextMode = next;
}

void Cellar::getTemp() {
// Don't trust the HIH temp anymore
//  curTemp = hih->getTempF();
  curRH = hih->getRH();

  // The curTemp is now the average of all four sensors
  avgTemp = 0;
  for (int i=0; i<NUM_SENSORS; i++) {
    curTemp[i] = ds18b20->getTempF(i);
    avgTemp += curTemp[i];
  }
  avgTemp = avgTemp / NUM_SENSORS;
  histAvgTemp->addValue(avgTemp);

#ifdef CELLAR_DEBUG
  Serial.printlnf("avg:%d", avgTemp);
#endif
}

void Cellar::checkTemp() {
  if (!histAvgTemp->isFull()) {
    return;
  }

  int avg = histAvgTemp->getAverage();
  if (enabled) {
    if (avg <= config.setTemp - config.hysteresis) {
      disable();
    }
  } else {
    if (avg >= config.setTemp + config.hysteresis) {
      enable();
    }
  }
}

void Cellar::checkSpinner(ulong now) {
  if (drawMode != CELLAR_MAIN || now < spinTime) {
    return;
  }

  spinTime = now + SPIN_DELAY;
  char spinChar = spinner[1];
  switch(spinChar) {
    case '|': spinChar = '\\'; break;
    case '\\': spinChar = '-'; break;
    case '-': spinChar = '/'; break;
    case '/': spinChar = '|'; break;
  }
  spinner[1] = spinChar;
  display->writeCharToDisplay(1, 4, spinChar);
}

void Cellar::checkBreatheLed(ulong now){
  if (now < breatheTime) {
    return;
  }

  breatheTime = now + BREATHE_DELAY;
  breatheMode += 8; // 32 steps between 0 and 255
  if (breatheMode > 255) {
    breatheMode = -255;
  }
  analogWrite(OUTPUT_LED, abs(breatheMode));
}

void Cellar::enable() {
  enabled = true;
  updateNeeded = true;
  avgIdleTime->addValue(curDuration);
  startTime = millis();
  spinTime = startTime + SPIN_DELAY;
  spinner[1] = '|';
  digitalWrite(OUTPUT_RELAY, HIGH);
  digitalWrite(D7, HIGH);
  breatheMode = 0;
}

void Cellar::disable() {
  enabled = false;
  updateNeeded = true;
  avgRunTime->addValue(curDuration);
  startTime = millis();
  spinner[1] = '*';
  digitalWrite(OUTPUT_RELAY, LOW);
  digitalWrite(OUTPUT_LED, LOW);
  digitalWrite(D7, LOW);
}

void Cellar::draw() {
  display->clear(CLEAR_BUFF);
  switch (drawMode) {
    case CELLAR_MAIN:       drawMain(); break;
    case CELLAR_TEMPS:      drawTemps(); break;
    case CELLAR_STATS:      drawStats(); break;
    case CELLAR_STATS_IDLE: drawStatsLog(true); break;
    case CELLAR_STATS_RUN:  drawStatsLog(false); break;
    case CELLAR_SET_TEMP:   drawSetTemp(); break;
  }
  display->display();
}

/*!
 * TEMP xxyy
 *      xxyyz
 * RH   jjkk
 *      jjkkl%
 * =*=
 * 12m   60.5
 */
void Cellar::drawMain() {
  // Current temp
  display->writeText(0, 0, "Temp:");
  drawReading(avgTemp, 3, 0);

  // Current RH
  display->writeText(2, 2, "RH:");
  drawReading(curRH, 3, 1);

  // Spinner
  display->setFont(font_lcdSm);
  display->writeText(0, 4, spinner);

  // Duration
  drawDuration3char(curDuration, 0, 5);

  // Set temp
  drawReadingSmall(config.setTemp, 6, 5, 4);
}

/*!
 * Max:  60.6
 * Min:  60.4
 * xxyy  rrss
 * xxyyz rrsst
 * jjkk  aabb
 * jjkkl aabbc
 */
void Cellar::drawTemps() {
  display->writeText(0, 0, "Max:");
  drawReadingSmall(histAvgTemp->getMax(), 6, 0);

  display->writeText(0, 1, "Min:");
  drawReadingSmall(histAvgTemp->getMin(), 6, 1);

  // Individual sensors
  for (int i=0; i<NUM_SENSORS; i++) {
    int x = (i/2) * 3;
    int y = i % 2 + 1;
    drawReading(curTemp[i], x, y);
  }
}

void Cellar::drawSetTemp() {
  display->setFont(font_lcdSm);
  display->writeText(0, 0, "SET", 0, 0, true);

  // Current temp
  drawReadingSmall(avgTemp, 6, 0, 4);

  // Set Temp
  display->writeText(0, 2, "Temp:", 0, 0, editMode==CELLAR_EDIT_TEMP);
  drawReading(config.setTemp, 3, 1);

  // Set Hysteresis
  display->writeText(0, 4, "Hyst:", 0, 0, editMode==CELLAR_EDIT_HYST);
  drawReading(config.hysteresis, 3, 2);
}

/*!
 * STATS
 *     RUN IDL
 * Cur     18m
 * Lst 28s 45m
 * Avg 30m  2h
 * Max 32m  4h
 */
void Cellar::drawStats() {
  display->setFont(font_lcdSm);
  display->writeText(0, 0, "STATS", 0, 0, true);
  display->writeText(4, 1, "RUN");
  display->writeText(7, 1, "IDL", 4);

  display->writeText(0, 2, "Cur");
  drawDuration3char(curDuration, enabled?4:7, 2, enabled?0:4, enabled);

  display->writeText(0, 3, "Lst");
  drawDuration3char(avgRunTime->getLast(), 4, 3);
  drawDuration3char(avgIdleTime->getLast(), 7, 3, 4);

  display->writeText(0, 4, "Avg");
  drawDuration3char(avgRunTime->getAverage(), 4, 4);
  drawDuration3char(avgIdleTime->getAverage(), 7, 4, 4);

  display->writeText(0, 5, "Max");
  drawDuration3char(avgRunTime->getMax(), 4, 5);
  drawDuration3char(avgIdleTime->getMax(), 7, 5, 4);
}

/*!
 * IDLE  cur
 *  123  123
 *  123  123
 *  123  123
 *  123  123
 *  123  123
 */
void Cellar::drawStatsLog(bool idle) {
  Averager *source = idle ? avgIdleTime : avgRunTime;

  display->setFont(font_lcdSm);
  display->writeText(0, 0, idle?"IDLE":"RUN", 0, 0, true);
  if ((!idle && enabled) || (idle && !enabled)) {
    drawDuration3char(curDuration, 6, 0);
  }
  for (int i=0; i<2; i++) {
    for (int j=0; j<5; j++) {
      drawDuration3char(source->getValue(i*5+j), (i*5)+1, j+1);
    }
  }
}

/*!
 * Draws a numeric reading with a large integer and small decimal number
 *  XXYY
 *  XXYYz
 */
void Cellar::drawReading(int value, int x, int y, int yOffset, bool invert) {
  // Need to calculate the position of the small number based on
  // the width of the large number and adjust offsets accordingly
  int z = ((x + 1) * font_lcdLg->width) + font_lcdLg->width + 2;
  int zx = z / font_lcdSm->width;
  int zo = (z % font_lcdSm->width) - 1;
  int zy = (y*2) + 1; // lg is 2 * sm, +1 so they align on the bottom

  display->setFont(font_lcdLg);
  sprintf(tempStr, "%2d", (value / 10));
  display->writeText(x, y, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", (value % 10));
  display->writeText(zx, zy, tempStr, zo);
}

/*!
 * Draws a small numeric reading to one decimal
 *  xx.y
 */
void Cellar::drawReadingSmall(int value, int x, int y, int xOffset, bool invert) {
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d.%d", (value / 10), (value % 10));
  display->writeText(x, y, tempStr, xOffset, 0, invert);
}

/*!
 * Draws a human readable duration in only 3 characters.
 * All have a suffix and two digits, except minutes which is
 * the most common result and can be three digits with no suffix.
 *
 *  0s - 59s | d < 60
 *  1m - 99m | 60 <= d < 6000
 * 100 - 999 | 6000 <= d < 60,000
 * 16h - 99h | 60,000 <= d 360,000
 *  4d - 99d | 360,000 <= d < 8,640,000
 * 14w - 99w | 8,640,000 <= d < 60,480,000
 *  1y - 99y | 60,480,000 <= d
 */
void Cellar::drawDuration3char(int seconds, int x, int y, int xOffset, bool invert) {
  int reduced;
  const char *pattern;
  if (seconds < 60) {
    reduced = seconds;
    pattern = "%2ds";
  } else if (seconds < 6000) {
    reduced = seconds / 60;
    pattern = "%2dm";
  } else if (seconds < 60000) {
    reduced = seconds / 60;
    pattern = "%3d";
  } else if (seconds < 360000) {
    reduced = seconds / 3600;
    pattern = "%2dh";
  } else if (seconds < 8640000) {
    reduced = seconds / 86400;
    pattern = "%2dd";
  } else if (seconds < 60480000) {
    reduced = seconds / 604800;
    pattern = "%2dw";
  } else {
    reduced = seconds / 31536000;
    pattern = "%2dy";
  }

  sprintf(tempStr, pattern, reduced);
  display->setFont(font_lcdSm);
  display->writeText(x, y, tempStr, xOffset, 0, invert);
}

void Cellar::handleButtonHome(int mode) {
  switch(mode) {
    case UP:
      changeMode(VIEW_TIMEOUT, CELLAR_MAIN);
      break;

    case FIRST:
      if (drawMode == CELLAR_SET_TEMP) {
        switch(editMode) {
          case CELLAR_EDIT_NONE: editMode=CELLAR_EDIT_TEMP; break;
          case CELLAR_EDIT_TEMP: editMode=CELLAR_EDIT_HYST; break;
          case CELLAR_EDIT_HYST: editMode=CELLAR_EDIT_NONE; break;
        }
      }
      updateNeeded = true;
      break;

    case REPEAT: // Need to hold to reset, repeat doesn't matter
      if (drawMode == CELLAR_STATS) {
        avgRunTime->reset();
        avgIdleTime->reset();
      } else if (drawMode == CELLAR_TEMPS) {
        histAvgTemp->reset();
      }
      updateNeeded = true;
      break;

    default:
      break;
  }
}

void Cellar::handleButtonUp(int mode) {
  switch(mode) {
    case UP:
      changeMode(VIEW_TIMEOUT, CELLAR_MAIN);
      break;

    case FIRST:
    case REPEAT:
      if (drawMode == CELLAR_SET_TEMP && editMode != CELLAR_EDIT_NONE) {
        switch(editMode) {
          case CELLAR_EDIT_TEMP: config.setTemp++; break;
          case CELLAR_EDIT_HYST: config.hysteresis++; break;
        }
        changeMode(VIEW_TIMEOUT, CELLAR_MAIN);
        configChanged = true;
      } else {
        switch(drawMode) {
          case CELLAR_MAIN: changeMode(0, CELLAR_SET_TEMP); break;
          case CELLAR_TEMPS: changeMode(0, CELLAR_MAIN); break;
          case CELLAR_STATS: changeMode(0, CELLAR_TEMPS); break;
          case CELLAR_STATS_RUN: changeMode(0, CELLAR_STATS); break;
          case CELLAR_STATS_IDLE: changeMode(0, CELLAR_STATS_RUN); break;
          case CELLAR_SET_TEMP: changeMode(0, CELLAR_STATS_IDLE); break;
        }
      }
      updateNeeded = true;
      break;
  }
}

void Cellar::handleButtonDn(int mode) {
  switch(mode) {
    case UP:
      changeMode(VIEW_TIMEOUT, CELLAR_MAIN);
      break;

    case FIRST:
    case REPEAT:
      if (drawMode == CELLAR_SET_TEMP && editMode != CELLAR_EDIT_NONE) {
        switch(editMode) {
          case CELLAR_EDIT_TEMP: config.setTemp--; break;
          case CELLAR_EDIT_HYST: config.hysteresis--; break;
        }
        changeMode(VIEW_TIMEOUT, CELLAR_MAIN);
        configChanged = true;
      } else {
        switch(drawMode) {
          case CELLAR_MAIN: changeMode(0, CELLAR_TEMPS); break;
          case CELLAR_TEMPS: changeMode(0, CELLAR_STATS); break;
          case CELLAR_STATS: changeMode(0, CELLAR_STATS_RUN); break;
          case CELLAR_STATS_RUN: changeMode(0, CELLAR_STATS_IDLE); break;
          case CELLAR_STATS_IDLE: changeMode(0, CELLAR_SET_TEMP); break;
          case CELLAR_SET_TEMP: changeMode(0, CELLAR_MAIN); break;
        }
      }
      updateNeeded = true;
      break;
  }
}
