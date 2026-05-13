import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

# Добавляем ключ для конфига
CONF_SERIAL_NUMBER = "serial_number"

# Определяем пространство имен
mercury_ns = cg.esphome_ns.namespace("mercury")
Mercury = mercury_ns.class_("Mercury", cg.PollingComponent, uart.UARTDevice)

# Зависимости
DEPENDENCIES = ["uart"]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Mercury),
# Добавляем настройку серийного номера (по умолчанию 0)
    cv.Optional(CONF_SERIAL_NUMBER, default=0): cv.uint32_t,
}).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

# Передаем значение из YAML в C++ метод set_serial_number
    if CONF_SERIAL_NUMBER in config:
        cg.add(var.set_serial_number(config[CONF_SERIAL_NUMBER]))