#pragma once

#include "afuue_common.h"
#include "menu.h"
#include "wave_generator.h"
#include "key_system.h"
#include "sensors.h"
#include "afuue_midi.h"

class Afuue {
public:
  Afuue()
    : canvas(&M5.Lcd)
    , afuueMidi()
    , menu(&canvas, &afuueMidi)
    , generator(&waveInfo)
  {

  }
  void Initialize();
  void Loop();

private:
  M5Canvas canvas;
  AfuueMIDI afuueMidi;
  Menu menu;

  WaveInfo waveInfo;
  WaveGenerator generator;
  KeySystem key;
  Sensors sensors;

  hw_timer_t * timer = NULL;
  QueueHandle_t xQueue;
  bool enablePlay = false;

  float maximumVolume = 0.0f;
  float targetNote = 60.0f;
  float currentNote = 60.0f;
  float startNote = 60.0f;
  float keyTimeMs = 0.0f;
  float keySenseTimeMs = 50.0f;
  int attackSoftness = 0;
  float attackNoiseLevel = 0.0f;
  const float ATTACKNOISETIME_LENGTH = 50.0f; //ms
  float noteOnTimeMs = 0.0f;
  float noteOnAccX, noteOnAccY, noteOnAccZ;
  float forcePlayTime = 0.0f;

  void GetMenuParams();

  void Update(float td);
  void Control(float td);
  void MenuExec();

  bool IsEnablePlay() const {
    return enablePlay;
  }
  Menu& GetMenu() {
    return menu;
  }
  KeySystem& GetKey() {
    return key;
  }
  Sensors& GetSensors() {
    return sensors;
  }
  WaveGenerator& GetGenerator() {
    return generator;
  }

  //static void SerialTask(void *pvParameters);
  static void UpdateTask(void *pvParameters);
  static void ControlTask(void *pvParameters);
  static void MenuTask(void *pvParameters);
  static void CreateWaveTask(void *pvParameters);
  static void IRAM_ATTR OnTimer();

};
