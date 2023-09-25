#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>

#include "esphome.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/ble_client/ble_client.h"

#include "multimeter121gw_parser.h"

namespace esphome {
namespace multimeter_121gw {

class PacketListener {
public:
    virtual void packet_received(const packet& p) = 0;
};

class Multimeter121GWNode : public esphome::ble_client::BLEClientNode, 
                            public esphome::api::CustomAPIDevice {
public:
    Multimeter121GWNode(): handle_(0), packet_listener_(0) {}
    void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                        esp_ble_gattc_cb_param_t *param) override;
    void set_service_uuid(const  esphome::esp32_ble_tracker::ESPBTUUID& uuid) {
        service_uuid_ = uuid;
    }

    void register_listener(PacketListener* listener) {
        packet_listener_ = listener;
    }
private:
    uint16_t handle_;
    esphome::esp32_ble_tracker::ESPBTUUID service_uuid_;
    Parser parser_;
    PacketListener* packet_listener_;
};

class Multimeter121GW : public Component, public esphome::esp32_ble_tracker::ESPBTDeviceListener {
public:
    Multimeter121GW(): client_(0) {}

    void setup() override {};
    void loop() override {};
    void dump_config() override;

    bool parse_device(const esphome::esp32_ble_tracker::ESPBTDevice &device) override;

    void set_client(esphome::ble_client::BLEClient* cl) {
        client_ = cl;
        client_->register_ble_node(&client_node_);
    }

    void register_listener(PacketListener* listener) {
        client_node_.register_listener(listener);
    }
private:
    esphome::ble_client::BLEClient* client_;
    Multimeter121GWNode client_node_;
};

class PacketReceivedTrigger :  public Trigger<const packet&>, public PacketListener {
public:
    PacketReceivedTrigger(Multimeter121GW* parent) {
        parent->register_listener(this);
    }
    void packet_received(const packet& p) override {
        trigger(p);
    }
};


}
}