from esphome.components import fan
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_DIRECTION,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_OPTIONS,
    CONF_OSCILLATING,
    CONF_PRESET_MODES,
    CONF_SPEED,
    CONF_STATE,
    CONF_STEP,
)
from .. import (
    miot_ns,
    CONF_MIOT_ID,
    CONF_MIOT_SIID,
    CONF_MIOT_PIID,
    Miot,
)

DEPENDENCIES = ["miot"]

MiotFan = miot_ns.class_("MiotFan", cg.Component, fan.Fan)

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

CONFIG_SCHEMA = cv.All(
    fan.fan_schema(MiotFan)
    .extend(
        {
            cv.GenerateID(CONF_MIOT_ID): cv.use_id(Miot),
            cv.Required(CONF_STATE): cv.Schema(
                {
                    cv.Required(CONF_MIOT_SIID): cv.uint32_t,
                    cv.Required(CONF_MIOT_PIID): cv.uint32_t,
                }
            ),
            cv.Required(CONF_SPEED): cv.Schema(
                {
                    cv.Required(CONF_MIOT_SIID): cv.uint32_t,
                    cv.Required(CONF_MIOT_PIID): cv.uint32_t,
                    cv.Required(CONF_MIN_VALUE): cv.uint32_t,
                    cv.Required(CONF_MAX_VALUE): cv.uint32_t,
                    cv.Optional(CONF_STEP, default=1): cv.uint32_t,
                }
            ),
            cv.Optional(CONF_OSCILLATING): cv.Schema(
                {
                    cv.Required(CONF_MIOT_SIID): cv.uint32_t,
                    cv.Required(CONF_MIOT_PIID): cv.uint32_t,
                }
            ),
            cv.Optional(CONF_DIRECTION): cv.Schema(
                {
                    cv.Required(CONF_MIOT_SIID): cv.uint32_t,
                    cv.Required(CONF_MIOT_PIID): cv.uint32_t,
                }
            ),
            cv.Optional(CONF_PRESET_MODES): cv.Schema(
                {
                    cv.Required(CONF_MIOT_SIID): cv.uint32_t,
                    cv.Required(CONF_MIOT_PIID): cv.uint32_t,
                    cv.Required(CONF_OPTIONS): ensure_option_map,
                }
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_MIOT_ID])

    var = await fan.new_fan(config, parent)
    await cg.register_component(var, config)

    state = config.get(CONF_STATE)
    cg.add(var.set_state_config(state[CONF_MIOT_SIID], state[CONF_MIOT_PIID]))
    speed = config.get(CONF_SPEED)
    cg.add(var.set_speed_config(speed[CONF_MIOT_SIID], speed[CONF_MIOT_PIID], speed[CONF_MIN_VALUE], speed[CONF_MAX_VALUE], speed[CONF_STEP]))
    if oscillating := config.get(CONF_OSCILLATING):
        cg.add(var.set_oscillating_config(oscillating[CONF_MIOT_SIID], oscillating[CONF_MIOT_PIID]))
    if direction := config.get(CONF_DIRECTION):
        cg.add(var.set_direction_config(direction[CONF_MIOT_SIID], direction[CONF_MIOT_PIID]))
    if preset_modes := config.get(CONF_PRESET_MODES):
        cg.add(var.set_preset_modes_config(preset_modes[CONF_MIOT_SIID], preset_modes[CONF_MIOT_PIID]))
        options_map = preset_modes[CONF_OPTIONS]
        for k in options_map:
            cg.add(var.set_preset_mode_name(k, options_map[k]))
