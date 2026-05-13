import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import Mercury, mercury_ns

CONF_MERCURY_ID = "mercury_id"

TEXT_SENSORS = {
    "datetime": text_sensor.text_sensor_schema(),
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MERCURY_ID): cv.use_id(Mercury),
}).extend({cv.Optional(key): schema for key, schema in TEXT_SENSORS.items()})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_MERCURY_ID])
    for key in TEXT_SENSORS:
        if key in config:
            conf = config[key]
            sens = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(parent, f"set_{key}_sensor")(sens))
