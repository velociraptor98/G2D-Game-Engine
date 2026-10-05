-- Sprites draw a rectangle of a texture. Animation steps that rectangle along
-- a spritesheet row. z_index orders drawing; flip and rotation change how a
-- sprite is drawn.
local hud = require("examples.lib.hud")

local function sprite(texture, x, y, size, fields)
    local components = {
        transform = { position = { x = x, y = y }, scale = { x = 2, y = 2 } },
        sprite = { texture_id = texture, width = size, height = size },
    }
    for name, value in pairs(fields or {}) do
        if name == "sprite" then
            for k, v in pairs(value) do components.sprite[k] = v end
        else
            components[name] = value
        end
    end
    return { components = components }
end

local function caption(x, y, text)
    return hud.label(x, y, text, { font = "hud-small", color = { r = 170, g = 170, b = 170 } })
end

local entities = hud.entities(
    hud.title("SPRITES AND ANIMATION"),
    hud.lines(18, 60, {
        "Each chopper shows one row of the same spritesheet, animated at 10 frames per second.",
    }, { color = { r = 170, g = 180, b = 190 } })
)

local rows = { "row 0 (down)", "row 1 (right)", "row 2 (left)", "row 3 (up)" }
for i, row in ipairs(rows) do
    local x = 60 + (i - 1) * 180
    entities[#entities + 1] = sprite("chopper", x, 110, 32, {
        sprite = { src_rect_y = (i - 1) * 32 },
        animation = { num_frames = 2, speed_rate = 10 },
    })
    entities[#entities + 1] = caption(x - 4, 180, row)
end

entities[#entities + 1] = sprite("tank", 60, 260, 32)
entities[#entities + 1] = caption(60, 330, "as drawn")
entities[#entities + 1] = sprite("tank", 170, 260, 32, { sprite = { flip = "horizontal" } })
entities[#entities + 1] = caption(170, 330, "flip")
entities[#entities + 1] = sprite("tank", 280, 260, 32, {
    script = function(entity, delta_time, elapsed_ms)
        set_rotation(entity, elapsed_ms * 0.09)
    end,
})
entities[#entities + 1] = caption(280, 330, "rotation (script)")
entities[#entities + 1] = sprite("radar", 440, 250, 64, {
    transform = { position = { x = 440, y = 250 } },
    animation = { num_frames = 8, speed_rate = 16 },
})
entities[#entities + 1] = caption(440, 330, "8 frames, fast")
entities[#entities + 1] = sprite("radar", 580, 250, 64, {
    transform = { position = { x = 580, y = 250 } },
    animation = { num_frames = 8, speed_rate = 2 },
})
entities[#entities + 1] = caption(580, 330, "8 frames, slow")

entities[#entities + 1] = hud.label(18, 380, "z_index decides what is drawn on top:", { color = { r = 170, g = 180, b = 190 } })
entities[#entities + 1] = sprite("heliport", 60, 420, 32, { sprite = { z_index = 1 } })
entities[#entities + 1] = sprite("chopper", 90, 440, 32, {
    sprite = { z_index = 2, src_rect_y = 32 },
    animation = { num_frames = 2, speed_rate = 10 },
})
entities[#entities + 1] = sprite("tree", 128, 420, 16, {
    transform = { position = { x = 128, y = 420 }, scale = { x = 3, y = 3 } },
    sprite = { z_index = 3, width = 16, height = 32 },
})
entities[#entities + 1] = caption(200, 450, "heliport z 1  <  chopper z 2  <  tree z 3")
entities[#entities + 1] = hud.footer()

Scene = {
    assets = hud.with_fonts({
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "tank", file = "assets/images/tank-big-right.png" },
        { type = "texture", id = "radar", file = "assets/images/radar.png" },
        { type = "texture", id = "heliport", file = "assets/images/heliport.png" },
        { type = "texture", id = "tree", file = "assets/images/tree-small-6.png" },
    }),
    entities = entities,
}
