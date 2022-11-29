#Автор кода https://github.com/Brokly/ESPHome-Mercury230
#My telegram https://t.me/DieMetRik

#import logging
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import automation, pins
from esphome.components import sensor, text_sensor, uart
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_ID,
    CONF_ENERGY,
    CONF_PIN,
    CONF_UART_ID,
    CONF_CURRENT,
    CONF_DATA,
    STATE_CLASS_MEASUREMENT,
    CONF_VALUE,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_STEP,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ACCURACY_DECIMALS,
    CONF_FREQUENCY,
    DEVICE_CLASS_FREQUENCY,
    UNIT_HERTZ,
    CONF_VOLTAGE,
    #CONF_IMPORT,
    DEVICE_CLASS_VOLTAGE,
    UNIT_VOLT,
    CONF_PHASE_ANGLE,
    UNIT_DEGREES,
    CONF_CURRENT,
    DEVICE_CLASS_CURRENT,
    UNIT_AMPERE,
    #ICON_CURRENT_AC,
    CONF_POWER,
    DEVICE_CLASS_POWER,
    UNIT_WATT,
    #ICON_POWER,
    UNIT_EMPTY,
    CONF_POWER_FACTOR,
    DEVICE_CLASS_POWER_FACTOR,
    CONF_IMPORT_ACTIVE_ENERGY,
    CONF_IMPORT_REACTIVE_ENERGY,
    UNIT_KILOWATT_HOURS,
    STATE_CLASS_TOTAL_INCREASING,
    DEVICE_CLASS_ENERGY,
    CONF_USE_ADDRESS,
    CONF_PASSWORD,
    CONF_UPDATE_INTERVAL,
)

#_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@Brokly"]
DEPENDENCIES = ["sensor", "text_sensor", "uart"]
AUTO_LOAD = ["output"]

CONF_ACTIVE_LED_PIN = "active_led_pin"
CONF_FIRM_VERSION = "firmware_version"
ICON_FIRM_VERSION = "mdi:select-inverse"
CONF_SERIAL_NUMBER = "serial_number"
ICON_SERIAL_NUMBER = "mdi:numeric"
CONF_CONNECT_STATUS = "connect_status"
ICON_CONNECT_STATUS = "mdi:lan-disconnect"
CONF_DATA_FABRICATE = "date_fabricate"
ICON_DATA_FABRICATE = "mdi:factory"
ICON_PHASE_ANGLE = "mdi:alpha"
#ICON_RATIO = "mdi:alpha-r-circle-outline"
ICON_FREQUENCY = "mdi:sine-wave"
CONF_ADMIN="admin"
CONF_PASS_HEX="pass_in_hex"
SUMM = "_summ"
PhA = "_a"
PhB = "_b"
PhC = "_c"
DirAA = "_aa"
DirRA = "_ra"
DirAB = "_ab"
DirRB = "_rb"
DirAC = "_ac"
DirRC = "_rc"

VALID_password_CHARACTERS = (
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
)

Mercury230_ns = cg.esphome_ns.namespace("energy_meter_mercury230")
Mercury230 = Mercury230_ns.class_("Mercury", sensor.Sensor, cg.Component)

def output_info(config):
    #_LOGGER.info(config)
    return config

def validate_password(value):
    value = cv.string_strict(value)
    if not value:
        return value
    if len(value) < 6:
        raise cv.Invalid("Password must be at 6 characters long")
    if len(value) > 6:
        raise cv.Invalid("Password must be at 6 characters long")
    for char in value:
        if char not in VALID_password_CHARACTERS:
            raise cv.Invalid(
                f"Password must only consist of upper/lowercase characters and numbers. The character '{char}' cannot be used"
            )
    return value

def validate_update_interval(value):
    value = cv.positive_time_period_milliseconds(value)
    if value < cv.time_period("5s"):
        raise cv.Invalid(
            "Update interval must be greater than or equal to 5 seconds if set."
        )
    if value > cv.time_period("30min"):
        raise cv.Invalid(
            "The update interval must be greater than or equal to 30 minutes if set."
        )
    return value
 
#шаблон сенсора напряжений
voltSensor=sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_VOLTAGE,
        unit_of_measurement=UNIT_VOLT,
        #icon=???,
        accuracy_decimals=2,
    )
#шаблон сенсора мощности
powerSensor=sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_POWER,
        unit_of_measurement=UNIT_WATT,
        #icon=ICON_POWER,
        accuracy_decimals=2,
    )
#шаблон сенсора тока
currentSensor=sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_CURRENT,
        unit_of_measurement=UNIT_AMPERE,
        #icon=ICON_CURRENT_AC,
        accuracy_decimals=2,
    )
#шаблон сенсора углов
angleSensor=sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        unit_of_measurement=UNIT_DEGREES,
        icon=ICON_PHASE_ANGLE,
        accuracy_decimals=2,
    )
