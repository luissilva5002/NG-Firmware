#pragma once
#include "ble/ble_protocol.h" // For DataPoint struct
#include "config/device_config.h"
#include <vector>

class RingBuffer {
public:
    RingBuffer() : head(0), full(false) {}

    void push(DataPoint dp) {
        buffer[head] = dp;
        head++;
        if (head >= BUFFER_LIMIT) {
            head = 0;
            full = true;
        }
    }

    // Unrolls the ring buffer into a flat vector (Oldest -> Newest)
    void unrollTo(std::vector<DataPoint>& target) {
        target.clear();
        target.reserve(BUFFER_LIMIT);

        if (full) {
            // Part 1: Head -> End (Oldest part)
            for (int i = head; i < BUFFER_LIMIT; i++) target.push_back(buffer[i]);
            // Part 2: 0 -> Head (Newer part)
            for (int i = 0; i < head; i++) target.push_back(buffer[i]);
        } else {
            // Buffer never filled, just 0 -> Head
            for (int i = 0; i < head; i++) target.push_back(buffer[i]);
        }
    }

    void reset() {
        head = 0;
        full = false;
    }

    bool isFull() const { return full; }

private:
    DataPoint buffer[BUFFER_LIMIT];
    int head;
    bool full;
};