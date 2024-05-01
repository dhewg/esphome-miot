import esphome.config_validation as cv
from esphome.components import event
import esphome.codegen as cg
from esphome.const import CONF_EVENT_TYPE
from .. import miot_ns, CONF_MIOT_ID, CONF_MIOT_SIID, CONF_MIOT_EIID, Miot

DEPENDENCIES = ["miot"]

MiotEvent = miot_ns.class_("MiotEvent", event.Event, cg.Component)

CONFIG_SCHEMA = event.event_schema(MiotEvent).extend(
    {
        cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
        cv.Required(CONF_MIOT_SIID): cv.uint32_t,
        cv.Required(CONF_MIOT_EIID): cv.uint32_t,
        cv.Required(CONF_EVENT_TYPE): cv.string_strict,
    }
)

async def to_code(config):
    var = await event.new_event(config, event_types=[config[CONF_EVENT_TYPE]])
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_MIOT_ID])
    cg.add(var.set_miot_config(parent, config[CONF_MIOT_SIID], config[CONF_MIOT_EIID], cg.RawExpression(f'"{config[CONF_EVENT_TYPE]}"')))