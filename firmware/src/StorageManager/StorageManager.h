#pragma once
#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "Model/SequencerModel.h"

class StorageManager
{
public:
    StorageManager();

    bool init();
    bool saveState(const SequencerModel &model);
    bool loadState(SequencerModel &model);

private:
    const char *FILENAME = "state.json";
    bool _isInitialized = false;
};