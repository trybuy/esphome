#include "reflow_web_server.h"
#include "web_assets.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/util.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32
#include "esp_http_server.h"
#include <cstring>
#include <sstream>
#endif

namespace esphome {
namespace reflow_web_server {

static const char *const TAG = "reflow_web_server";

// Static instance for C callback access
ReflowWebServer* ReflowWebServer::instance_ = nullptr;

void ReflowWebServer::setup() {
    ESP_LOGCONFIG(TAG, "Setting up Reflow Web Server...");
    
    instance_ = this;
    
#ifdef USE_ESP32
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = this->port_;
    config.max_uri_handlers = 10;
    config.stack_size = 8192;
    
    if (httpd_start(&this->server_, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler,
            .user_ctx = this
        };
        httpd_register_uri_handler(this->server_, &index_uri);
        
        httpd_uri_t temperature_data_uri = {
            .uri = "/temperature_data",
            .method = HTTP_GET,
            .handler = temperature_data_handler,
            .user_ctx = this
        };
        httpd_register_uri_handler(this->server_, &temperature_data_uri);
        
        httpd_uri_t style_uri = {
            .uri = "/style.css",
            .method = HTTP_GET,
            .handler = style_handler,
            .user_ctx = this
        };
        httpd_register_uri_handler(this->server_, &style_uri);
        
        httpd_uri_t script_uri = {
            .uri = "/script.js",
            .method = HTTP_GET,
            .handler = script_handler,
            .user_ctx = this
        };
        httpd_register_uri_handler(this->server_, &script_uri);
        
        httpd_uri_t switch_control_uri = {
            .uri = "/switch_control",
            .method = HTTP_POST,
            .handler = switch_control_handler,
            .user_ctx = this
        };
        httpd_register_uri_handler(this->server_, &switch_control_uri);
        
        ESP_LOGCONFIG(TAG, "Web Server started on port %u", this->port_);
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
#endif
    
    if (this->temperature_sensor_ != nullptr) {
        this->temperature_sensor_->add_on_state_callback([this](float state) {
            this->on_temperature_update(state);
        });
    }
}

void ReflowWebServer::loop() {
    // Nothing to do in loop for this component
}

void ReflowWebServer::dump_config() {
    ESP_LOGCONFIG(TAG, "Reflow Web Server:");
    ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
    ESP_LOGCONFIG(TAG, "  Authentication: %s", this->username_.empty() ? "NO" : "YES");
    if (this->temperature_sensor_ != nullptr) {
        ESP_LOGCONFIG(TAG, "  Temperature Sensor: %s", this->temperature_sensor_->get_name().c_str());
    }
    if (this->reflow_switch_ != nullptr) {
        ESP_LOGCONFIG(TAG, "  Reflow Switch: %s", this->reflow_switch_->get_name().c_str());
    }
}

float ReflowWebServer::get_setup_priority() const {
    return setup_priority::WIFI - 1.0f;
}

#ifdef USE_ESP32
esp_err_t ReflowWebServer::index_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    
    if (!server->authenticate_request(req)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Reflow Controller\"");
        httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t ReflowWebServer::temperature_data_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    
    if (!server->authenticate_request(req)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    std::string json = server->get_temperature_data_json();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

esp_err_t ReflowWebServer::style_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, STYLE_CSS, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t ReflowWebServer::script_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, SCRIPT_JS, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t ReflowWebServer::switch_control_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    
    if (!server->authenticate_request(req)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Bad Request", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    buf[ret] = '\0';
    
    // Parse JSON: {"state": true/false}
    bool new_state = strstr(buf, "true") != nullptr;
    
    if (server->reflow_switch_ != nullptr) {
        if (new_state) {
            server->reflow_switch_->turn_on();
        } else {
            server->reflow_switch_->turn_off();
        }
    }
    
    // Return current state
    std::string response = "{\"state\": ";
    response += (server->reflow_switch_ && server->reflow_switch_->state) ? "true" : "false";
    response += "}";
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}
#endif

std::string ReflowWebServer::get_temperature_data_json() {
    std::ostringstream json;
    json << "[";
    bool first = true;
    
    for (const auto &point : this->temperature_data_) {
        if (!first) {
            json << ",";
        }
        first = false;
        
        // Convert to JavaScript timestamp (milliseconds since epoch)
        uint64_t js_timestamp = static_cast<uint64_t>(point.timestamp) * 1000;
        json << "[" << js_timestamp << "," << point.temperature << "]";
    }
    
    json << "]";
    return json.str();
}

bool ReflowWebServer::authenticate_request(httpd_req_t *req) {
    if (this->username_.empty()) {
        return true;  // No authentication required
    }
    
#ifdef USE_ESP32
    char auth_header[256];
    size_t header_len = httpd_req_get_hdr_value_len(req, "Authorization");
    
    if (header_len == 0 || header_len >= sizeof(auth_header)) {
        return false;
    }
    
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth_header, sizeof(auth_header)) != ESP_OK) {
        return false;
    }
    
    // Basic authentication check (simplified)
    std::string auth_string = this->username_ + ":" + this->password_;
    std::vector<uint8_t> auth_bytes(auth_string.begin(), auth_string.end());
    std::string expected = "Basic " + esphome::base64_encode(auth_bytes);
    return std::string(auth_header) == expected;
#endif
    
    return false;
}

void ReflowWebServer::on_temperature_update(float state) {
    if (std::isnan(state)) {
        return;  // Skip NaN values
    }
    
    uint32_t now = millis() / 1000;  // Convert to seconds
    
    // Add new data point
    this->temperature_data_.push_back({now, state});
    
    // Remove old data points to keep memory usage reasonable
    while (this->temperature_data_.size() > MAX_DATA_POINTS) {
        this->temperature_data_.pop_front();
    }
}





}  // namespace reflow_web_server
}  // namespace esphome