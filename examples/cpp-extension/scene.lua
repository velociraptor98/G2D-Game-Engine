-- Uses the `spin` component, which only exists because main.cpp registered it.
-- The plain g2d runner doesn't know `spin` and would warn about it.
local hud = require("examples.lib.hud")

local function spinner(texture, x, y, size, degrees_per_second)
    return {
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 3, y = 3 } },
            sprite = { texture_id = texture, width = size, height = size },
            spin = { degrees_per_second = degrees_per_second },
        },
    }
end

Scene = {
    background_color = { r = 24, g = 30, b = 44 },
    assets = hud.with_fonts({
        { type = "texture", id = "tank", file = "assets/images/tank-big-right.png" },
        { type = "texture", id = "rock", file = "assets/images/rock-big-1.png" },
    }),
    entities = hud.entities(
        spinner("tank", 120, 260, 32, 45),
        spinner("tank", 340, 260, 32, -120),
        spinner("rock", 560, 260, 32, 360),
        hud.title("C++ EXTENSION"),
        hud.lines(18, 52, {
            "examples/cpp-extension/main.cpp adds a SpinComponent and a SpinSystem,",
            "registers \"spin\" so this scene file can use it, and runs the system from an update hook.",
            "The orange frame is drawn by a render hook.",
        }),
        hud.footer("ESC quit")
    ),
}
