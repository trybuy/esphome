#include "multimeter121gw_parser.h"

namespace esphome {
namespace multimeter_121gw {

bool Parser::validate_checksum(buffer_iterator start) {
    auto end = start + sizeof(packet) - 1;
    uint8_t sum = 0;
    for (auto it = start; it != end; ++it) {
        sum ^= *it;
    }
    // if (sum != *end) {
    //     ESP_LOGD("multimeter_121gw", "Wrong checksum: 0x%x vs 0x%x", sum, *end);
    //     return false;
    // }
    return sum == *end;
}

packet Parser::parse(buffer_iterator start) {
    const uint8_t* data = &(*start);
    packet res = *((packet*)data);
    res.value = (data[7] << 8) + data[8];
    res.sub_value = (data[11] << 8) + data[12];
    return res;
}

}
}