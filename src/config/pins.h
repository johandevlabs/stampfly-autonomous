#pragma once

// TODO: Define board pin mappings here.

// INT_I2C (StampFly)
static constexpr int PIN_I2C_SDA = 3; // G3
static constexpr int PIN_I2C_SCL = 4; // G4
static constexpr uint32_t I2C_FREQ = 100000;

// ToF-1 (down)
static constexpr int PIN_TOF1_XSHUT = 7; // G7
static constexpr int PIN_TOF1_GPIO1 = 6; // G6

// ToF-2 (front)
static constexpr int PIN_TOF2_XSHUT = 9; // G9
static constexpr int PIN_TOF2_GPIO1 = 8; // G8

// ToF addresses (7-bit)
static constexpr uint8_t TOF_ADDR7_FRONT = 0x29; // front stays default
static constexpr uint8_t TOF_ADDR7_DOWN  = 0x30; // down gets moved here

// ToF addresses (8-bit, for InitSensor())
static constexpr uint8_t TOF_ADDR8_FRONT = (TOF_ADDR7_FRONT << 1); // 0x52
static constexpr uint8_t TOF_ADDR8_DOWN  = (TOF_ADDR7_DOWN  << 1); // 0x60

