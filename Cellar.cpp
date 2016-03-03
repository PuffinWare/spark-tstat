
#include "Cellar.h"
#include <spark_wiring.h>
#include <spark_wiring_eeprom.h>
#include <spark_wiring_usbserial.h>

#include "oled_font.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"
#include <functional>

#define CELLAR_DEBUG 1
#define AVERAGE_CNT 10

Cellar::Cellar() {
  this->enabled = false;
  this->curRH = 0;
  memset(curTemp, 0, sizeof(int)*NUM_SENSORS);
  this->curDuration = 0;
  this->startTime = millis();
  this->waitTime = startTime + 1500;
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
  curDuration = (millis() - startTime) / 1000;

  btnHome->poll();
  btnUp->poll();
  btnDn->poll();
  hih->poll();
  if (ds18b20->poll()) {
    getTemp();
    checkTemp();
    if (drawMode == CELLAR_MAIN) {
      updateNeeded = true;
    }
  }

  if (waitTime > 0 && (millis() > waitTime)) {
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

void Cellar::enable() {
  enabled = true;
  updateNeeded = true;
  digitalWrite(D7, HIGH);
  avgIdleTime->addValue(curDuration);
  startTime = millis();
}

void Cellar::disable() {
  enabled = false;
  updateNeeded = true;
  digitalWrite(D7, LOW);
  avgRunTime->addValue(curDuration);
  startTime = millis();
}

void Cellar::draw() {

  display->clear(CLEAR_BUFF);
  switch (drawMode) {
    case CELLAR_MAIN: drawMain(); break;
    case CELLAR_STATS: drawStats(); break;
    case CELLAR_STATS_IDLE: drawStatsLog(true); break;
    case CELLAR_STATS_RUN: drawStatsLog(false); break;
    case CELLAR_SET_TEMP: drawSetTemp(); break;
  }
  display->display();
}

void Cellar::drawMain() {
  char tempStr[10];
  int tVal;
  int tDec;

  // Current temp
  tVal = avgTemp / 10;
  tDec = avgTemp % 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d", tVal);
  display->writeText(0, 0, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", tDec);
  display->writeText(4, 1, tempStr);

  // Current RH
  tVal = curRH / 10;
  tDec = curRH % 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d", tVal);
  display->writeText(0, 1, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d%%", tDec);
  display->writeText(4, 3, tempStr);

  // Set temp
  tVal = config.setTemp / 10;
  tDec = config.setTemp % 10;
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d.%d", tVal, tDec);
  display->writeText(6, 0, tempStr, 4);

  // Individual sensors
  display->setFont(font_lcdSm);
  for (int i=0; i<NUM_SENSORS; i++) {
    tVal = curTemp[i] / 10;
    tDec = curTemp[i] % 10;

    sprintf(tempStr, "%d.%d", tVal, tDec);
    switch(i) {
      case 0:
        display->writeText(0, 4, tempStr);
      case 1:
        display->writeText(0, 5, tempStr);
      case 2:
        display->writeText(6, 4, tempStr, 4);
      case 3:
        display->writeText(6, 5, tempStr, 4);
    }
  }

  // Duration fields
  display->setFont(font_lcdSm);
  getDuration3char(curDuration, tempStr);
  display->writeText(7, 1, tempStr, 4, enabled);

  getDuration3char(avgIdleTime->getAverage(), tempStr);
  display->writeText(7, 2, tempStr, 4);

  getDuration3char(avgRunTime->getAverage(), tempStr);
  display->writeText(7, 3, tempStr, 4);
}

void Cellar::drawSetTemp() {
  char tempStr[8];
  int tVal;
  int tDec;

  display->setFont(font_lcdSm);
  display->writeText(0, 0, "SET", 0, true);

  // Current temp
  tVal = avgTemp / 10;
  tDec = avgTemp % 10;
  sprintf(tempStr, "%d.%d", tVal, tDec);
  display->writeText(6, 0, tempStr, 4);

  // Set Temp
  display->writeText(0, 2, "Temp:", 0, editMode==CELLAR_EDIT_TEMP);
  tVal = config.setTemp / 10;
  tDec = config.setTemp % 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%2d", tVal);
  display->writeText(3, 1, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", tDec);
  display->writeText(9, 3, tempStr, 3);

  // Set Hysteresis
  display->writeText(0, 4, "Hyst:", 0, editMode==CELLAR_EDIT_HYST);
  tVal = config.hysteresis / 10;
  tDec = config.hysteresis % 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%2d", tVal);
  display->writeText(3, 2, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", tDec);
  display->writeText(9, 5, tempStr, 3);
}

/*!
 * STATS
 *     RUN IDL
 * Cur     18m
 * Max 32m  4h
 * Avg 30m  2h
 * Lst 28s 45m
 */
void Cellar::drawStats() {
  char tempStr[4];

  display->setFont(font_lcdSm);
  display->writeText(0, 0, "STATS", 0, true);
  display->writeText(4, 1, "RUN");
  display->writeText(7, 1, "IDL", 4);

  display->writeText(0, 2, "Cur");
  getDuration3char(curDuration, tempStr);
  if (enabled) {
    display->writeText(4, 2, tempStr, 0, true);
  } else {
    display->writeText(7, 2, tempStr, 4, false);
  }

  display->writeText(0, 3, "Lst");
  getDuration3char(avgRunTime->getLast(), tempStr);
  display->writeText(4, 3, tempStr);
  getDuration3char(avgIdleTime->getLast(), tempStr);
  display->writeText(7, 3, tempStr, 4);

  display->writeText(0, 4, "Avg");
  getDuration3char(avgRunTime->getAverage(), tempStr);
  display->writeText(4, 4, tempStr);
  getDuration3char(avgIdleTime->getAverage(), tempStr);
  display->writeText(7, 4, tempStr, 4);

  display->writeText(0, 5, "Max");
  getDuration3char(avgRunTime->getMax(), tempStr);
  display->writeText(4, 5, tempStr);
  getDuration3char(avgIdleTime->getMax(), tempStr);
  display->writeText(7, 5, tempStr, 4);
}

/*!
 * IDLE  pg 1
 *  123  123
 *  123  123
 *  123  123
 *  123  123
 *  123  123
 */
void Cellar::drawStatsLog(bool idle) {
  char tempStr[4];
  Averager *source = idle ? avgIdleTime : avgRunTime;

  display->setFont(font_lcdSm);
  display->writeText(0, 0, idle?"IDLE":"RUN", 0, true);
  for (int i=0; i<2; i++) {
    for (int j=0; j<5; j++) {
      getDuration3char(source->getValue(i*5+j), tempStr);
      display->writeText(i*5 + 1, j+1, tempStr);
    }
  }
}

void Cellar::handleButtonHome(int mode) {
  switch(mode) {
    case UP:
      changeMode(10000, CELLAR_MAIN);
      break;

    case FIRST:
    case REPEAT:
      if (drawMode == CELLAR_SET_TEMP) {
        switch(editMode) {
          case CELLAR_EDIT_NONE: editMode=CELLAR_EDIT_TEMP; break;
          case CELLAR_EDIT_TEMP: editMode=CELLAR_EDIT_HYST; break;
          case CELLAR_EDIT_HYST: editMode=CELLAR_EDIT_NONE; break;
        }
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
      changeMode(10000, CELLAR_MAIN);
      break;

    case FIRST:
    case REPEAT:
      if (drawMode == CELLAR_SET_TEMP && editMode != CELLAR_EDIT_NONE) {
        switch(editMode) {
          case CELLAR_EDIT_TEMP: config.setTemp++; break;
          case CELLAR_EDIT_HYST: config.hysteresis++; break;
        }
        configChanged = true;
      } else {
        switch(drawMode) {
          case CELLAR_MAIN: changeMode(0, CELLAR_SET_TEMP); break;
          case CELLAR_STATS: changeMode(0, CELLAR_MAIN); break;
          case CELLAR_STATS_RUN: changeMode(0, CELLAR_STATS); break;
          case CELLAR_STATS_IDLE: changeMode(0, CELLAR_STATS_RUN); break;
          case CELLAR_SET_TEMP: changeMode(0, CELLAR_STATS_IDLE); break;
        }
      }

      updateNeeded = true;
      break;

    default:
      break;
  }
}

void Cellar::handleButtonDn(int mode) {

  switch(mode) {
    case UP:
      changeMode(10000, CELLAR_MAIN);
      break;

    case FIRST:
    case REPEAT:
      if (drawMode == CELLAR_SET_TEMP && editMode != CELLAR_EDIT_NONE) {
        switch(editMode) {
          case CELLAR_EDIT_TEMP: config.setTemp--; break;
          case CELLAR_EDIT_HYST: config.hysteresis--; break;
        }
        configChanged = true;
      } else {
        switch(drawMode) {
          case CELLAR_MAIN: changeMode(0, CELLAR_STATS); break;
          case CELLAR_STATS: changeMode(0, CELLAR_STATS_RUN); break;
          case CELLAR_STATS_RUN: changeMode(0, CELLAR_STATS_IDLE); break;
          case CELLAR_STATS_IDLE: changeMode(0, CELLAR_SET_TEMP); break;
          case CELLAR_SET_TEMP: changeMode(0, CELLAR_MAIN); break;
        }
      }

      updateNeeded = true;
      break;

    default:
      break;
  }
}

/*!
 * Returns a human readable duration in only 3 character.
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
void Cellar::getDuration3char(int seconds, char *buff) {
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
  sprintf(buff, pattern, reduced);
}