#шаблон сенсора коефициентов
ratioSensor=sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_POWER_FACTOR,
        unit_of_measurement=UNIT_EMPTY,
        #icon=ICON_RATIO,
        accuracy_decimals=2,
    )

initParams = {
     cv.GenerateID(): cv.declare_id(Mercury230),
     # частота
     cv.Optional(CONF_FREQUENCY): sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        accuracy_decimals=1,
        #device_class=DEVICE_CLASS_FREQUENCY,
        state_class=STATE_CLASS_MEASUREMENT,
        icon=ICON_FREQUENCY,
     ),

     # активная энергия
     cv.Optional(CONF_ENERGY+DirAA): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),
    # реактивная энергия
     cv.Optional(CONF_ENERGY+DirRA): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),
     # активная энергия
     cv.Optional(CONF_ENERGY+DirAB): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),
    # реактивная энергия
     cv.Optional(CONF_ENERGY+DirRB): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),
     # активная энергия
     cv.Optional(CONF_ENERGY+DirAC): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),
    # реактивная энергия
     cv.Optional(CONF_ENERGY+DirRC): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOWATT_HOURS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
     ),


     # версия прошивки устройства
     cv.Optional(CONF_FIRM_VERSION): text_sensor.text_sensor_schema(
        icon=ICON_FIRM_VERSION,
     ),
     # серийник устройства
     cv.Optional(CONF_SERIAL_NUMBER): text_sensor.text_sensor_schema(
        icon=ICON_SERIAL_NUMBER,
     ),
     # статус соединения 
     cv.Optional(CONF_CONNECT_STATUS): text_sensor.text_sensor_schema(
        icon=ICON_CONNECT_STATUS,
     ),
     # дата изготовления
     cv.Optional(CONF_DATA_FABRICATE): text_sensor.text_sensor_schema(
        icon=ICON_DATA_FABRICATE,
     ),
     # нога светодиода индикации связи
     cv.Optional(CONF_ACTIVE_LED_PIN ): pins.gpio_output_pin_schema,
     # пароль для подключения
     cv.Optional(CONF_PASSWORD): validate_password,
     # пароль в HEX 
     cv.Optional(CONF_PASS_HEX, default=False): cv.boolean, 
     # тип пароля админский или нет
     cv.Optional(CONF_ADMIN, default=False): cv.boolean, 
     # адрес счетчика
     cv.Optional(CONF_USE_ADDRESS): cv.int_range(min=1, max=240),
     # update interval
     cv.Optional(CONF_UPDATE_INTERVAL, default="30sec"): validate_update_interval,
}




# вольты
initParams[cv.Optional(CONF_VOLTAGE+PhA)] = voltSensor;
initParams[cv.Optional(CONF_VOLTAGE+PhB)] = voltSensor;
initParams[cv.Optional(CONF_VOLTAGE+PhC)] = voltSensor;
# мощности
initParams[cv.Optional(CONF_POWER+PhA)] = powerSensor;
initParams[cv.Optional(CONF_POWER+PhB)] = powerSensor;
initParams[cv.Optional(CONF_POWER+PhC)] = powerSensor;
initParams[cv.Optional(CONF_POWER+SUMM)] = powerSensor;
# токи
initParams[cv.Optional(CONF_CURRENT+PhA)] = currentSensor;
initParams[cv.Optional(CONF_CURRENT+PhB)] = currentSensor;
initParams[cv.Optional(CONF_CURRENT+PhC)] = currentSensor;
initParams[cv.Optional(CONF_CURRENT+SUMM)] = currentSensor;
# углы
initParams[cv.Optional(CONF_PHASE_ANGLE+PhA)] = angleSensor;
initParams[cv.Optional(CONF_PHASE_ANGLE+PhB)] = angleSensor;
initParams[cv.Optional(CONF_PHASE_ANGLE+PhC)] = angleSensor;
# коэфициенты
initParams[cv.Optional(CONF_POWER_FACTOR+PhA)] = ratioSensor;
initParams[cv.Optional(CONF_POWER_FACTOR+PhB)] = ratioSensor;
initParams[cv.Optional(CONF_POWER_FACTOR+PhC)] = ratioSensor;

CONFIG_SCHEMA = cv.All(sensor.SENSOR_SCHEMA.extend(initParams).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA), output_info)

