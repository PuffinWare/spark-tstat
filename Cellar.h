#ifndef STAT_CELLAR_H
#define STAT_CELLAR_H

#include "common.h"
#include "HIH6130.h"
#include "DS2482.h"
#include "DS18B20.h"
#include "ButtonInterrupt.h"
#include "OledDisplay.h"
#include "Averager.h"

static const int NUM_SENSORS = 4;

// Two magic bytes and the actual value
static const byte CONFIG_MAGIC[] = {0x1e, 0xe7};
typedef struct {
  int version;    // config version
  int setTemp;
  int hysteresis; // +/- temp
} CELLAR_CONFIG;

typedef enum CELLAR_DISPLAY_MODE {
  CELLAR_NORMAL, // None specified
  CELLAR_SET_TEMP,
  CELLAR_STATS,
} CELLAR_DISPLAY_MODE;

class Cellar {

public:
  Cellar();
  void loop();

private:
  CELLAR_CONFIG config;
  bool enabled;
  int curRH;
  int curTemp[NUM_SENSORS];
  int avgTemp;   // the instant average of all the temp sensors
  Averager *histAvgTemp; // historical average over time

  ulong startTime;  //! Time the pump was turned on
  int curRunTime;  //! How long in the current state
  Averager *avgRunTime; //! Average run time for the pump
  Averager *avgIdleTime; //! Average run time for the pump

  uint drawMode;
  bool btnToggle;

  font_t* font_lcdSm;
  font_t* font_lcdLg;
  OledDisplay *display;
  HIH6130 *hih;
  DS2482 *ds2482;
  DS18B20 *ds18b20;
  ButtonInterrupt *btnHome;
  ButtonInterrupt *btnUp;
  ButtonInterrupt *btnDn;

  void writeConfig();
  void getTemp();
  void checkTemp();
  void enable();
  void disable();
  void drawTemp();
  void drawSetTemp(bool update);
  void handleButtonHome(int mode);
  void handleButtonUp(int mode);
  void handleButtonDn(int mode);
  void getDuration3char(int seconds, char *buff);
};

#endif //STAT_CELLAR_H
