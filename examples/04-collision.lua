-- Colliders report overlaps; what happens next is up to each entity. Rocks
-- are solid. The tank blocks against them, the trucks bounce, and the
-- soldiers have no collision_response, so they walk straight through.
local hud = require("examples.lib.hud")

local entities = {}

local function rock(x, y)
    entities[#entities + 1] = {
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 1.5, y = 1.5 } },
            sprite = { texture_id = "rock", width = 32, height = 32 },
            boxcollider = { width = 32, height = 32 },
            solid = {},
        },
    }
end

-- A walled pen from (64, 120) to (736, 552), 48 px per rock.
for x = 64, 688, 48 do
    rock(x, 120)
    rock(x, 504)
end
for y = 168, 456, 48 do
    rock(64, y)
    rock(688, y)
end
rock(352, 264)
rock(400, 264)
rock(352, 312)

local function truck(x, y, vx, vy)
    entities[#entities + 1] = {
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 1.5, y = 1.5 } },
            rigidbody = { velocity = { x = vx, y = vy } },
            sprite = { texture_id = "truck", width = 32, height = 32, flip = vx < 0 and "horizontal" or "none" },
            boxcollider = { width = 32, height = 24, offset = { x = 0, y = 4 } },
            collision_response = { on_solid = "bounce", flip_sprite_on_bounce = true },
        },
    }
end
truck(160, 200, -90, 60)
truck(520, 400, 70, -110)

entities[#entities + 1] = {
    components = {
        transform = { position = { x = 200, y = 400 }, scale = { x = 1.5, y = 1.5 } },
        rigidbody = {},
        sprite = { texture_id = "tank", width = 32, height = 32, z_index = 1 },
        boxcollider = { width = 32, height = 24, offset = { x = 0, y = 4 } },
        keyboard_controller = { speed = 160 },
        collision_response = { on_solid = "block" },
        script = function(entity)
            local vx = get_velocity(entity)
            if vx < 0 then set_flip(entity, true) elseif vx > 0 then set_flip(entity, false) end
        end,
    },
}

entities[#entities + 1] = {
    components = {
        transform = { position = { x = 20, y = 330 }, scale = { x = 1.5, y = 1.5 } },
        rigidbody = {},
        sprite = { texture_id = "soldiers", width = 32, height = 32, z_index = 2 },
        boxcollider = { width = 32, height = 32 },
        -- Walks across the pen and back, through the walls.
        script = function(entity, delta_time, elapsed_ms)
            local leg = math.floor(elapsed_ms / 9000) % 2
            set_velocity(entity, leg == 0 and 80 or -80, 0)
        end,
    },
}

for _, label in ipairs(hud.entities(
    hud.title("COLLISION"),
    hud.lines(18, 50, {
        "Yellow boxes are colliders (C toggles them). The rocks are solid.",
        "Arrows drive the tank: on_solid = \"block\", so it stops at walls and slides along them.",
        "Trucks use \"bounce\". The soldiers have no collision_response and walk through.",
    }),
    hud.footer("C colliders      ESC back")
)) do
    entities[#entities + 1] = label
end

Scene = {
    background_color = { r = 40, g = 44, b = 36 },
    assets = hud.with_fonts({
        { type = "texture", id = "rock", file = "assets/images/rock-big-1.png" },
        { type = "texture", id = "truck", file = "assets/images/truck-right.png" },
        { type = "texture", id = "tank", file = "assets/images/tank-big-right.png" },
        { type = "texture", id = "soldiers", file = "assets/images/army-group-1.png" },
    }),
    entities = entities,
    on_start = function() set_debug_overlay(true) end,
    on_key = function(key)
        if key == "C" then
            toggle_debug_overlay()
            return true
        end
        return false
    end,
}
