from esphome.components import button
import esphome.config_validation as cv
import esphome.codegen as cg
from .. import miot_ns, CONF_MIOT_ID, CONF_MIOT_SIID, Miot

DEPENDENCIES = ["miot"]

MiotButton = miot_ns.class_("MiotButton", button.Button, cg.Component)

CONF_MIOT_AIID = "miot_aiid"
CONF_MIOT_ACTION_ARGS = "miot_action_args"

CONFIG_SCHEMA = (
    button.button_schema(MiotButton)
    .extend(
        {
            cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
            cv.Required(CONF_MIOT_SIID): cv.uint32_t,
            cv.Required(CONF_MIOT_AIID): cv.uint32_t,
            cv.Optional(CONF_MIOT_ACTION_ARGS): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_MIOT_ID])
    cg.add(var.set_miot_config(parent, config[CONF_MIOT_SIID], config[CONF_MIOT_AIID]))
    if CONF_MIOT_ACTION_ARGS in config:
        cg.add(var.set_action_args(config[CONF_MIOT_ACTION_ARGS]))
