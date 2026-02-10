#include "ota/firmware_storage.h"

FirmwareStorage::FirmwareStorage() : updateHandle(0), updatePartition(nullptr), isWriting(false) {}

bool FirmwareStorage::start(size_t totalSize) {
    printf("OTA: Storage Init. Target Size: %u\n", totalSize);

    // 1. Identify the next OTA partition
    updatePartition = esp_ota_get_next_update_partition(NULL);
    if (!updatePartition) {
        printf("OTA Error: No OTA partition found!\n");
        return false;
    }

    printf("OTA: Writing to partition subtype %d at offset 0x%x\n", 
           updatePartition->subtype, updatePartition->address);

    // 2. Begin OTA
    // OTA_SIZE_UNKNOWN allows streaming, or use totalSize if known strict
    esp_err_t err = esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &updateHandle);
    if (err != ESP_OK) {
        printf("OTA Error: esp_ota_begin failed (0x%x)\n", err);
        return false;
    }

    isWriting = true;
    return true;
}

bool FirmwareStorage::write(const uint8_t* data, size_t len) {
    if (!isWriting) return false;

    esp_err_t err = esp_ota_write(updateHandle, data, len);
    if (err != ESP_OK) {
        printf("OTA Error: Write failed (0x%x)\n", err);
        return false;
    }
    return true;
}

bool FirmwareStorage::complete() {
    if (!isWriting) return false;

    // 1. End OTA writing
    esp_err_t err = esp_ota_end(updateHandle);
    if (err != ESP_OK) {
        printf("OTA Error: End failed (0x%x)\n", err);
        return false;
    }

    // 2. Validate (Optional: Check image header)
    // ESP-IDF does basic validation in esp_ota_end

    // 3. Set Boot Partition
    err = esp_ota_set_boot_partition(updatePartition);
    if (err != ESP_OK) {
        printf("OTA Error: Set boot partition failed (0x%x)\n", err);
        return false;
    }

    printf("OTA: Success! Next boot will be from new partition.\n");
    isWriting = false;
    return true;
}

void FirmwareStorage::abort() {
    if (isWriting) {
        esp_ota_end(updateHandle); // Clean up handle
        isWriting = false;
        printf("OTA: Aborted.\n");
    }
}