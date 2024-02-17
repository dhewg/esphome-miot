import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, time
from esphome.const import CONF_ID, CONF_TIME_ID

DEPENDENCIES = ["uart"]

miot_ns = cg.esphome_ns.namespace("miot")
Miot = miot_ns.class_("Miot", cg.Component, uart.UARTDevice)

CONF_MIOT_ID = "miot_id"
CONF_MIOT_HEARTBEAT_SIID = "miot_heartbeat_siid"
CONF_MIOT_HEARTBEAT_PIID = "miot_heartbeat_piid"
CONF_MIOT_SIID = "miot_siid"
CONF_MIOT_PIID = "miot_piid"
CONF_MIOT_POLL = "miot_poll"
CONF_MIOT_TRUE = "miot_true"
CONF_MIOT_TRUE_DEFAULT = "true"
CONF_MIOT_FALSE = "miot_false"
CONF_MIOT_FALSE_DEFAULT = "false"

def validate_heartbeat(config):
    if (CONF_MIOT_HEARTBEAT_SIID in config) != (CONF_MIOT_HEARTBEAT_PIID in config):
        raise cv.Invalid("either none or both of miot_heartbeat_siid and miot_heartbeat_piid are required")
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Miot),
            cv.Optional(CONF_MIOT_HEARTBEAT_SIID): cv.uint32_t,
            cv.Optional(CONF_MIOT_HEARTBEAT_PIID): cv.uint32_t,
            cv.Optional(cv.GenerateID(CONF_TIME_ID)): cv.use_id(time.RealTimeClock),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA),
    validate_heartbeat,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if (CONF_MIOT_HEARTBEAT_SIID in config) and (CONF_MIOT_HEARTBEAT_PIID in config):
        cg.add(var.set_heartbeat_config(config[CONF_MIOT_HEARTBEAT_SIID], config[CONF_MIOT_HEARTBEAT_PIID]))
    if (CONF_TIME_ID in config):
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(time_))

    cg.add_define("USE_OTA_STATE_CALLBACK")
