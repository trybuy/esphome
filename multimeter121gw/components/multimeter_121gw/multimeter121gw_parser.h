#pragma once

#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>


namespace esphome {
namespace multimeter_121gw {

const uint8_t START = 0xf2;

enum Mode {
    Low_Z = 0,
    DCV = 1,
    ACV = 2,
    DCmV = 3,
    ACmV = 4,
    Temp = 5,
    Hz = 6,
    mS = 7,
    Duty = 8,
    Resistor = 9,
    Continuity = 10,
    Diode = 11,
    Capacitor = 12,
    ACuVA = 13,
    ACmVA = 14,
    ACVA = 15,
    ACuA = 16,
    DCuA = 17,
    ACmA = 18,
    DCmA = 19,
    ACA = 20,
    DCA = 21,
    DCuVA = 22,
    DCmVA = 23,
    DCVA = 24
};

#pragma pack (1)
struct packet {
    uint8_t start;
    uint32_t serial;
    uint8_t mode;
    uint8_t range;
    uint16_t value;
    uint8_t sub_mode;
    uint8_t sub_range;
    uint16_t sub_value;
    uint8_t bar_status;
    uint8_t bar_value;
    uint8_t icon_status1;
    uint8_t icon_status2;
    uint8_t icon_status3;
    uint8_t checksum;
};
#pragma pack (0)

inline std::ostream& operator <<(std::ostream& os, const packet& p) {
    os << "packet{0x" << std::hex << p.serial; 
    os << std::hex << ",0x" << (unsigned int)p.mode << ",0x" <<  (unsigned int)p.range << ',' <<  std::dec << (uint16_t)p.value;
    os << std::hex << ",0x" << (unsigned int)p.sub_mode << ",0x" <<  (unsigned int)p.sub_range << ',' <<  std::dec << (uint16_t)p.sub_value;
    os << std::hex << ",0x" << p.checksum  << '}';
    return os;
}

typedef std::vector<uint8_t>::const_iterator buffer_iterator;

class Parser {
public:
    bool handle(const uint8_t* data, uint16_t len, packet& result) {
        buffer_.insert(buffer_.end(), data, data + len);
        auto start_it = buffer_.begin();
        bool parsed = false;
        while ((start_it = std::find(start_it, buffer_.end(), START)) != buffer_.end() &&
                (buffer_.end() - start_it) >= sizeof(packet)) {
            if (validate_checksum(start_it)) {
                result = parse(start_it);
                start_it += sizeof(packet);
                parsed = true;
            } else {
                ++start_it;
            }
        }
        buffer_.erase(buffer_.begin(), start_it);
        return parsed;
    }
private:
    bool validate_checksum(buffer_iterator start);
    packet parse(buffer_iterator start);
private:
    std::vector<uint8_t> buffer_;
};

}
}