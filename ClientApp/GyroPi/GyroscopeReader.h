// ClientApp/GyroPi/GyroscopeReader.h — Tilt input for GyroPi
//
// Reads raw IMU data from a BMI160 over SPI and applies a complementary
// filter to produce stable, normalised tilt values ready to send to the server.
//
// Hardware path: compile with -DUSE_HARDWARE_SENSOR
//   Uses SPIDriver.hpp → /dev/spidev0.0 → BMI160
//
// No-hardware path (default): returns a flat/still placeholder.

#pragma once
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>

#ifdef USE_HARDWARE_SENSOR
#include "SPIDriver.hpp"
#endif

// ── Raw IMU sample from the BMI160 ───────────────────────────────────────────
// Accelerometer in g-force, gyroscope in degrees/second
struct RawImuData
{
    float accelX = 0, accelY = 0, accelZ = 1; // accelZ = 1 → sensor lying flat
    float gyroX = 0, gyroY = 0, gyroZ = 0;
};

// ── Normalised output sent to the server ─────────────────────────────────────
struct TiltReading
{
    float tiltX = 0; // normalised [-1.0, 1.0] — roll  → ball X-axis
    float tiltY = 0; // normalised [-1.0, 1.0] — pitch → ball Y-axis
};

class GyroscopeReader
{
public:
    // Returns the current normalised tilt.
    // Call once per tick (every TICK_MS ms) for correct filter behaviour.
    TiltReading read();

private:
    // Opens /dev/spidev0.0 and wakes the BMI160 accelerometer + gyroscope.
    void initialiseHardware();

    // Reads one raw sample (accel + gyro) from the BMI160 via SPI.
    RawImuData readRawImu();

    // ── Complementary filter state (degrees) ─────────────────────────────────
    float rollAngle_ = 0.0f;
    float pitchAngle_ = 0.0f;

    // Timestamp of the previous read — used to compute elapsed time between ticks
    std::chrono::steady_clock::time_point lastReadTime_ = std::chrono::steady_clock::now();

    bool initialised_ = false;

    // Tilt angle that maps to ±1.0 normalised output (degrees).
    // Increase to reduce sensitivity (larger physical tilt needed for full output).
    static constexpr float MAX_TILT_DEGREES = 45.0f;

    // Gyroscope weight in the complementary filter (0–1).
    // Lowered from 0.98 to reduce drift accumulation when lying still.
    static constexpr float GYRO_WEIGHT = 0.999f;

    // Hysteresis thresholds (normalised 0–1):
    // activeThreshold_  — tilt must exceed this before output becomes non-zero
    // releaseThreshold_ — tilt must drop below this before output returns to zero
    static constexpr float ACTIVATE_THRESHOLD = 0.25f;
    static constexpr float RELEASE_THRESHOLD = 0.10f;

    bool hysteresisActiveX_ = false;
    bool hysteresisActiveY_ = false;

    static float clamp(float value, float lowerBound, float upperBound)
    {
        return value < lowerBound   ? lowerBound
               : value > upperBound ? upperBound
                                    : value;
    }

    float applyHysteresis(float value, bool &active)
    {
        if (active)
        {
            if (std::abs(value) < RELEASE_THRESHOLD)
            {
                active = false;
                return 0.0f;
            }
        }
        else
        {
            if (std::abs(value) < ACTIVATE_THRESHOLD)
            {
                return 0.0f;
            }
            active = true;
        }
        return value;
    }

#ifdef USE_HARDWARE_SENSOR
    std::unique_ptr<SPIDriver> spiDriver_;
#endif
};

