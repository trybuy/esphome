#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "../reflow_curve/reflow_curve.h"

#ifdef USE_ESP32
#include "esp_http_server.h"
#endif

#include <deque>
#include <memory>
#include <string>

namespace esphome {
namespace reflow_web_server {

struct TemperatureDataPoint {
    std::string iso_timestamp;
    float temperature;
};

class ReflowWebServer : public Component {
public:
    ReflowWebServer() = default;
    
    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override;
    
    void set_port(uint16_t port) { port_ = port; }
    void set_username(const std::string &username) { username_ = username; }
    void set_password(const std::string &password) { password_ = password; }
    void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
    void set_reflow_curve(reflow_curve::ReflowCurve *reflow_curve) { reflow_curve_ = reflow_curve; }
    void set_update_interval(uint32_t update_interval) { update_interval_ = update_interval; }
    void set_time_component(time::RealTimeClock *time_component) { time_component_ = time_component; }
    
protected:
    static esp_err_t index_handler(httpd_req_t *req);
    static esp_err_t data_handler(httpd_req_t *req);
    static esp_err_t style_handler(httpd_req_t *req);
    static esp_err_t script_handler(httpd_req_t *req);
    static esp_err_t switch_control_handler(httpd_req_t *req);
    
    std::string get_temperature_data_json();
    bool authenticate_request(httpd_req_t *req);
    void on_temperature_update(float state);
    
    
    httpd_handle_t server_{nullptr};
    uint16_t port_{80};
    std::string username_;
    std::string password_;
    
    sensor::Sensor *temperature_sensor_{nullptr};
    reflow_curve::ReflowCurve *reflow_curve_{nullptr};
    time::RealTimeClock *time_component_{nullptr};
    uint32_t update_interval_{1000};
    uint32_t last_update_{0};
    
    std::deque<TemperatureDataPoint> temperature_data_;
    static const size_t MAX_DATA_POINTS = 500;
    
    // Static instance for C callback access
    static ReflowWebServer* instance_;
};

}  // namespace reflow_web_server
}  // namespace esphome