#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/time.h"
#include "pid_controller.h"
#include <vector>
#include <string>
#include <deque>

namespace esphome {
namespace reflow_curve {

// Forward declaration - struct defined in generated header
struct ReflowProfilePoint;


class ReflowCurve : public Component {
public:
    ReflowCurve() = default;
    
    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override;
    
    // Switch-like interface
    void turn_on();
    void turn_off();
    bool is_on() const { return is_active_; }
    
    // Set the physical switch to control
    void set_reflow_switch(switch_::Switch *reflow_switch) { reflow_switch_ = reflow_switch; }
    
    // Set the temperature sensor
    void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
    
    // Set the time component
    void set_time_component(time::RealTimeClock *time_component) { time_component_ = time_component; }
    
    // Get profile data
    std::vector<std::pair<std::string, float>> get_profile_data_with_timestamps() const;
    
    // Get temperature data
    std::string get_temperature_data_json() const;
    
    // PID Controller configuration
    void set_pid_config(const PIDConfig &config) { pid_controller_.set_config(config); }
    PIDConfig& get_pid_config() { return pid_controller_.get_config(); }
    const PIDState& get_pid_state() const { return pid_controller_.get_state(); }
    
    // Callbacks for state changes
    void add_on_state_callback(std::function<void(bool)> &&callback);

protected:
    bool is_active_{false};
    ESPTime start_timestamp_;
    switch_::Switch *reflow_switch_{nullptr};
    sensor::Sensor *temperature_sensor_{nullptr};
    time::RealTimeClock *time_component_{nullptr};
    std::vector<std::function<void(bool)>> state_callbacks_;
    
    std::deque<TemperatureDataPoint> temperature_data_;
    static const size_t MAX_DATA_POINTS = 500;
    
    // PID Controller
    PIDController pid_controller_;
    uint32_t last_control_time_{0};
    
    void trigger_state_callbacks();
    void on_temperature_update(float state);
};

}  // namespace reflow_curve
}  // namespace esphome
