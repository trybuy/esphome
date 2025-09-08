#include "reflow_curve.h"
#include "reflow_profile_data.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include <ctime>

namespace esphome {
namespace reflow_curve {

static const char *const TAG = "reflow_curve";

void ReflowCurve::setup() {
    ESP_LOGCONFIG(TAG, "Setting up Reflow Curve...");
}

void ReflowCurve::loop() {
    // Nothing to do in loop for this component
}

void ReflowCurve::dump_config() {
    ESP_LOGCONFIG(TAG, "Reflow Curve:");
    ESP_LOGCONFIG(TAG, "  Profile Points: %d", REFLOW_PROFILE_SIZE);
    ESP_LOGCONFIG(TAG, "  State: %s", this->is_active_ ? "ON" : "OFF");
}

float ReflowCurve::get_setup_priority() const {
    return setup_priority::LATE;
}

void ReflowCurve::turn_on() {
    if (!this->is_active_) {
        this->is_active_ = true;
        
        // Turn on physical switch
        if (this->reflow_switch_ != nullptr) {
            this->reflow_switch_->turn_on();
        }
        
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
        std::string timestamp = point_time.strftime("%Y-%m-%dT%H:%M:%S");
        
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

}  // namespace reflow_curve
}  // namespace esphome
