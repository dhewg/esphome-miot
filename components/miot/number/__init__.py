from esphome.components import number
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_STEP
from .. import miot_ns, CONF_MIOT_ID, CONF_MIOT_SIID, CONF_MIOT_PIID, CONF_MIOT_POLL, Miot

DEPENDENCIES = ["miot"]

MiotNumber = miot_ns.class_("MiotNumber", number.Number, cg.Component)

def validate_min_max(config):
    if config[CONF_MAX_VALUE] <= config[CONF_MIN_VALUE]:
        raise cv.Invalid("max_value must be greater than min_value")
    return config

CONFIG_SCHEMA = cv.All(
    number.number_schema(MiotNumber)
    .extend(
        {
            cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
            cv.Required(CONF_MIOT_SIID): cv.uint32_t,
            cv.Required(CONF_MIOT_PIID): cv.uint32_t,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_STEP): cv.positive_float,
            cv.Optional(CONF_MIOT_POLL, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_min_max,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )

    parent = await cg.get_variable(config[CONF_MIOT_ID])
    cg.add(var.set_miot_config(parent, config[CONF_MIOT_SIID], config[CONF_MIOT_PIID], config[CONF_MIOT_POLL]))
