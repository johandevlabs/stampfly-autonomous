#pragma once
#include <SPI.h>

// StampFly known-good SPI settings (Phase 0 probe verified)

// BMI270 IMU
static constexpr int PIN_CS_BMI270 = 46;
static const SPISettings SPI_BMI270_SETTINGS(
    8000000,      // 8 MHz (match official baseline)
    MSBFIRST,
    SPI_MODE0
);


// PMW3901 optical flow
static constexpr int PIN_CS_PMW3901 = 12;
static const SPISettings SPI_PMW3901_SETTINGS( 
    1000000,      // 1 MHz
    MSBFIRST,
    SPI_MODE3
);
