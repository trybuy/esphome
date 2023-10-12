#include <algorithm>

#include "irda_meter.h"
#include "hal/uart_ll.h"

namespace esphome {
namespace irda_meter {


static const char *const TAG = "irda_meter";
static const uint64_t READ_TIMEOUT_IN_MICROSECONDS = 10 * 1000000; // 10 seconds


void enable_irda_tx(uart_port_t uart_num, bool enable) {
    auto uart_ctx = UART_LL_GET_HW(uart_num);
    //uart_ctx->conf0.irda_tx_en = enable;
}

void IrdaMeter::setup() {
    // esp_err_t err = uart_set_mode(uart_->get_hw_serial_number(), UART_MODE_IRDA);
    // if (err != ESP_OK) {
    //     ESP_LOGW(TAG, "uart_set_mode failed: %s", esp_err_to_name(err));
    //     this->mark_failed();
    // }

    uint32_t invert = 0;
    invert |= UART_SIGNAL_TXD_INV;
    //invert |= UART_SIGNAL_IRDA_RX_INV;

    esp_err_t err = uart_set_line_inverse(uart_->get_hw_serial_number(), invert);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "uart_set_line_inverse failed: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
    }
    state_ = IDLE;
}

void IrdaMeter::loop() {
    switch (state_) {
        case WRITE_INIT:
            ESP_LOGD(TAG, "Writing INIT");
            //write_array("\x2F\x3F\x21\x0D\x0A", 5);
            write_array("/?!\r\n", 5);
            start_reading(READING_INIT_RESPONSE);
            break;
        case READING_INIT_RESPONSE:
            read(WRITE_INIT);
            if (buffer_pos_ > 64) {
                 state_ = WRITE_GET;
            }
            break;
        case WRITE_GET:
            ESP_LOGD(TAG, "Writing GET");
            write_array("\x06\x30\x30\x30\x0D\x0A", 6);
            start_reading(READING_GET_RESPONSE);
            break;
        case READING_GET_RESPONSE:
            read();
            if (buffer_pos_ >= BUFFER_SIZE) {
                 state_ = IDLE;
            }
            break;
        case IDLE:
        default:
            break;
    }
}

void IrdaMeter::write_array(const char *data, size_t len) {
    enable_irda_tx(uart_->get_hw_serial_number(), true);
    uart_->write_array((const uint8_t *)data, len);
    uart_->flush();
    enable_irda_tx(uart_->get_hw_serial_number(), false);
}

void IrdaMeter::read(uint8_t timeout_state) {
    uint64_t passed = esp_timer_get_time() - read_start_;
    if (passed > READ_TIMEOUT_IN_MICROSECONDS) {
        ESP_LOGW(TAG, "Read timeout, state: %d", state_);
        state_ = timeout_state;
        return;
    }

    size_t available_len = uart_->available();
    if (!available_len) {
        return;
    }
    size_t to_read = std::min(available_len, BUFFER_SIZE - buffer_pos_);
    if(uart_->read_array(buffer_ + buffer_pos_, to_read)) {
        buffer_pos_ += to_read;
        buffer_[buffer_pos_] = 0;
        ESP_LOGD(TAG, "got data: %s", buffer_);
    }
}

void IrdaMeter::get_meter_data() {
    if (state_ != IDLE) {
        ESP_LOGW(TAG, "Can't get data, already processing in state: %d", state_);
        return; 
    }
    state_ = WRITE_INIT;
}

void IrdaMeter::start_reading(uint8_t s) {
    state_ = s;
    buffer_pos_ = 0;
    read_start_ =  esp_timer_get_time();
}

}
}