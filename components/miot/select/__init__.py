from esphome.components import select
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_OPTIONS, CONF_OPTIMISTIC
from .. import miot_ns, CONF_MIOT_ID, CONF_MIOT_SIID, CONF_MIOT_PIID, CONF_MIOT_POLL, Miot

DEPENDENCIES = ["miot"]

MiotSelect = miot_ns.class_("MiotSelect", select.Select, cg.Component)

def ensure_option_map(value):
    cv.check_not_templatable(value)
    option = cv.All(cv.int_range(0, 2**32 - 1))
    mapping = cv.All(cv.string_strict)
    options_map_schema = cv.Schema({option: mapping})
    value = options_map_schema(value)

    all_values = list(value.keys())
    unique_values = set(value.keys())
    if len(all_values) != len(unique_values):
        raise cv.Invalid("Mapping values must be unique.")

    return value

CONFIG_SCHEMA = (
    select.select_schema(MiotSelect)
    .extend(
        {
            cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
            cv.Required(CONF_MIOT_SIID): cv.uint32_t,
            cv.Required(CONF_MIOT_PIID): cv.uint32_t,
            cv.Required(CONF_OPTIONS): ensure_option_map,
            cv.Optional(CONF_MIOT_POLL, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    options_map = config[CONF_OPTIONS]
    var = await select.new_select(config, options=list(options_map.values()))
    await cg.register_component(var, config)
    cg.add(var.set_select_mappings(list(options_map.keys())))
    parent = await cg.get_variable(config[CONF_MIOT_ID])
    cg.add(var.set_miot_config(parent, config[CONF_MIOT_SIID], config[CONF_MIOT_PIID], config[CONF_MIOT_POLL]))
