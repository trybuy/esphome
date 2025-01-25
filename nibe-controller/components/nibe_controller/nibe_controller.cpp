#include "nibe_controller.h"

namespace esphome {
namespace nibe {

static const char *const TAG = "nibe-controller";

void NibeController::setup() {
    sel_pin_->setup();
    sel_pin_->digital_write(mode_);
    last_update_timestamp_ = millis();
    phase1_.setup();
    phase2_.setup();
    phase3_.setup();
    ESP_LOGD(TAG, "Setup completed");
}    
void NibeController::update() {
    auto timestamp = millis();
    auto interval = timestamp - last_update_timestamp_;
    float seconds = interval / 1000.0f;
    if (seconds <= 1.0) {
        return;
    }

    auto ph1_values = phase1_.get_values(mode_, seconds);
    auto ph2_values = phase2_.get_values(mode_, seconds);
    auto ph3_values = phase3_.get_values(mode_, seconds);
    // ESP_LOGD(TAG, "Got power: ph1_power=%f, ph2_power=%f; ph1_frequency=%f, ph2_frequency=%f", 
    //                 ph1_values.power.value, ph2_values.power.value, ph1_values.power.frequency, ph2_values.power.frequency);
    // ESP_LOGD(TAG, "Got energy: ph1=%f, ph2=%f; ph3=%f", ph1_values.power.energy, ph2_values.power.energy, ph3_values.power.energy);
    // ESP_LOGD(TAG, "Got %s: ph1=%.3f, ph2=%.3f,  ph3=%.3f; ph1_frequency=%f, ph2_frequency=%f, ph3_frequency=%f", 
    //                 mode_ ? "voltage" : "current", ph1_values.cf1_value.value, 
    //                 ph2_values.cf1_value.value, ph3_values.cf1_value.value, 
    //                 ph1_values.cf1_value.frequency, ph2_values.cf1_value.frequency, 
    //                 ph3_values.cf1_value.frequency);

    mode_ = !mode_;
    sel_pin_->digital_write(mode_);
    phase1_.get_cf1();
    phase2_.get_cf1();
    phase3_.get_cf1();
    last_update_timestamp_ = timestamp;
    
    phase1_.publish_values(!mode_,  ph1_values.power.value, ph1_values.power.energy, ph1_values.cf1_value.value);
    phase2_.publish_values(!mode_,  ph2_values.power.value, ph2_values.power.energy, ph2_values.cf1_value.value);
    phase3_.publish_values(!mode_,  ph3_values.power.value, ph3_values.power.energy, ph3_values.cf1_value.value);
    if (!mode_ && voltage_sensor_) {
        voltage_sensor_->publish_state( ph1_values.cf1_value.value);
    }
    if (power_sensor_) {
        power_sensor_->publish_state(ph1_values.power.value + ph2_values.power.value + ph3_values.power.value);
    }
    if (energy_sensor_) {
        energy_sensor_->publish_state(ph1_values.power.energy + ph2_values.power.energy + ph3_values.power.energy);
    }
}

}
}