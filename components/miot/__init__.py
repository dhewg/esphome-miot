import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

miot_ns = cg.esphome_ns.namespace("miot")
Miot = miot_ns.class_("Miot", cg.Component, uart.UARTDevice)

CONF_MIOT_ID = "miot_id"
CONF_MIOT_HEARTBEAT_SIID = "miot_heartbeat_siid"
CONF_MIOT_HEARTBEAT_PIID = "miot_heartbeat_piid"
CONF_MIOT_SIID = "miot_siid"
CONF_MIOT_PIID = "miot_piid"
CONF_MIOT_POLL = "miot_poll"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Miot),
            cv.Optional(CONF_MIOT_HEARTBEAT_SIID): cv.uint32_t,
            cv.Optional(CONF_MIOT_HEARTBEAT_PIID): cv.uint32_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_heartbeat_config(config[CONF_MIOT_HEARTBEAT_SIID], config[CONF_MIOT_HEARTBEAT_PIID]))

    cg.add_define("USE_OTA_STATE_CALLBACK")