async def to_code(config):
    #_LOGGER.info("--------------")
    #_LOGGER.info(config)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    #uart 
    parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.initUart(parent))
    # прошивка
    if (CONF_FIRM_VERSION) in config:
        sens = await text_sensor.new_text_sensor(config[CONF_FIRM_VERSION])
        cg.add(var.set_vers_string(sens))
    # connect status
    if (CONF_CONNECT_STATUS) in config:
        sens = await text_sensor.new_text_sensor(config[CONF_CONNECT_STATUS])
        cg.add(var.set_error_string(sens))
    # дата изготовления
    if (CONF_DATA_FABRICATE) in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DATA_FABRICATE])
        cg.add(var.set_fab_date_string(sens))
    # серийник устройства
    if (CONF_SERIAL_NUMBER) in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SERIAL_NUMBER])
        cg.add(var.set_sn_string(sens))
    #нога светодиода сигнализации
    if (CONF_ACTIVE_LED_PIN ) in config:
        pin = await cg.gpio_pin_expression(config[CONF_ACTIVE_LED_PIN])
        cg.add(var.set_active_pin(pin))
    # частота
    if (CONF_FREQUENCY) in config:
        sens = await sensor.new_sensor(config[CONF_FREQUENCY])
        cg.add(var.set_Freq(sens))
        


    # активная энергия
    if (CONF_ENERGY+DirAA) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirAA])
        cg.add(var.set_ValueAA(sens))
    # реактивная энергия
    if (CONF_ENERGY+DirRA) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirRA])
        cg.add(var.set_ValueRA(sens))

    # активная энергия
    if (CONF_ENERGY+DirAB) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirAB])
        cg.add(var.set_ValueAB(sens))
    # реактивная энергия
    if (CONF_ENERGY+DirRB) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirRB])
        cg.add(var.set_ValueRB(sens))
    # активная энергия
    if (CONF_ENERGY+DirAC) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirAC])
        cg.add(var.set_ValueAC(sens))
    # реактивная энергия
    if (CONF_ENERGY+DirRC) in config:
        sens = await sensor.new_sensor(config[CONF_ENERGY+DirRC])
        cg.add(var.set_ValueRC(sens))


       
    if (CONF_VOLTAGE+PhA) in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE+PhA])
        cg.add(var.set_VoltA(sens))
    if (CONF_VOLTAGE+PhB) in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE+PhB])
        cg.add(var.set_VoltB(sens))
    if (CONF_VOLTAGE+PhC) in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE+PhC])
        cg.add(var.set_VoltC(sens))
    # мощности
    if (CONF_POWER+PhA) in config:
        sens = await sensor.new_sensor(config[CONF_POWER+PhA])
        cg.add(var.set_WattA(sens))
    if (CONF_POWER+PhB) in config:
        sens = await sensor.new_sensor(config[CONF_POWER+PhB])
        cg.add(var.set_WattB(sens))
    if (CONF_POWER+PhC) in config:
        sens = await sensor.new_sensor(config[CONF_POWER+PhC])
        cg.add(var.set_WattC(sens))
    if (CONF_POWER+SUMM) in config:
        sens = await sensor.new_sensor(config[CONF_POWER+SUMM])
        cg.add(var.set_Watts(sens))
    # токи
    if (CONF_CURRENT+PhA) in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT+PhA])
        cg.add(var.set_AmpA(sens))
    if (CONF_CURRENT+PhB) in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT+PhB])
        cg.add(var.set_AmpB(sens))
    if (CONF_CURRENT+PhC) in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT+PhC])
        cg.add(var.set_AmpC(sens))
    if (CONF_CURRENT+SUMM) in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT+SUMM])
        cg.add(var.set_Amps(sens))
    # углы
    if (CONF_PHASE_ANGLE+PhA) in config:
        sens = await sensor.new_sensor(config[CONF_PHASE_ANGLE+PhA])
        cg.add(var.set_AngleA(sens))
    if (CONF_PHASE_ANGLE+PhB) in config:
        sens = await sensor.new_sensor(config[CONF_PHASE_ANGLE+PhB])
        cg.add(var.set_AngleB(sens))
    if (CONF_PHASE_ANGLE+PhC) in config:
        sens = await sensor.new_sensor(config[CONF_PHASE_ANGLE+PhC])
        cg.add(var.set_AngleC(sens))
    # коэфициенты
    if (CONF_POWER_FACTOR+PhA) in config:
        sens = await sensor.new_sensor(config[CONF_POWER_FACTOR+PhA])
        cg.add(var.set_RatioA(sens))
    if (CONF_POWER_FACTOR+PhB) in config:
        sens = await sensor.new_sensor(config[CONF_POWER_FACTOR+PhB])
        cg.add(var.set_RatioB(sens))
    if (CONF_POWER_FACTOR+PhC) in config:
        sens = await sensor.new_sensor(config[CONF_POWER_FACTOR+PhC])
        cg.add(var.set_RatioC(sens))
    
    # пароли
    if (CONF_PASSWORD ) in config:
        cg.add(var.set_pass(config[CONF_PASSWORD]))
    # уровень доступа  
    if (CONF_ADMIN ) in config:
        cg.add(var.set_admin(config[CONF_ADMIN]))
    # пароль в HEX  
    if (CONF_PASS_HEX ) in config:
        cg.add(var.set_hex_pass(config[CONF_PASS_HEX]))
    # адрес счетчика
    if (CONF_USE_ADDRESS) in config:
        cg.add(var.set_useraddr(config[CONF_USE_ADDRESS]))
    # update intrerval
    if (CONF_UPDATE_INTERVAL) in config:
        cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

