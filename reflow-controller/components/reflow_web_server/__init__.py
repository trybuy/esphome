import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)
from esphome.components import web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.core import CORE
import os

DEPENDENCIES = ["network"]
AUTO_LOAD = ["web_server_base"]

reflow_web_server_ns = cg.esphome_ns.namespace("reflow_web_server")
ReflowWebServer = reflow_web_server_ns.class_("ReflowWebServer", cg.Component)

CONF_REFLOW_CURVE = "reflow_curve"
CONF_UPDATE_INTERVAL = "update_interval"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ReflowWebServer),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
        cv.Optional(CONF_REFLOW_CURVE): cv.use_id(cg.Component),
        cv.Optional(CONF_UPDATE_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)




async def to_code(config):
    paren = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    var = cg.new_Pvariable(config[CONF_ID], paren)
    await cg.register_component(var, config)
    
    if CONF_REFLOW_CURVE in config:
        reflow_curve = await cg.get_variable(config[CONF_REFLOW_CURVE])
        cg.add(var.set_reflow_curve(reflow_curve))
    
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    
    # Register the PlatformIO pre-script to generate web assets before compilation
    component_dir = os.path.dirname(os.path.abspath(__file__))
    script_path = os.path.join(component_dir, "gen_web_assets.py")
    
    # Add the pre-script to PlatformIO options automatically
    cg.add_platformio_option("extra_scripts", [f"pre:{script_path}"])