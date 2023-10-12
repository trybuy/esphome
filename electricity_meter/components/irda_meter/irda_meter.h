#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart_component_esp_idf.h"

namespace esphome {
namespace irda_meter {

struct meter_data {

};

enum state {
    IDLE = 0,
    WRITE_INIT,
    READING_INIT_RESPONSE,
    WRITE_GET,
    READING_GET_RESPONSE
};

const size_t BUFFER_SIZE = 512;

class IrdaMeter : public esphome::Component {
public: 
    IrdaMeter(esphome::uart::IDFUARTComponent* uart): uart_(uart), state_(IDLE), buffer_pos_(0), read_start_(0) {}
    void setup() override;
    void loop() override;

    void get_meter_data();
private:
    void write_array(const char *data, size_t len);
    void start_reading(uint8_t s);
    void read(uint8_t timeout_stat = IDLE);
private:
    esphome::uart::IDFUARTComponent* uart_;
    uint8_t state_;
    uint8_t buffer_[BUFFER_SIZE];
    size_t buffer_pos_;
    uint64_t read_start_;
};

template<typename... Ts> class IrdaMeterGetDataAction : public Action<Ts...>, public Parented<IrdaMeter> {
public:
    void play(Ts... x) override {
        this->parent_->get_meter_data();
    }
};

}
}