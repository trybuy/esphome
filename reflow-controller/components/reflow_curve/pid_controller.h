#pragma once

#include <vector>
#include <deque>
#include <utility>
#include <cmath>
#include <algorithm>
#include <string>

namespace esphome {
namespace reflow_curve {

// PID Controller Configuration
struct PIDConfig {
    // PID (controller output is duty 0..1)
    double Kp   = 1.2;
    double Ki   = 0.08;   // per second
    double Kd   = 4.0;    // seconds (derivative on measurement)
    double Kff  = 0.20;   // feed-forward on dTset/dt (°C/s)

    // Plant characteristics
    double tau_dead_s = 6.0;    // dead-time look-ahead

    // Filtering
    double meas_tau_s = 0.7;    // EMA time constant

    // Time-proportioning window (SSR/relay)
    double window_s   = 1.5;

    // Safety
    double max_temp_c = 260.0;
    double i_min      = -100.0;
    double i_max      =  100.0;

    // Predictive clamp near setpoint
    double near_band_c = 5.0;
    double u_near_max  = 0.6;

    double preheat_target_c   = 120.0;  // setpoint during warmup
    unsigned int preheat_duration_s = 60;   // length of warmup
};

// PID Controller State
struct PIDState {
    // Persist across ticks
    double int_e         = 0.0;
    double filt_temp_c   = 25.0;
    double last_temp_c   = 25.0;
    bool   last_valid    = false;

    // SSR window bookkeeping
    double window_t      = 0.0;  // [0, window_s)
    double on_time_s     = 0.0;

    // Diagnostics (optional)
    double last_u        = 0.0;
    double last_set_c    = 25.0;
    double last_set_slope= 0.0;
};

// Temperature data structure
struct TemperatureDataPoint {
    std::string iso_timestamp;
    float temperature;
};

// PID Controller Class
class PIDController {
public:
    PIDController() = default;
    explicit PIDController(const PIDConfig &config) : config_(config) {}
    
    // Configuration management
    void set_config(const PIDConfig &config) { config_ = config; }
    PIDConfig& get_config() { return config_; }
    const PIDConfig& get_config() const { return config_; }
    
    // State management
    const PIDState& get_state() const { return state_; }
    void reset_state();
    
    // Setpoint lookup from profile
    std::pair<double, double> lookup_setpoint(int now_s) const;
    
    // Main control function
    bool control_tick(int now_s, double dt_s, const std::deque<TemperatureDataPoint> &temp_data);

private:
    PIDConfig config_;
    PIDState state_;
};

}  // namespace reflow_curve
}  // namespace esphome
