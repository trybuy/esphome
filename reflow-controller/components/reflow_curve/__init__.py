import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)
from esphome.components import switch, sensor
from esphome.core import CORE
import os

DEPENDENCIES = []

reflow_curve_ns = cg.esphome_ns.namespace("reflow_curve")
ReflowCurve = reflow_curve_ns.class_("ReflowCurve", cg.Component)

CONF_REFLOW_SWITCH = "reflow_switch"
CONF_TEMPERATURE_SENSOR = "temperature_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ReflowCurve),
        cv.Optional(CONF_REFLOW_SWITCH): cv.use_id(switch.Switch),
        cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_REFLOW_SWITCH in config:
        reflow_switch = await cg.get_variable(config[CONF_REFLOW_SWITCH])
        cg.add(var.set_reflow_switch(reflow_switch))
    
    if CONF_TEMPERATURE_SENSOR in config:
        temp_sensor = await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])
        cg.add(var.set_temperature_sensor(temp_sensor))

    # Try to find and set the time component automatically
    for comp in CORE.config.get("time", []):
        time_comp_id = comp.get(CONF_ID)
        if time_comp_id:
            time_comp = await cg.get_variable(time_comp_id)
            cg.add(var.set_time_component(time_comp))
            break  # Use the first time component found

    # Register the PlatformIO pre-script to generate reflow profile data before compilation
    component_dir = os.path.dirname(os.path.abspath(__file__))
    script_path = os.path.join(component_dir, "gen_reflow_profile.py")
    
    # Add the pre-script to PlatformIO options automatically
    cg.add_platformio_option("extra_scripts", [f"pre:{script_path}"])
