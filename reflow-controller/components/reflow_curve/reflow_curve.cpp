#include "reflow_curve.h"
#include "reflow_profile_data.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include <ctime>
#include <sstream>

namespace esphome {
namespace reflow_curve {

static const char *const TAG = "reflow_curve";

void ReflowCurve::setup() {
    ESP_LOGCONFIG(TAG, "Setting up Reflow Curve...");
    
    if (this->temperature_sensor_ != nullptr) {
        this->temperature_sensor_->add_on_state_callback([this](float state) {
            this->on_temperature_update(state);
        });
    }
}

void ReflowCurve::loop() {
    // Run PID control if active
    if (this->is_active_ && this->start_timestamp_.is_valid()) {
        uint32_t now_millis = millis();
        double dt_s = (now_millis - this->last_control_time_) / 1000.0;
        
        // Run control at reasonable frequency (e.g., every 100ms)
        if (dt_s >= 0.1) {
            // Calculate elapsed time since start
            ESPTime now_time = ESPTime::from_epoch_local(now_millis / 1000);
            unsigned int elapsed_s = static_cast<unsigned int>(now_time.timestamp - this->start_timestamp_.timestamp);
            
            // Run PID control
            bool heater_should_be_on = this->pid_controller_.control_tick(elapsed_s, dt_s, this->temperature_data_);
            
            // Control the switch based on PID output
            if (this->reflow_switch_ != nullptr) {
                if (heater_should_be_on) {
                    if (!this->reflow_switch_->state) {
                        this->reflow_switch_->turn_on();
                    }
                } else if(this->reflow_switch_->state) {
                    this->reflow_switch_->turn_off();
                }
            }
            
            this->last_control_time_ = now_millis;
        }
    }
}

void ReflowCurve::dump_config() {
    ESP_LOGCONFIG(TAG, "Reflow Curve:");
    ESP_LOGCONFIG(TAG, "  Profile Points: %d", REFLOW_PROFILE_SIZE);
    ESP_LOGCONFIG(TAG, "  State: %s", this->is_active_ ? "ON" : "OFF");
    if (this->temperature_sensor_ != nullptr) {
        ESP_LOGCONFIG(TAG, "  Temperature Sensor: %s", this->temperature_sensor_->get_name().c_str());
    }
}

float ReflowCurve::get_setup_priority() const {
    return setup_priority::LATE;
}

void ReflowCurve::turn_on() {
    if (!this->is_active_) {
        this->is_active_ = true;
        
        // Reset PID controller state
        this->pid_controller_.reset_state();
        this->last_control_time_ = millis();
        
        // Get current time for start timestamp using ESPHome time component
        if (this->time_component_ != nullptr) {
            this->start_timestamp_ = this->time_component_->now();
            if (!this->start_timestamp_.is_valid()) {
                // Fallback to millis if time is not synced yet
                this->start_timestamp_ = ESPTime::from_epoch_local(millis() / 1000);
            }
        } else {
            // Fallback to millis if time component is not available
            this->start_timestamp_ = ESPTime::from_epoch_local(millis() / 1000);
        }
        
        ESP_LOGI(TAG, "Reflow curve started at: %s", this->start_timestamp_.strftime("%Y-%m-%d %H:%M:%S").c_str());
        this->trigger_state_callbacks();
    }
}

void ReflowCurve::turn_off() {
    if (this->is_active_) {
        this->is_active_ = false;
        
        // Turn off physical switch
        if (this->reflow_switch_ != nullptr) {
            this->reflow_switch_->turn_off();
        }
        
        this->start_timestamp_ = ESPTime{};
        ESP_LOGI(TAG, "Reflow curve stopped");
        this->trigger_state_callbacks();
    }
}

std::vector<std::pair<std::string, float>> ReflowCurve::get_profile_data_with_timestamps() const {
    std::vector<std::pair<std::string, float>> result;
    
    if (!this->is_active_ || !this->start_timestamp_.is_valid()) {
        return result;  // Return empty if not active
    }
    
    // Generate timestamps for each profile point
    for (int i = 0; i < REFLOW_PROFILE_SIZE; i++) {
        const auto &point = REFLOW_PROFILE_DATA[i];
        
        // Calculate point time by adding seconds to start time
        ESPTime point_time = this->start_timestamp_;
        point_time.timestamp += point.time_seconds;
        
        // Convert to ISO timestamp string
        std::string timestamp = ESPTime::from_epoch_local(point_time.timestamp).strftime("%Y-%m-%dT%H:%M:%S");
        
        result.push_back({timestamp, point.temperature_celsius});
    }
    
    return result;
}

void ReflowCurve::add_on_state_callback(std::function<void(bool)> &&callback) {
    this->state_callbacks_.push_back(std::move(callback));
}

void ReflowCurve::trigger_state_callbacks() {
    for (auto &callback : this->state_callbacks_) {
        callback(this->is_active_);
    }
}

std::string ReflowCurve::get_temperature_data_json() const {
    std::ostringstream json;
    json << "[";
    bool first = true;
    
    for (const auto &point : this->temperature_data_) {
        if (!first) {
            json << ",";
        }
        first = false;
        
        // Return ISO timestamp as string with temperature value
        json << "[\"" << point.iso_timestamp << "\"," << point.temperature << "]";
    }
    
    json << "]";
    return json.str();
}

void ReflowCurve::on_temperature_update(float state) {
    if (std::isnan(state)) {
        return;  // Skip NaN values
    }
    
    std::string iso_timestamp;
    
    // Try to get actual time using ESPHome time component
    if (this->time_component_ != nullptr) {
        auto time_now = this->time_component_->now();
        if (time_now.is_valid()) {
            // Convert ESPHome time to ISO string format
            iso_timestamp = time_now.strftime("%Y-%m-%dT%H:%M:%S");
        } else {
            // Fallback to relative time if not synced yet
            uint32_t millis_time = millis() / 1000;
            iso_timestamp = "T+" + std::to_string(millis_time) + "s";
        }
    } else {
        // Fallback to relative time if time component is not available
        uint32_t millis_time = millis() / 1000;
        iso_timestamp = "T+" + std::to_string(millis_time) + "s";
    }
    
    // Add new data point
    this->temperature_data_.push_back({iso_timestamp, state});
    
    // Remove old data points to keep memory usage reasonable
    while (this->temperature_data_.size() > MAX_DATA_POINTS) {
        this->temperature_data_.pop_front();
    }
}

}  // namespace reflow_curve
}  // namespace esphome
