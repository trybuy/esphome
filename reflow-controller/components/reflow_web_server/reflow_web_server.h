#pragma once

#include "esphome/core/component.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "../reflow_curve/reflow_curve.h"

#ifdef USE_ARDUINO
#include <ESPAsyncWebServer.h>
#elif USE_ESP_IDF
#include "esphome/core/hal.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

#include <memory>
#include <string>

namespace esphome {
namespace reflow_web_server {

class ReflowWebServer : public AsyncWebHandler, public Component {
public:
    ReflowWebServer(web_server_base::WebServerBase *base);
    
    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override;
    
    void set_reflow_curve(reflow_curve::ReflowCurve *reflow_curve) { reflow_curve_ = reflow_curve; }
    void set_update_interval(uint32_t update_interval) { update_interval_ = update_interval; }
    
    // AsyncWebHandler interface
    bool canHandle(AsyncWebServerRequest *request) override;
    void handleRequest(AsyncWebServerRequest *request) override;
    
protected:
    void handle_index(AsyncWebServerRequest *request);
    void handle_data(AsyncWebServerRequest *request);
    void handle_profile_data(AsyncWebServerRequest *request);
    void handle_style(AsyncWebServerRequest *request);
    void handle_script(AsyncWebServerRequest *request);
    void handle_switch_control(AsyncWebServerRequest *request);
    
    web_server_base::WebServerBase *base_;
    reflow_curve::ReflowCurve *reflow_curve_{nullptr};
    uint32_t update_interval_{1000};
    uint32_t last_update_{0};
};

}  // namespace reflow_web_server
}  // namespace esphome