#include "sensors_hal.h"
#include "MAX30100_PulseOximeter.h"

PulseOximeter pox;

// I2C Address for BMI160 and MAX3010x
const uint8_t BMI160_ADDR = 0x68;

static bool isHrActive = false;

static bool isI2cStarted = false;

void sensors_init() {
    // Empty per user request to keep setup() simple and prevent boot hangs
    isI2cStarted = false;
    isHrActive = false;
}

bool sensors_heartrate_init() {
    if (isHrActive) return true;
    
    if (!isI2cStarted) {
        Serial.println("[SNS] Starting I2C Bus...");
        Wire.begin(I2C_SDA, I2C_SCL);
        Wire.setClock(400000);
        isI2cStarted = true;
    }
    
    Serial.println("[SNS] Initializing MAX30100...");
    if (!pox.begin()) {
        Serial.println("[SNS] ERROR: MAX30100 Failed.");
        return false;
    }
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    isHrActive = true;
    return true;
}

void sensors_heartrate_deinit() {
    if (!isHrActive) return;
    Serial.println("[SNS] De-init: Stopping MAX30100...");
    // MAX30100lib doesn't have a direct shutdown, 
    // but we'll stop calling update and turn off LEDs by setting current to 0
    pox.setIRLedCurrent(MAX30100_LED_CURR_0MA);
    isHrActive = false;
}

void sensors_update() {
    if (isHrActive) {
        pox.update();
    }
}

void sensors_test_bmi160() {
    if (!isI2cStarted) return; // Don't crash if I2C isn't ready
    
    // Read Accel XYZ (registers 0x12 to 0x17)
    Wire.beginTransmission(BMI160_ADDR);
    Wire.write(0x12);
    Wire.endTransmission(false); // Repeated start
    
    Wire.requestFrom(BMI160_ADDR, (uint8_t)6);
    if (Wire.available() == 6) {
        int16_t ax = Wire.read() | (Wire.read() << 8);
        int16_t ay = Wire.read() | (Wire.read() << 8);
        int16_t az = Wire.read() | (Wire.read() << 8);
        Serial.printf("[SNS] BMI160 Tilt -> X: %d, Y: %d, Z: %d\n", ax, ay, az);
    } else {
        Serial.println("[SNS] BMI160 Read Failed! I2C error.");
    }
}

void sensors_prepare_sleep() {
    Serial.println("[SNS] Sensors sleep bypassed.");
    // In the future, we can add pox.shutdown() if the library supports it
    Wire.end(); 
}

int sensors_read_heartrate() {
    // Return latest calculated heart rate from library
    return (int)pox.getHeartRate();
}

float sensors_read_battery_voltage() {
    // Per Rule-2.5: Battery Percentage & Charging Behavior
    // Average 20 samples for better stability
    uint32_t total = 0;
    for (int i = 0; i < 20; i++) {
        total += analogRead(B_READ_PIN);
        delay(1);
    }
    float raw_val = total / 20.0;
    
    // Voltage divider calculation:
    // User uses 220k and 220k (1:1 divider), so multiplier is 2.0
    // ESP32-C3 ADC is 12-bit (0-4095). 
    // Default attenuation is usually 11dB (0-3.1V range covers 0-4095)
    // V_batt = (ADC / 4095.0) * Reference_Voltage * Divider_Multiplier
    // Using 3.1V as a typical reference for 11dB attenuation
    float battery_voltage = (raw_val / 4095.0) * 3.1 * 2.0; 
    
    // Add small calibration offset if needed (based on 220k resistor tolerance)
    return battery_voltage;
}

float sensors_battery_to_percentage(float voltage) {
    // Rule-2.5 Non-linear mapping (Li-ion curve)
    // 4.2V = 100%
    // 3.7V = ~50%
    // 3.4V = 0% (Rule-8 Critical Level)
    
    if (voltage >= 4.20) return 100.0;
    if (voltage <= 3.40) return 0.0;
    
    // Simplified non-linear curve (piecewise linear approximation)
    if (voltage > 3.70) {
        // Range 3.7 to 4.2 (100-50%)
        return 50.0 + (voltage - 3.70) * (50.0 / 0.50);
    } else {
        // Range 3.4 to 3.7 (0-50%)
        return (voltage - 3.40) * (50.0 / 0.30);
    }
}
