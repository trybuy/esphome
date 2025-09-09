#include "pid_controller.h"
#include "reflow_profile_data.h"
#include "esphome/core/log.h"

namespace esphome {
namespace reflow_curve {

static const char *const PID_TAG = "pid_controller";

void PIDController::reset_state() {
    state_ = PIDState{};
}

std::pair<double, double> PIDController::lookup_setpoint(unsigned int now_s) const {
    if (REFLOW_PROFILE_SIZE == 0) return {25.0, 0.0};
    
    unsigned int sec = now_s;
    if (sec <= REFLOW_PROFILE_DATA[0].time_seconds) {
        return {REFLOW_PROFILE_DATA[0].temperature_celsius, 0.0};
    }
    if (sec >= REFLOW_PROFILE_DATA[REFLOW_PROFILE_SIZE - 1].time_seconds) {
        return {REFLOW_PROFILE_DATA[REFLOW_PROFILE_SIZE - 1].temperature_celsius, 0.0};
    }
    
    // Find the profile points that bracket this time
    unsigned int idx = 0;
    for (unsigned int i = 0; i < REFLOW_PROFILE_SIZE - 1; i++) {
        if (REFLOW_PROFILE_DATA[i].time_seconds <= sec && sec < REFLOW_PROFILE_DATA[i + 1].time_seconds) {
            idx = i;
            break;
        }
    }
    
    // Linear interpolation between points
    if (idx >= REFLOW_PROFILE_SIZE - 1) {
        return {REFLOW_PROFILE_DATA[REFLOW_PROFILE_SIZE - 1].temperature_celsius, 0.0};
    }
    
    const auto &curr = REFLOW_PROFILE_DATA[idx + 1];
    const auto &prev = REFLOW_PROFILE_DATA[idx];
    
    // Calculate slope (°C per second)
    double dt = curr.time_seconds - prev.time_seconds;
    double dT = curr.temperature_celsius - prev.temperature_celsius;
    double slope = (dt > 0) ? (dT / dt) : 0.0;
    
    // Interpolate temperature at current time
    double t_frac = (now_s - prev.time_seconds) / dt;
    double temp = prev.temperature_celsius + t_frac * dT;
    
    return {temp, slope};
}

bool PIDController::control_tick(unsigned int now_s, double dt_s, const std::deque<TemperatureDataPoint> &temp_data) {
    if (temp_data.empty() || dt_s <= 0.0) {
        state_.last_u = 0.0;
        state_.on_time_s = 0.0;
        return false;
    }

    // 1) Setpoint & discrete slope (°C/s) from profile
    auto [Tset, dTset_dt] = lookup_setpoint(now_s);
    state_.last_set_c = Tset;
    state_.last_set_slope = dTset_dt;

    // 2) Filter measurement (EMA)
    const double raw = temp_data.back().temperature;
    const double alpha = std::exp(-dt_s / std::max(1e-6, config_.meas_tau_s)); // 0..1
    state_.filt_temp_c = alpha * state_.filt_temp_c + (1.0 - alpha) * raw;

    // 3) Derivative on measurement
    double dTdt = 0.0;
    if (state_.last_valid) {
        dTdt = (state_.filt_temp_c - state_.last_temp_c) / dt_s;
    }
    state_.last_temp_c = state_.filt_temp_c;
    state_.last_valid = true;

    // 4) Dead-time look-ahead
    const double Tproj = state_.filt_temp_c + dTdt * config_.tau_dead_s;

    // 5) PID (+FF) with anti-windup gating
    const double e = Tset - Tproj;
    const double p = config_.Kp * e;
    double i_next = state_.int_e + config_.Ki * e * dt_s;                // raw integrator step
    const double d = -config_.Kd * dTdt;                                // derivative on meas
    const double u_ff = config_.Kff * dTset_dt;                         // °C/s → duty trimming
    double u_unsat = p + i_next + d + u_ff;

    const bool allow_integrate = (u_unsat > 0.0 && u_unsat < 1.0) ||
                               (u_unsat >= 1.0 && e < 0.0) ||
                               (u_unsat <= 0.0 && e > 0.0);

    if (allow_integrate) {
        state_.int_e = std::clamp(i_next, config_.i_min, config_.i_max);
    }

    double u = p + state_.int_e + d + u_ff;

    // Predictive clamp when close to setpoint and heating up fast
    if (std::abs(e) < config_.near_band_c && dTdt > 0.0) {
        u = std::min(u, config_.u_near_max);
    }

    // Safety
    if (!(u == u) || raw >= config_.max_temp_c + 2.0) u = 0.0; // NaN or overtemp
    u = std::clamp(u, 0.0, 1.0);
    state_.last_u = u;

    // 6) Time-proportioning window (SSR/relay)
    state_.window_t += dt_s;
    if (state_.window_t >= config_.window_s) {
        state_.window_t = 0.0;
        state_.on_time_s = u * config_.window_s;
    }
    const bool heater_on = (state_.window_t < state_.on_time_s);
    
    // Log PID state for debugging
    ESP_LOGD(PID_TAG, "PID: T=%.1f°C, Tset=%.1f°C, u=%.3f, heater=%s", 
             state_.filt_temp_c, Tset, u, heater_on ? "ON" : "OFF");
    
    return heater_on;
}

}  // namespace reflow_curve
}  // namespace esphome
