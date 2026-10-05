-- The examples menu. Each demo focuses on one part of the engine.
local hud = require("examples.lib.hud")

local demos = {
    { key = "1", name = "Sprites and animation", file = "examples/01-sprites-animation.lua" },
    { key = "2", name = "Tilemap and camera", file = "examples/02-tilemap-camera.lua" },
    { key = "3", name = "Keyboard input", file = "examples/03-input.lua" },
    { key = "4", name = "Collision", file = "examples/04-collision.lua" },
    { key = "5", name = "Projectiles, teams and health", file = "examples/05-combat.lua" },
    { key = "6", name = "Text and audio", file = "examples/06-text-audio.lua" },
    { key = "7", name = "Lua scripting", file = "examples/07-scripting.lua" },
    { key = "8", name = "Jungle - a complete example game", file = "games/jungle/level1.lua" },
}

local menu = {}
for _, demo in ipairs(demos) do
    menu[#menu + 1] = demo.key .. "    " .. demo.name
end

Scene = {
    background_color = { r = 18, g = 24, b = 32 },
    assets = hud.with_fonts({
        { type = "texture", id = "radar", file = "assets/images/radar.png" },
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
    }),
    entities = hud.entities(
        hud.title("G2D ENGINE SHOWCASE"),
        hud.lines(18, 64, {
            "Every demo is a Lua scene file run by the same engine binary.",
            "Press a number to open one. ESC in a demo comes back here.",
        }, { color = { r = 170, g = 180, b = 190 } }),
        hud.lines(60, 150, menu, { font = "hud-menu", spacing = 38 }),
        {
            components = {
                transform = { position = { x = 690, y = 20 }, scale = { x = 1.5, y = 1.5 } },
                sprite = { texture_id = "radar", width = 64, height = 64, fixed = true },
                animation = { num_frames = 8, speed_rate = 6 },
            },
        },
        {
            components = {
                transform = { position = { x = 600, y = 420 }, scale = { x = 3, y = 3 } },
                sprite = { texture_id = "chopper", width = 32, height = 32, src_rect_y = 64 },
                animation = { num_frames = 2, speed_rate = 12 },
                script = function(entity, delta_time, elapsed_ms)
                    set_position(entity, 600, 420 + 12 * math.sin(elapsed_ms / 400))
                end,
            },
        },
        hud.footer("1-8 open a demo      ESC quit")
    ),

    on_key = function(key)
        for _, demo in ipairs(demos) do
            if key == demo.key then
                load_scene(demo.file)
                return true
            end
        end
        return false
    end,
}
