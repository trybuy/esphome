#include "reflow_web_server.h"
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
    httpd_resp_send(req, server->get_index_html(), HTTPD_RESP_USE_STRLEN);
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
    httpd_resp_send(req, server->get_style_css(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t ReflowWebServer::script_handler(httpd_req_t *req) {
    ReflowWebServer* server = static_cast<ReflowWebServer*>(req->user_ctx);
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, server->get_script_js(), HTTPD_RESP_USE_STRLEN);
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

const char* ReflowWebServer::get_index_html() {
    return R"html(<!DOCTYPE html>
<html>
<head>
    <title>Reflow Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://code.highcharts.com/highcharts.js"></script>
    <script src="https://code.highcharts.com/modules/boost.js"></script>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Reflow Controller</h1>
            <div class="subtitle">Advanced Temperature Monitoring & Control</div>
        </div>
        
        <div class="dashboard">
            <div class="status-grid" id="statusGrid">
                <div class="status-card switch">
                    <button class="switch-button" id="reflowSwitchBtn" onclick="toggleReflowSwitch()">
                        <span id="switchState">OFF</span>
                    </button>
                </div>
                <div class="status-card temperature">
                    <div class="status-value" id="currentTemp">-- &deg;C</div>
                </div>
            </div>
            
            <div class="chart-container">
                <div class="chart-title">Temperature Profile</div>
                <div id="temperatureChart"></div>
            </div>
        </div>
    </div>
    <script src="/script.js"></script>
</body>
</html>)html";
}

const char* ReflowWebServer::get_style_css() {
    return R"(/* Enhanced Reflow Controller Web Interface */
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    margin: 0;
    padding: 20px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    color: #333;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    background: rgba(255, 255, 255, 0.95);
    border-radius: 15px;
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
    overflow: hidden;
}

.header {
    background: linear-gradient(135deg, #ff6b6b 0%, #ee5a24 100%);
    color: white;
    padding: 30px;
    text-align: center;
    position: relative;
}

.header h1 {
    margin: 0;
    font-size: 2.5em;
    font-weight: 300;
}

.header .subtitle {
    margin-top: 10px;
    font-size: 1.1em;
    opacity: 0.9;
}

.dashboard {
    padding: 30px;
}

.status-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 20px;
    margin-bottom: 30px;
}

.status-card {
    background: white;
    border-radius: 12px;
    padding: 25px;
    box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);
    border-left: 4px solid;
    transition: transform 0.3s ease, box-shadow 0.3s ease;
}

.status-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 10px 25px rgba(0, 0, 0, 0.15);
}

.status-card.temperature {
    border-left-color: #e74c3c;
}

.status-card.switch {
    border-left-color: #3498db;
}

.status-label {
    font-size: 0.9em;
    color: #7f8c8d;
    margin-bottom: 10px;
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 600;
}

.status-value {
    font-size: 2.2em;
    font-weight: 700;
    color: #2c3e50;
}

.chart-container {
    background: white;
    border-radius: 12px;
    padding: 25px;
    box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);
}

.chart-title {
    font-size: 1.4em;
    font-weight: 600;
    color: #2c3e50;
    margin-bottom: 20px;
    text-align: center;
}

#temperatureChart {
    height: 400px;
    width: 100%;
}

.switch-button {
    background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%);
    color: white;
    border: none;
    padding: 12px 24px;
    border-radius: 25px;
    cursor: pointer;
    font-weight: 600;
    font-size: 1.1em;
    transition: all 0.3s ease;
    text-transform: uppercase;
    letter-spacing: 1px;
    box-shadow: 0 4px 15px rgba(231, 76, 60, 0.3);
}

.switch-button:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba(231, 76, 60, 0.4);
}

.switch-button.on {
    background: linear-gradient(135deg, #27ae60 0%, #2ecc71 100%);
    box-shadow: 0 4px 15px rgba(46, 204, 113, 0.3);
}

.switch-button.on:hover {
    box-shadow: 0 6px 20px rgba(46, 204, 113, 0.4);
}

@media (max-width: 768px) {
    body {
        padding: 10px;
    }
    
    .header h1 {
        font-size: 2em;
    }
    
    .dashboard {
        padding: 20px;
    }
    
    .status-grid {
        grid-template-columns: 1fr;
    }
    
    #temperatureChart {
        height: 300px;
    }
})";
}

const char* ReflowWebServer::get_script_js() {
    return R"(// Reflow Controller Dashboard JavaScript
class ReflowDashboard {
    constructor() {
        this.temperatureChart = null;
        this.updateInterval = 1000;
        this.init();
    }
    
    init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }
    
    setup() {
        this.initializeChart();
        this.startDataUpdates();
    }
    
    initializeChart() {
        this.temperatureChart = Highcharts.chart('temperatureChart', {
            chart: {
                type: 'spline',
                animation: false,
                backgroundColor: 'transparent'
            },
            title: { text: null },
            xAxis: {
                type: 'datetime',
                title: { text: 'Time' }
            },
            yAxis: {
                title: { text: 'Temperature (&deg;C)' },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#808080'
                }]
            },
            tooltip: {
                formatter: function() {
                    return '<b>Temperature</b><br/>' +
                           Highcharts.dateFormat('%H:%M:%S', this.x) + '<br/>' +
                           '<span style="color: #e74c3c; font-weight: bold;">' +
                           this.y.toFixed(1) + ' &deg;C</span>';
                }
            },
            legend: { enabled: false },
            plotOptions: {
                spline: {
                    marker: { enabled: false },
                    lineWidth: 3,
                    color: '#e74c3c'
                }
            },
            series: [{
                name: 'Temperature',
                data: []
            }],
            credits: { enabled: false }
        });
    }
    
    updateTemperatureData() {
        fetch('/temperature_data')
            .then(response => response.json())
            .then(data => {
                if (data && data.length > 0) {
                    this.temperatureChart.series[0].setData(data, true);
                    
                    const latest = data[data.length - 1];
                    if (latest) {
                        document.getElementById('currentTemp').innerHTML = latest[1].toFixed(1) + ' &deg;C';
                    }
                }
            })
            .catch(error => {
                console.error('Error fetching temperature data:', error);
            });
    }
    
    startDataUpdates() {
        this.updateTemperatureData();
        setInterval(() => this.updateTemperatureData(), this.updateInterval);
    }
}

// Initialize dashboard
new ReflowDashboard();

// Switch control function
function toggleReflowSwitch() {
    const btn = document.getElementById('reflowSwitchBtn');
    const stateSpan = document.getElementById('switchState');
    const currentState = btn.classList.contains('on');
    const newState = !currentState;
    
    fetch('/switch_control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ state: newState })
    })
    .then(response => response.json())
    .then(data => {
        if (data.state) {
            btn.classList.add('on');
            stateSpan.textContent = 'ON';
        } else {
            btn.classList.remove('on');
            stateSpan.textContent = 'OFF';
        }
    })
    .catch(error => {
        console.error('Error controlling switch:', error);
    });
})";
}


}  // namespace reflow_web_server
}  // namespace esphome