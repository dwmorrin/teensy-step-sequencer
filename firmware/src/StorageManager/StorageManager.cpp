#include "StorageManager.h"
#include "Debug.h"

StorageManager::StorageManager() {}

bool StorageManager::init()
{
    LOGLN("SD_INIT: Attempting to mount SD Card...");
    if (!SD.begin(BUILTIN_SDCARD))
    {
        LOGLN("SD_INIT_ERROR: SD.begin() failed! Is the card inserted and formatted to FAT32?");
        _isInitialized = false;
        return false;
    }
    LOGLN("SD_INIT: Success.");
    _isInitialized = true;
    return true;
}

bool StorageManager::saveState(const SequencerModel &model)
{
    if (!_isInitialized)
    {
        LOGLN("SD_SAVE_ERROR: Aborted, SD card was never initialized.");
        return false;
    }

    if (SD.exists(FILENAME))
    {
        SD.remove(FILENAME);
    }

    File file = SD.open(FILENAME, FILE_WRITE);
    if (!file)
    {
        LOGLN("SD_SAVE_ERROR: Failed to open state.json for writing.");
        return false;
    }

    JsonDocument doc;
    model.serialize(doc);

    // Write to SD Card
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    // --- NEW: Print exact JSON output to Serial Monitor for debugging ---
    LOGLN("SD_SAVE: JSON Dump -> ");
    serializeJson(doc, Serial);
    LOGLN("\n-------------------------");
    // --------------------------------------------------------------------

    if (bytesWritten == 0)
    {
        LOGLN("SD_SAVE_ERROR: serializeJson returned 0 bytes written.");
        return false;
    }

    LOGLN("SD_SAVE: Success! Wrote %u bytes.", bytesWritten);
    return true;
}

bool StorageManager::loadState(SequencerModel &model)
{
    if (!_isInitialized)
    {
        LOGLN("SD_LOAD_ERROR: Aborted, SD card not initialized.");
        return false;
    }

    if (!SD.exists(FILENAME))
    {
        LOGLN("SD_LOAD: No state.json found. Starting fresh.");
        return false;
    }

    File file = SD.open(FILENAME, FILE_READ);
    if (!file)
    {
        LOGLN("SD_LOAD_ERROR: Failed to open state.json for reading.");
        return false;
    }

    LOGLN("SD_LOAD: Reading state.json (Size: %lu bytes)...", file.size());

    // --- NEW: Read entire file into a String to bypass SD stream delays ---
    String jsonStr = file.readString();
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error)
    {
        LOG("SD_LOAD_ERROR: Deserialize failed with code: ");
        LOGLN(error.c_str());
        return false;
    }

    LOGLN("SD_LOAD_DEBUG: Parsed JSON memory usage: %u bytes", doc.memoryUsage());
    LOGLN("SD_LOAD: Applying state to model...");

    model.deserialize(doc);

    LOGLN("SD_LOAD: Success!");
    return true;
}