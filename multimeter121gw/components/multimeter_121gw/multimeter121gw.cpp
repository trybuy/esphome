#include "multimeter121gw.h"

namespace esphome {
namespace multimeter_121gw {
    static const char *TAG = "multimeter_121gw";
    // 121GW charasteristic "e7add780-b042-4876-aae1-112855353cc1"
    static const esphome::esp32_ble::ESPBTUUID M121GW_CHARASTERISTIC = 
        esphome::esp32_ble::ESPBTUUID::from_raw((const uint8_t[16]){
            0xC1,0x3C,0x35,0x55,0x28,0x11,0xE1,0xAA,
            0x76,0x48,0x42,0xB0,0x80,0xD7,0xAD,0xE7
        });

    void Multimeter121GWNode::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                    esp_ble_gattc_cb_param_t *param) {
        switch (event) {
            case ESP_GATTC_OPEN_EVT: {
                if (param->open.status == ESP_GATT_OK) {
                    ESP_LOGI(TAG, "Connected successfully!");
                }
                break;
            }
            case ESP_GATTC_DISCONNECT_EVT: {
                ESP_LOGW(TAG, "Disconnected!");
                break;
            }
            case ESP_GATTC_SEARCH_CMPL_EVT: {
                handle_ = 0;
                auto *chr = this->parent()->get_characteristic(service_uuid_, M121GW_CHARASTERISTIC);
                if (chr == nullptr) {
                    ESP_LOGW(TAG, "No sensor characteristic found at service %s char %s", service_uuid_.to_string().c_str(),
                            M121GW_CHARASTERISTIC.to_string().c_str());
                    break;
                }
                handle_ = chr->handle;
                auto status = esp_ble_gattc_register_for_notify(this->parent()->get_gattc_if(),
                                                                    this->parent()->get_remote_bda(), chr->handle);
                if (status) {
                    ESP_LOGW(TAG, "esp_ble_gattc_register_for_notify failed, status=%d", status);
                }
                break;
            }
            case ESP_GATTC_NOTIFY_EVT: {
                if (param->notify.conn_id != this->parent()->get_conn_id() || param->notify.handle != handle_)
                    break;

                packet result;
                if (parser_.handle(param->notify.value, param->notify.value_len, result)) {
                    if (packet_listener_) {
                        packet_listener_->packet_received(result);
                    }
                }
                break;
            }
            case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
                this->node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
                break;
            }
            default:
                break;
        }
    }

    void Multimeter121GW::dump_config(){
        ESP_LOGCONFIG(TAG, "Lsitening for BT charasteristic:  %s", M121GW_CHARASTERISTIC.to_string().c_str());
    }

    bool Multimeter121GW::parse_device(const esphome::esp32_ble_tracker::ESPBTDevice &device)  {
        if (!client_) {
            return false;
        }
        if (device.get_name() != "121GW" && address_ != device.address_uint64()) {
            return false;
        }
        ESP_LOGD(TAG, "Found 121GW device: %s", device.address_str().c_str());
        for (auto uuid : device.get_service_uuids()) {
            ESP_LOGD(TAG, "    - %s", uuid.to_string().c_str());
            client_node_.set_service_uuid(uuid);
        }
        client_->set_address(device.address_uint64());
        return false;
    }
}
}