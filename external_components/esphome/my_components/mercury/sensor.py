import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, UNIT_VOLT, UNIT_AMPERE, UNIT_WATT, 
    UNIT_KILOWATT_HOURS, UNIT_HERTZ, DEVICE_CLASS_ENERGY, 
    STATE_CLASS_TOTAL_INCREASING, ICON_COUNTER
)
from . import Mercury, mercury_ns

# Определяем ключи для YAML
CONF_MERCURY_ID = "mercury_id"
SENSORS = {
    "voltage": sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, accuracy_decimals=1),
    "current": sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=2),
    "power": sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=2),
    "t1": sensor.sensor_schema(unit_of_measurement=UNIT_KILOWATT_HOURS, accuracy_decimals=2, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING, icon=ICON_COUNTER),
    "t2": sensor.sensor_schema(unit_of_measurement=UNIT_KILOWATT_HOURS, accuracy_decimals=2, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING, icon=ICON_COUNTER),
    "t3": sensor.sensor_schema(unit_of_measurement=UNIT_KILOWATT_HOURS, accuracy_decimals=2, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING, icon=ICON_COUNTER),
    "total": sensor.sensor_schema(unit_of_measurement=UNIT_KILOWATT_HOURS, accuracy_decimals=2, icon=ICON_COUNTER),
    "frequency": sensor.sensor_schema(unit_of_measurement=UNIT_HERTZ, accuracy_decimals=2),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MERCURY_ID): cv.use_id(Mercury),
}).extend({cv.Optional(key): schema for key, schema in SENSORS.items()})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_MERCURY_ID])
    for key in SENSORS:
        if key in config:
            conf = config[key]
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(parent, f"set_{key}_sensor")(sens))
