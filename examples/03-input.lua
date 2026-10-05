-- keyboard_controller turns held keys into velocity. Key bindings and the
-- spritesheet row for each direction are set per entity.
local hud = require("examples.lib.hud")

Scene = {
    background_color = { r = 28, g = 52, b = 32 },
    assets = hud.with_fonts({
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "tank", file = "assets/images/tank-big-right.png" },
    }),
    entities = hud.entities(
        {
            tag = "chopper",
            components = {
                transform = { position = { x = 250, y = 300 }, scale = { x = 2, y = 2 } },
                rigidbody = {},
                sprite = { texture_id = "chopper", width = 32, height = 32, src_rect_y = 32 },
                animation = { num_frames = 2, speed_rate = 12 },
                keyboard_controller = {
                    speed = 220,
                    sprite_rows = { down = 0, right = 1, left = 2, up = 3 },
                },
                stay_in_bounds = {},
            },
        },
        {
            tag = "tank",
            components = {
                transform = { position = { x = 500, y = 320 }, scale = { x = 2, y = 2 } },
                rigidbody = {},
                sprite = { texture_id = "tank", width = 32, height = 32 },
                keyboard_controller = {
                    speed = 110,
                    keys = { up = { "I" }, down = { "K" }, left = { "J" }, right = { "L" } },
                },
                stay_in_bounds = {},
                -- The tank image only faces right, so it flips instead of switching rows.
                script = function(entity)
                    local vx = get_velocity(entity)
                    if vx < 0 then set_flip(entity, true) elseif vx > 0 then set_flip(entity, false) end
                end,
            },
        },
        hud.title("KEYBOARD INPUT"),
        hud.lines(18, 52, {
            "Arrows or WASD fly the chopper; each direction shows its own spritesheet row.",
            "I J K L drive the tank: key bindings belong to the entity, set in the scene file.",
            "Both have stay_in_bounds, so neither can leave the window.",
        }),
        hud.status(18, 530, "chopper-speed"),
        hud.status(18, 552, "tank-speed"),
        hud.footer()
    ),

    on_update = function()
        for _, name in ipairs({ "chopper", "tank" }) do
            local vx, vy = get_velocity(get_entity_by_tag(name))
            set_text(get_entity_by_tag(name .. "-speed"), string.format("%s velocity: %.0f, %.0f", name, vx, vy))
        end
    end,
}
