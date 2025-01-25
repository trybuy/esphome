#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/pulse_counter/pulse_counter_sensor.h"

namespace esphome {
namespace nibe {

struct freq_value {
    float frequency;
    float value;
};

struct power_value : freq_value {
    float energy;
};

struct BL0937_values {
    power_value power;
    freq_value cf1_value;
};



class BL0937 {
public:
    BL0937(bool use_hw = true)
        : cf_store_(*pulse_counter::get_storage(true))
        , cf1_store_(*pulse_counter::get_storage(use_hw))
        , current_coefficient_(0.0067f)
        , voltage_coefficient_(0.155f)
        , power_coefficient_(1)
        , cf_total_(0) {}
public:
    void setup() {
        cf_store_.filter_us = cf1_store_.filter_us = 35;
        cf_store_.pulse_counter_setup(cf_pin_);
        cf1_store_.pulse_counter_setup(cf1_pin_);
    }

    void set_cf_pin(InternalGPIOPin *cf_pin) { cf_pin_ = cf_pin; }
    void set_cf1_pin(InternalGPIOPin *cf1_pin) { cf1_pin_ = cf1_pin; }
    void set_current_coefficient(float c) { current_coefficient_ = c; }
    void set_voltage_coefficient(float c) { voltage_coefficient_ = c; }
    void set_power_coefficient(float c) { power_coefficient_ = c; }

    float get_cf1() {
        return cf1_store_.read_raw_value();
    }

    BL0937_values get_values(bool mode, float seconds) {
        return {get_power(seconds), mode ? get_voltage(seconds) : get_current(seconds)};
    }

    void publish_values(bool mode, float power, float energy, float cf1_value) {
        if (power_sensor_) {
            power_sensor_->publish_state(power);
        }
        if (energy_sensor_) {
            energy_sensor_->publish_state(energy);
        }
        if (mode) {
            if (voltage_sensor_) {
                voltage_sensor_->publish_state(cf1_value);
            }
        } else {
            if (current_sensor_) {
                current_sensor_->publish_state(cf1_value);
            }
        }
    }

    power_value get_power(float seconds) {
        auto cf = cf_store_.read_raw_value();
        cf_total_ += cf;
        float frequency = get_frequency(cf, seconds);
        return {frequency, frequency * power_coefficient_, (cf_total_ * power_coefficient_) / 3600000};
    }

    freq_value get_current(float seconds) {
        float frequency = get_frequency(get_cf1(), seconds);
        return {frequency, frequency * current_coefficient_};
    }
    freq_value get_voltage(float seconds) {
        float frequency = get_frequency(get_cf1(), seconds);
        return {frequency, frequency * voltage_coefficient_};
    }

    float get_frequency(float f, float seconds) {
        float frequency = f / seconds;
        return frequency < 1.1 ? 0.0f : frequency;
    }
public:
    void set_voltage_sensor(sensor::Sensor* voltage_sensor) { voltage_sensor_ = voltage_sensor; }
    void set_current_sensor(sensor::Sensor* current_sensor) { current_sensor_ = current_sensor; }
    void set_power_sensor(sensor::Sensor* power_sensor) { power_sensor_ = power_sensor; }
    void set_energy_sensor(sensor::Sensor* energy_sensor) { energy_sensor_ = energy_sensor; }
private:
    InternalGPIOPin *cf_pin_; 
    InternalGPIOPin *cf1_pin_;
    pulse_counter::PulseCounterStorageBase &cf_store_;
    pulse_counter::PulseCounterStorageBase &cf1_store_;
    float current_coefficient_;
    float voltage_coefficient_;
    float power_coefficient_;
    pulse_counter::pulse_counter_t cf_total_;
private:
    sensor::Sensor* voltage_sensor_{0};
    sensor::Sensor* current_sensor_{0};
    sensor::Sensor* power_sensor_{0};
    sensor::Sensor* energy_sensor_{0};
};

class NibeController : public PollingComponent {
public: 
    NibeController(): sel_pin_(0), mode_(false), last_update_timestamp_(0), phase1_(true), phase2_(false), phase3_(false) {}
public:
    void setup() override;
    void update() override;

    float get_setup_priority() const override { return setup_priority::LATE; }

    void set_sel_pin(GPIOPin *sel_pin) { sel_pin_ = sel_pin; }

    BL0937& get_phase1() {return phase1_;}
    BL0937& get_phase2() {return phase2_;}
    BL0937& get_phase3() {return phase3_;}
public:
    void set_voltage_sensor(sensor::Sensor* voltage_sensor) { voltage_sensor_ = voltage_sensor; }
    void set_power_sensor(sensor::Sensor* power_sensor) { power_sensor_ = power_sensor; }
    void set_energy_sensor(sensor::Sensor* energy_sensor) { energy_sensor_ = energy_sensor; }
private:
    GPIOPin *sel_pin_;
    bool mode_;
    uint32_t last_update_timestamp_;
    BL0937 phase1_;
    BL0937 phase2_;
    BL0937 phase3_;

    sensor::Sensor* voltage_sensor_{0};
    sensor::Sensor* power_sensor_{0};
    sensor::Sensor* energy_sensor_{0};
};

}
}