// ── read() ────────────────────────────────────────────────────────────────────
inline TiltReading GyroscopeReader::read()
{
    if (!initialised_)
    {
        initialiseHardware();
        initialised_ = true;
    }

    // Compute elapsed time since the last call (seconds)
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastReadTime_).count();
    lastReadTime_ = currentTime;

    RawImuData rawSensorData = readRawImu();

    // Tilt angles derived from accelerometer alone (degrees).
    // atan2 gives a stable absolute angle but is noisy at high frequencies.
    float accelRollAngle = -std::atan2(rawSensorData.accelY,
                                       std::sqrt(rawSensorData.accelX * rawSensorData.accelX + rawSensorData.accelZ * rawSensorData.accelZ)) *
                           (180.0f / M_PI);

    float accelPitchAngle = std::atan2(rawSensorData.accelX,
                                       std::sqrt(rawSensorData.accelY * rawSensorData.accelY + rawSensorData.accelZ * rawSensorData.accelZ)) *
                            (180.0f / M_PI);

    // Complementary filter: blend gyroscope integration (responsive, drifts)
    // with accelerometer angle (stable long-term, noisy short-term).
    rollAngle_ = GYRO_WEIGHT * (rollAngle_ - rawSensorData.gyroX * deltaTime) + (1.0f - GYRO_WEIGHT) * accelRollAngle;

    pitchAngle_ = GYRO_WEIGHT * (pitchAngle_ - rawSensorData.gyroY * deltaTime) + (1.0f - GYRO_WEIGHT) * accelPitchAngle;

    // Normalise angles to the [-1.0, 1.0] range the server expects,
    // then zero out anything within the dead-zone (±HYSTERESIS).
    float normalizedX = clamp(rollAngle_ / MAX_TILT_DEGREES, -1.0f, 1.0f);
    float normalizedY = clamp(pitchAngle_ / MAX_TILT_DEGREES, -1.0f, 1.0f);

    TiltReading tiltOutput;
    tiltOutput.tiltX = applyHysteresis(normalizedX, hysteresisActiveX_);
    tiltOutput.tiltY = applyHysteresis(normalizedY, hysteresisActiveY_);

    // Safety: NaN can arise if the sensor returns all-zero values on startup
    // (atan2(0,0) is undefined).  Reset the filter and return flat.
    if (!std::isfinite(tiltOutput.tiltX) || !std::isfinite(tiltOutput.tiltY))
    {
        rollAngle_ = 0.0f;
        pitchAngle_ = 0.0f;
        return TiltReading{};
    }
    return tiltOutput;
}

// ── Hardware implementations ──────────────────────────────────────────────────
#ifdef USE_HARDWARE_SENSOR

inline void GyroscopeReader::initialiseHardware()
{
    spiDriver_ = std::make_unique<SPIDriver>("/dev/spidev0.0");

    // BMI160 CMD register (0x7E): value 0x11 = PMU_ACC_NORMAL — wake accelerometer
    uint8_t command = 0x11;
    spiDriver_->write(0x7E, &command, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // datasheet: up to 50 ms

    // BMI160 CMD register (0x7E): value 0x15 = PMU_GYR_NORMAL — wake gyroscope
    command = 0x15;
    spiDriver_->write(0x7E, &command, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(80)); // datasheet: up to 80 ms

    std::cout << "[GyroscopeReader] BMI160 initialised on /dev/spidev0.0\n";
}

inline RawImuData GyroscopeReader::readRawImu()
{
    uint8_t dataBuffer[6];
    RawImuData sensorData;

    // ── Gyroscope registers 0x0C–0x11 (gyr_x_lsb … gyr_z_msb) ──────────────
    // BMI160 default range: ±2000 °/s → 16.384 LSB per °/s
    // Byte order: LSB first (little-endian)
    spiDriver_->read(0x0C, dataBuffer, 6);
    int16_t rawGyroX = static_cast<int16_t>((dataBuffer[1] << 8) | dataBuffer[0]);
    int16_t rawGyroY = static_cast<int16_t>((dataBuffer[3] << 8) | dataBuffer[2]);

    sensorData.gyroX = rawGyroX / 16.384f; // convert to degrees/second
    sensorData.gyroY = rawGyroY / 16.384f;

    // ── Accelerometer registers 0x12–0x17 (acc_x_lsb … acc_z_msb) ───────────
    // BMI160 default range: ±2 g → 16384 LSB per g
    // Byte order: LSB first (little-endian)
    spiDriver_->read(0x12, dataBuffer, 6);
    int16_t rawAccelX = static_cast<int16_t>((dataBuffer[1] << 8) | dataBuffer[0]);
    int16_t rawAccelY = static_cast<int16_t>((dataBuffer[3] << 8) | dataBuffer[2]);
    int16_t rawAccelZ = static_cast<int16_t>((dataBuffer[5] << 8) | dataBuffer[4]);

    sensorData.accelX = rawAccelX / 16384.0f; // convert to g-force
    sensorData.accelY = rawAccelY / 16384.0f;
    sensorData.accelZ = rawAccelZ / 16384.0f;

    return sensorData;
}

#else // ── No-hardware stub ──────────────────────────────────────────────────

inline void GyroscopeReader::initialiseHardware() {}

inline RawImuData GyroscopeReader::readRawImu()
{
    return RawImuData{}; // flat/still — no sensor connected
}

#endif // USE_HARDWARE_SENSOR
