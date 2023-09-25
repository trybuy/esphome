import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import (CONF_ID, CONF_TRIGGER_ID)
from esphome.components import esp32_ble_tracker
from esphome.components import ble_client

DEPENDENCIES = ['esp32_ble_tracker', 'ble_client']

component_ns = cg.esphome_ns.namespace('multimeter_121gw')
component = component_ns.class_('Multimeter121GW', cg.Component, esp32_ble_tracker.ESPBTDeviceListener)
Packet = component_ns.class_("packet")
PacketConstRef = Packet.operator("ref").operator("const")
PacketReceivedTrigger = component_ns.class_(
    "PacketReceivedTrigger", automation.Trigger.template(PacketConstRef)
)

M121GW_BLE_CLIENT_ID = "ble_client_id"
CONF_ON_PACKET_RECEIVED = "on_packet_received"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(component),
    cv.Optional(CONF_ON_PACKET_RECEIVED): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PacketReceivedTrigger),
        }
    )
}).extend(cv.COMPONENT_SCHEMA).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(ble_client.BLE_CLIENT_SCHEMA)

async def to_code(config):
    cl = await cg.get_variable(config[M121GW_BLE_CLIENT_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)
    cg.add(var.set_client(cl))
    for conf in config.get(CONF_ON_PACKET_RECEIVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(PacketConstRef, "p")], conf)