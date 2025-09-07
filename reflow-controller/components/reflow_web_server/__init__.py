import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_PORT,
    CONF_AUTH,
    CONF_USERNAME,
    CONF_PASSWORD,
)
from esphome.components import sensor
from esphome.components import switch
from esphome.core import CORE
import os

DEPENDENCIES = ["network"]

reflow_web_server_ns = cg.esphome_ns.namespace("reflow_web_server")
ReflowWebServer = reflow_web_server_ns.class_("ReflowWebServer", cg.Component)

CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_REFLOW_SWITCH = "reflow_switch"
CONF_UPDATE_INTERVAL = "update_interval"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ReflowWebServer),
        cv.Optional(CONF_PORT, default=80): cv.port,
        cv.Optional(CONF_AUTH): cv.Schema(
            {
                cv.Required(CONF_USERNAME): cv.string_strict,
                cv.Required(CONF_PASSWORD): cv.string_strict,
            }
        ),
        cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_REFLOW_SWITCH): cv.use_id(switch.Switch),
        cv.Optional(CONF_UPDATE_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)




async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_port(config[CONF_PORT]))
    
    if CONF_AUTH in config:
        cg.add(var.set_username(config[CONF_AUTH][CONF_USERNAME]))
        cg.add(var.set_password(config[CONF_AUTH][CONF_PASSWORD]))
    
    if CONF_TEMPERATURE_SENSOR in config:
        temp_sensor = await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])
        cg.add(var.set_temperature_sensor(temp_sensor))
    
    if CONF_REFLOW_SWITCH in config:
        reflow_switch = await cg.get_variable(config[CONF_REFLOW_SWITCH])
        cg.add(var.set_reflow_switch(reflow_switch))
    
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    
    # Try to find and set the time component automatically
    for comp in CORE.config.get("time", []):
        time_comp_id = comp.get(CONF_ID)
        if time_comp_id:
            time_comp = await cg.get_variable(time_comp_id)
            cg.add(var.set_time_component(time_comp))
            break  # Use the first time component found

    # Register the PlatformIO pre-script to generate web assets before compilation
    component_dir = os.path.dirname(os.path.abspath(__file__))
    script_path = os.path.join(component_dir, "gen_web_assets.py")
    
    # Add the pre-script to PlatformIO options automatically
    cg.add_platformio_option("extra_scripts", [f"pre:{script_path}"])
    # cg.add_platformio_option("extra_scripts", ["pre:../../../components/reflow_web_server/gen_web_assets.py"])

    # No external libraries needed - using ESP-IDF's built-in HTTP server