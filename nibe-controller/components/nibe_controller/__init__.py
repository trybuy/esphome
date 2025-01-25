from esphome.core import ID
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    CONF_POWER,
    CONF_ENERGY,
    CONF_SEL_PIN,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_WATT,
    UNIT_KILOWATT_HOURS,
)

AUTO_LOAD = ["pulse_counter"]

nibe_ns = cg.esphome_ns.namespace('nibe')
BL0937 = nibe_ns.class_("BL0937")
BL0937Ref = BL0937.operator("ref")
NibeController = nibe_ns.class_('NibeController', cg.PollingComponent)


CF_PIN = "cf_pin"
CF1_PIN = "cf1_pin"
CONF_PHASE1 = "phase1"
CONF_PHASE2 = "phase2"
CONF_PHASE3 = "phase3"
CONF_VOLTAGE_COEFFICIENT = "voltage_coefficient"
CONF_CURRENT_COEFFICIENT = "current_coefficient"
CONF_POWER_COEFFICIENT = "power_coefficient"

PHASE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BL0937),
        cv.Required(CF_PIN): cv.All(pins.gpio_input_pin_schema),
        cv.Required(CF1_PIN): cv.All(pins.gpio_input_pin_schema),
        cv.Optional(CONF_VOLTAGE_COEFFICIENT): cv.positive_float,
        cv.Optional(CONF_CURRENT_COEFFICIENT): cv.positive_float,
        cv.Optional(CONF_POWER_COEFFICIENT): cv.positive_float,
        cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ENERGY): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(NibeController),
    cv.Required(CONF_PHASE1): cv.Schema(PHASE_SCHEMA),
    cv.Required(CONF_PHASE2): cv.Schema(PHASE_SCHEMA),
    cv.Required(CONF_PHASE3): cv.Schema(PHASE_SCHEMA),
    cv.Required(CONF_SEL_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    cv.Optional(CONF_ENERGY): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOWATT_HOURS,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
}).extend(cv.polling_component_schema("60s"))

async def phase_to_code(phase, config):
    cf = await cg.gpio_pin_expression(config[CF_PIN])
    cg.add(phase.set_cf_pin(cf))
    cf1 = await cg.gpio_pin_expression(config[CF1_PIN])
    cg.add(phase.set_cf1_pin(cf1))
    if CONF_VOLTAGE_COEFFICIENT in config:
        cg.add(phase.set_voltage_coefficient(config[CONF_VOLTAGE_COEFFICIENT]))
    if CONF_CURRENT_COEFFICIENT in config:
        cg.add(phase.set_current_coefficient(config[CONF_CURRENT_COEFFICIENT]))
    if CONF_POWER_COEFFICIENT in config:
        cg.add(phase.set_power_coefficient(config[CONF_POWER_COEFFICIENT]))
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(phase.set_voltage_sensor(sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(phase.set_current_sensor(sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(phase.set_power_sensor(sens))
    if CONF_ENERGY in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY])
        cg.add(phase.set_energy_sensor(sens))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    phase1 = cg.variable(ID(CONF_PHASE1, True, type=BL0937Ref), var.get_phase1())
    phase2 = cg.variable(ID(CONF_PHASE2, True, type=BL0937Ref), var.get_phase2())
    phase3 = cg.variable(ID(CONF_PHASE3, True, type=BL0937Ref), var.get_phase3())

    await phase_to_code(phase1, config[CONF_PHASE1])
    await phase_to_code(phase2, config[CONF_PHASE2])
    await phase_to_code(phase3, config[CONF_PHASE3])

    sel = await cg.gpio_pin_expression(config[CONF_SEL_PIN])
    cg.add(var.set_sel_pin(sel))

    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
    if CONF_ENERGY in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY])
        cg.add(var.set_energy_sensor(sens))

