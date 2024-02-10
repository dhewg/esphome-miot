import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
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
CONF_MIOT_AIID = "miot_aiid"
CONF_MIOT_POLL = "miot_poll"
CONF_MIOT_TRUE = "miot_true"
CONF_MIOT_TRUE_DEFAULT = "true"
CONF_MIOT_FALSE = "miot_false"
CONF_MIOT_FALSE_DEFAULT = "false"
CONF_MIOT_ACTION_ARGS = "miot_action_args"

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

    cg.add_define("USE_OTA_STATE_CALLBACK")

MiotAction = miot_ns.class_("MiotAction", automation.Action)

MIOT_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
        cv.Required(CONF_MIOT_SIID): cv.uint32_t,
        cv.Required(CONF_MIOT_AIID): cv.uint32_t,
        cv.Optional(CONF_MIOT_ACTION_ARGS, default=""): cv.string,
    }
)

@automation.register_action("miot.action", MiotAction, MIOT_ACTION_SCHEMA)
async def action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_MIOT_ID])
    return cg.new_Pvariable(action_id, template_arg, parent, config[CONF_MIOT_SIID], config[CONF_MIOT_AIID], config[CONF_MIOT_ACTION_ARGS])
