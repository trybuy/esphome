#include "reflow_web_server.h"
#include "web_assets.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/util.h"
#include "esphome/core/helpers.h"

#ifdef USE_ARDUINO
#include <ESPAsyncWebServer.h>
#elif USE_ESP_IDF
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

#include <sstream>

namespace esphome {
namespace reflow_web_server {

static const char *const TAG = "reflow_web_server";

ReflowWebServer::ReflowWebServer(web_server_base::WebServerBase *base) : base_(base) {}

void ReflowWebServer::setup() {
    ESP_LOGCONFIG(TAG, "Setting up Reflow Web Server...");
    
    this->base_->init();
    this->base_->add_handler(this);
    
    ESP_LOGCONFIG(TAG, "Reflow Web Server handlers registered");
}

void ReflowWebServer::loop() {
    // Nothing to do in loop for this component
}

void ReflowWebServer::dump_config() {
    ESP_LOGCONFIG(TAG, "Reflow Web Server:");
    if (this->reflow_curve_ != nullptr) {
        ESP_LOGCONFIG(TAG, "  Reflow Curve: Connected");
    }
}

float ReflowWebServer::get_setup_priority() const {
    return setup_priority::WIFI - 1.0f;
}

bool ReflowWebServer::canHandle(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_GET) {
        if (request->url() == "/" ||
            request->url() == "/data" ||
            request->url() == "/profile_data" ||
            request->url() == "/style.css" ||
            request->url() == "/script.js") {
            return true;
        }
    } else if (request->method() == HTTP_POST) {
        if (request->url() == "/switch_control") {
            return true;
        }
    }
    return false;
}

void ReflowWebServer::handleRequest(AsyncWebServerRequest *request) {
    if (request->url() == "/") {
        this->handle_index(request);
    } else if (request->url() == "/data") {
        this->handle_data(request);
    } else if (request->url() == "/profile_data") {
        this->handle_profile_data(request);
    } else if (request->url() == "/style.css") {
        this->handle_style(request);
    } else if (request->url() == "/script.js") {
        this->handle_script(request);
    } else if (request->url() == "/switch_control") {
        this->handle_switch_control(request);
    }
}

void ReflowWebServer::handle_index(AsyncWebServerRequest *request) {
    auto *response = request->beginResponse_P(200, "text/html", INDEX_HTML_GZIP, INDEX_HTML_GZIP_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void ReflowWebServer::handle_data(AsyncWebServerRequest *request) {
    // Combine temperature data and switch status into single JSON response
    std::string temperature_data = "[]";
    if (this->reflow_curve_ != nullptr) {
        temperature_data = this->reflow_curve_->get_temperature_data_json();
    }
    
    // Get reflow curve state
    bool switch_state = false;
    if (this->reflow_curve_ != nullptr) {
        switch_state = this->reflow_curve_->is_on();
    }
    
    std::string json = "{";
    json += "\"temperature_data\":" + temperature_data + ",";
    json += "\"switch_state\":";
    json += switch_state ? "true" : "false";
    json += "}";
    
    request->send(200, "application/json", json.c_str());
}

void ReflowWebServer::handle_profile_data(AsyncWebServerRequest *request) {
    // Get reflow profile data if active
    std::string reflow_profile_data = "[]";
    if (this->reflow_curve_ != nullptr && this->reflow_curve_->is_on()) {
        auto profile_points = this->reflow_curve_->get_profile_data_with_timestamps();
        std::ostringstream profile_json;
        profile_json << "[";
        bool first = true;
        for (const auto &point : profile_points) {
            if (!first) profile_json << ",";
            first = false;
            profile_json << "[\"" << point.first << "\"," << point.second << "]";
        }
        profile_json << "]";
        reflow_profile_data = profile_json.str();
    }
    
    request->send(200, "application/json", reflow_profile_data.c_str());
}

void ReflowWebServer::handle_style(AsyncWebServerRequest *request) {
    auto *response = request->beginResponse_P(200, "text/css", STYLE_CSS_GZIP, STYLE_CSS_GZIP_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void ReflowWebServer::handle_script(AsyncWebServerRequest *request) {
    auto *response = request->beginResponse_P(200, "application/javascript", SCRIPT_JS_GZIP, SCRIPT_JS_GZIP_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void ReflowWebServer::handle_switch_control(AsyncWebServerRequest *request) {
    if (request->hasParam("state")) {
        AsyncWebParameter* param = request->getParam("state");
        bool new_state = param->value() == "true";
        
        // Control reflow curve
        if (this->reflow_curve_ != nullptr) {
            if (new_state) {
                this->reflow_curve_->turn_on();
            } else {
                this->reflow_curve_->turn_off();
            }
        }
    }
    
    // Return current state based on reflow curve
    bool current_state = false;
    if (this->reflow_curve_ != nullptr) {
        current_state = this->reflow_curve_->is_on();
    }
    
    std::string response = "{\"state\": ";
    response += current_state ? "true" : "false";
    response += "}";
    
    request->send(200, "application/json", response.c_str());
}
}  // namespace reflow_web_server
}  // namespace esphome