-- A tilemap builds the world from a .map file and a tileset. The camera
-- follows the entity with camera_follow and stops at the world's edges.
local hud = require("examples.lib.hud")

Scene = {
    assets = hud.with_fonts({
        { type = "texture", id = "tiles", file = "assets/tilemaps/jungle.png" },
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "heliport", file = "assets/images/heliport.png" },
    }),
    tilemap = { map_file = "assets/tilemaps/jungle.map", texture_id = "tiles", tile_size = 32, scale = 2 },
    entities = hud.entities(
        {
            tag = "drone",
            components = {
                transform = { position = { x = 800, y = 640 }, scale = { x = 2, y = 2 } },
                sprite = { texture_id = "chopper", width = 32, height = 32, z_index = 2 },
                animation = { num_frames = 2, speed_rate = 12 },
                camera_follow = {},
                -- Flies a figure of eight that reaches every edge of the 1600 x 1280 world.
                script = function(entity, delta_time, elapsed_ms)
                    local t = elapsed_ms / 1000
                    set_position(entity, 760 + 700 * math.sin(t * 0.35), 600 + 560 * math.sin(t * 0.7))
                    local dx, dy = 0.35 * math.cos(t * 0.35), 1.4 * math.cos(t * 0.7)
                    if math.abs(dx) * 2 > math.abs(dy) then
                        set_sprite_row(entity, dx > 0 and 1 or 2)
                    else
                        set_sprite_row(entity, dy > 0 and 0 or 3)
                    end
                end,
            },
        },
        {
            components = {
                transform = { position = { x = 470, y = 410 }, scale = { x = 2, y = 2 } },
                sprite = { texture_id = "heliport", width = 32, height = 32, z_index = 1 },
            },
        },
        hud.label(452, 480, "heliport (a label in the world)", { fixed = false, font = "hud-small" }),
        hud.title("TILEMAP AND CAMERA"),
        hud.lines(18, 52, {
            "The world is 1600 x 1280: a 25 x 20 tile map at scale 2.",
            "The camera keeps the drone centred, but never shows past the map's edge.",
        }),
        hud.status(18, 552, "position"),
        hud.footer()
    ),

    on_update = function(delta_time, elapsed_ms)
        local x, y = get_position(get_entity_by_tag("drone"))
        set_text(get_entity_by_tag("position"), string.format("drone at %.0f, %.0f", x, y))
    end,
}
