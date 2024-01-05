#pragma once

#include <array>

namespace phase_stepping {
static constexpr int MOTOR_PERIOD = 1024; // The number of ticks per electrical period of the Trinamic driver
static constexpr int SIN_FRACTION = 4;
static constexpr int SIN_PERIOD = SIN_FRACTION * MOTOR_PERIOD;
static constexpr int CURRENT_AMPLITUDE = 248;
static constexpr int CORRECTION_HARMONICS = 16;
static constexpr int SUPPORTED_AXIS_COUNT = 2;
static constexpr int SERIAL_DECIMALS = 5; // Float decimals written in the serial output
static constexpr int ALLOWED_MISSED_TX = 5000;

struct SpectralItem {
    float mag = 0, pha = 0;
};

// WARNING: changing this type has consequences in regards to data persistency!
using MotorPhaseCorrection = std::array<SpectralItem, CORRECTION_HARMONICS + 1>;
} // namespace phase_stepping
