
#include "Cellar.h"
#include <spark_wiring.h>
#include <spark_wiring_eeprom.h>
#include <spark_wiring_usbserial.h>

#include "oled_font.h"
#include "font_lcd6x8.h"
#include "font_lcd11x16.h"
#include <functional>

//#define CELLAR_DEBUG 1

Cellar::Cellar() {
  this->enabled = false;
  this->curRH = 0;
  this->curRunTime = 0;
  this->startTime = millis();
  this->btnToggle = false;
  this->drawMode = CELLAR_NORMAL;
  this->histAvgTemp = new Averager(20);
  this->avgRunTime = new Averager(20);
  this->avgIdleTime = new Averager(20);

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

  // Setup config
  if (EEPROM.read(0) == CONFIG_MAGIC[0] &&
      EEPROM.read(1) == CONFIG_MAGIC[1]) {
    // valid eeprom data
#ifdef CELLAR_DEBUG
    Serial.printlnf("Load config");
#endif
    EEPROM.get(2, config);
  } else {
    // needs to be initialized
#ifdef CELLAR_DEBUG
    Serial.printlnf("Create config");
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
  curRunTime = (millis() - startTime) / 1000;

  btnHome->poll();
  btnUp->poll();
  btnDn->poll();
  hih->poll();
  if (ds18b20->poll()) {
    getTemp();
    checkTemp();
    if (drawMode == CELLAR_NORMAL) {
      drawTemp();
    }
  }
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
  digitalWrite(D7, HIGH);
  avgIdleTime->addValue(curRunTime);
  startTime = millis();
}

void Cellar::disable() {
  enabled = false;
  digitalWrite(D7, LOW);
  avgRunTime->addValue(curRunTime);
  startTime = millis();
}

void Cellar::drawTemp() {
  display->clear(CLEAR_BUFF);

  display->setFont(font_lcdLg);
  char tempStr[20];

  int ftemp = avgTemp / 10;
  int fdec = avgTemp % 10;

  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d.", ftemp);
  display->writeText(0, 0, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d", fdec);
  display->writeText(5, 1, tempStr);

  int rh = curRH / 10;
  int rhdec = curRH % 10;
  display->setFont(font_lcdLg);
  sprintf(tempStr, "%d.", rh);
  display->writeText(0, 1, tempStr);
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d%%", rhdec);
  display->writeText(5, 3, tempStr);

  drawSetTemp(false);

  display->setFont(font_lcdSm);
  for (int i=0; i<NUM_SENSORS; i++) {
    ftemp = curTemp[i];
    fdec = ftemp % 10;
    ftemp = ftemp / 10;

    sprintf(tempStr, "%d.%d", ftemp, fdec);
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
  getDuration3char(curRunTime, tempStr);
  display->writeText(7, 1, tempStr, 4, enabled);

  getDuration3char(avgIdleTime->getAverage(), tempStr);
  display->writeText(7, 2, tempStr, 4);

  getDuration3char(avgRunTime->getAverage(), tempStr);
  display->writeText(7, 3, tempStr, 4);

  display->display();
}

void Cellar::drawSetTemp(bool update) {
  char tempStr[8];
  int ftemp = config.setTemp / 10;
  int fdec = config.setTemp % 10;
  display->setFont(font_lcdSm);
  sprintf(tempStr, "%d.%d", ftemp, fdec);
  display->writeText(6, 0, tempStr, 4);

  if (update) {
    display->display();
  }
}

void Cellar::handleButtonHome(int mode) {
  switch(mode) {
    case UP:
      digitalWrite(D7, LOW);
      break;

    case FIRST:
      digitalWrite(D7, HIGH);
      btnToggle = true;
      ds18b20->update(0);
      break;

    case REPEAT:
      digitalWrite(D7, btnToggle ? HIGH : LOW);
      btnToggle = !btnToggle;
      break;

    default:
      break;
  }
}

void Cellar::handleButtonUp(int mode) {
  switch(mode) {
    case UP:
      writeConfig();
      break;

    case FIRST:
    case REPEAT:
      config.setTemp++;
      drawSetTemp(true);
      break;

    default:
      break;
  }
}

void Cellar::handleButtonDn(int mode) {
  switch(mode) {
    case UP:
      writeConfig();
      break;

    case FIRST:
    case REPEAT:
      config.setTemp--;
      drawSetTemp(true);
      break;

    default:
      break;
  }
}

/*!
 * Duration
 *  0s  59s  d < 60
 *  1m  99m  60 <= d < 6000
 * 100  999  6000 <= d < 60,000
 * 16h  99h  60,000 <= d 360,000
 *  4d  99d  360,000 <= d < 8,640,000
 * 14w  99w  8,640,000 <= d < 60,480,000
 *  1y  99y  60,480,000 <= d
 */
void Cellar::getDuration3char(int seconds, char *buff) {
  int reduced;
  if (seconds < 60) {
    sprintf(buff, "%2ds", seconds);
  } else if (seconds < 6000) {
    reduced = seconds / 60;
    sprintf(buff, "%2dm", reduced);
  } else if (seconds < 60000) {
    reduced = seconds / 60;
    sprintf(buff, "%3d", reduced);
  } else if (seconds < 360000) {
    reduced = seconds / 3600;
    sprintf(buff, "%2dh", reduced);
  } else if (seconds < 8640000) {
    reduced = seconds / 86400;
    sprintf(buff, "%2dd", reduced);
  } else if (seconds < 60480000) {
    reduced = seconds / 604800;
    sprintf(buff, "%2dw", reduced);
  } else {
    reduced = seconds / 31536000;
    sprintf(buff, "%2dy", reduced);
  }
}