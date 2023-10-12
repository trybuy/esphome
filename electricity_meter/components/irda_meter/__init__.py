import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (CONF_ID)
from esphome.components import uart
from esphome import automation

DEPENDENCIES = ['uart']

component_ns = cg.esphome_ns.namespace('irda_meter')
component = component_ns.class_('IrdaMeter', cg.Component)
IrdaMeterGetDataAction = component_ns.class_("IrdaMeterGetDataAction", automation.Action)

UART_ID = "uart_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(component)
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    cl = await cg.get_variable(config[UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], cl)
    await cg.register_component(var, config)

@automation.register_action(
    "irda_meter.get_data",
    IrdaMeterGetDataAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(component)
        }
    ),
)
async def irda_meter_get_data_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
