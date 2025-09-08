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
from esphome.core import CORE
import os

DEPENDENCIES = ["network"]

reflow_web_server_ns = cg.esphome_ns.namespace("reflow_web_server")
ReflowWebServer = reflow_web_server_ns.class_("ReflowWebServer", cg.Component)

CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_REFLOW_CURVE = "reflow_curve"
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
        cv.Optional(CONF_REFLOW_CURVE): cv.use_id(cg.Component),
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
    
    
    if CONF_REFLOW_CURVE in config:
        reflow_curve = await cg.get_variable(config[CONF_REFLOW_CURVE])
        cg.add(var.set_reflow_curve(reflow_curve))
    
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    

    # Register the PlatformIO pre-script to generate web assets before compilation
    component_dir = os.path.dirname(os.path.abspath(__file__))
    script_path = os.path.join(component_dir, "gen_web_assets.py")
    
    # Add the pre-script to PlatformIO options automatically
    cg.add_platformio_option("extra_scripts", [f"pre:{script_path}"])
    # cg.add_platformio_option("extra_scripts", ["pre:../../../components/reflow_web_server/gen_web_assets.py"])

    # No external libraries needed - using ESP-IDF's built-in HTTP server