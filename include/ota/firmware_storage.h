#pragma once
#include <Arduino.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

class FirmwareStorage {
public:
    FirmwareStorage();
    
    // Prepare partition for writing
    bool start(size_t totalSize);
    
    // Write a chunk of data
    bool write(const uint8_t* data, size_t len);
    
    // Finalize, verify, and set boot partition
    bool complete();
    
    // cleanup on error
    void abort();

private:
    esp_ota_handle_t updateHandle;
    const esp_partition_t* updatePartition;
    bool isWriting;
};