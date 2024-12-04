#pragma once
#include <Arduino.h>

static inline uint64_t millis64() {
    static uint32_t last = 0;
    static uint32_t upper = 0;
    uint64_t time = millis();
    if (time < last) {
        upper++;
    }
    last = time;
    return (((uint64_t) upper) << 32) | time;
}