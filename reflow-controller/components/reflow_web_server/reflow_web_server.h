#pragma once

#include "esphome/core/component.h"
#include "../reflow_curve/reflow_curve.h"

#ifdef USE_ESP32
#include "esp_http_server.h"
#endif

#include <deque>
#include <memory>
#include <string>

namespace esphome {
namespace reflow_web_server {

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
    void set_reflow_curve(reflow_curve::ReflowCurve *reflow_curve) { reflow_curve_ = reflow_curve; }
    void set_update_interval(uint32_t update_interval) { update_interval_ = update_interval; }
    
protected:
    static esp_err_t index_handler(httpd_req_t *req);
    static esp_err_t data_handler(httpd_req_t *req);
    static esp_err_t profile_data_handler(httpd_req_t *req);
    static esp_err_t style_handler(httpd_req_t *req);
    static esp_err_t script_handler(httpd_req_t *req);
    static esp_err_t switch_control_handler(httpd_req_t *req);
    
    bool authenticate_request(httpd_req_t *req);
    
    
    httpd_handle_t server_{nullptr};
    uint16_t port_{80};
    std::string username_;
    std::string password_;
    
    reflow_curve::ReflowCurve *reflow_curve_{nullptr};
    uint32_t update_interval_{1000};
    uint32_t last_update_{0};
    
    // Static instance for C callback access
    static ReflowWebServer* instance_;
};

}  // namespace reflow_web_server
}  // namespace esphome