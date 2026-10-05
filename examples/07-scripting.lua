-- Lua scripts give entities behaviour without any C++: each script runs every
-- frame as script(entity, delta_time, elapsed_ms).
local hud = require("examples.lib.hud")

local CENTRE_X, CENTRE_Y = 400, 330

-- Each orbiter circles the centre; its radius and speed come from the closure.
local function orbiter(radius, seconds_per_lap, phase)
    return {
        components = {
            transform = { scale = { x = 1.5, y = 1.5 } },
            sprite = { texture_id = "bandit", width = 32, height = 32, src_rect_y = 32 },
            animation = { num_frames = 2, speed_rate = 12 },
            script = function(entity, delta_time, elapsed_ms)
                local angle = phase + (elapsed_ms / 1000) * (2 * math.pi / seconds_per_lap)
                set_position(entity, CENTRE_X - 24 + radius * math.cos(angle), CENTRE_Y - 24 + radius * math.sin(angle))
                set_sprite_row(entity, math.sin(angle) > 0 and 2 or 1)
            end,
        },
    }
end

local function firework_spark(angle)
    local speed = 260
    return {
        components = {
            transform = { position = { x = CENTRE_X, y = CENTRE_Y }, scale = { x = 3, y = 3 } },
            rigidbody = { velocity = { x = speed * math.cos(angle), y = speed * math.sin(angle) } },
            sprite = { texture_id = "spark", width = 4, height = 4, z_index = 5 },
            lifetime = { duration = 1200 },
        },
    }
end

Scene = {
    background_color = { r = 16, g = 20, b = 30 },
    assets = hud.with_fonts({
        { type = "texture", id = "bandit", file = "assets/images/bandit-spritesheet.png" },
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "truck", file = "assets/images/truck-right.png" },
        { type = "texture", id = "turret", file = "assets/images/tank-big-down.png" },
        { type = "texture", id = "spark", file = "assets/images/bullet-friendly.png" },
        { type = "texture", id = "shell", file = "assets/images/bullet-enemy.png" },
    }),
    entities = hud.entities(
        orbiter(110, 4, 0),
        orbiter(170, 7, 2),
        orbiter(230, 11, 4),
        {
            tag = "player",
            components = {
                transform = { position = { x = 120, y = 440 }, scale = { x = 1.5, y = 1.5 } },
                rigidbody = {},
                sprite = { texture_id = "chopper", width = 32, height = 32, src_rect_y = 32, z_index = 2 },
                animation = { num_frames = 2, speed_rate = 12 },
                keyboard_controller = { speed = 200, sprite_rows = { down = 0, right = 1, left = 2, up = 3 } },
                stay_in_bounds = {},
            },
        },
        {
            components = {
                transform = { position = { x = 700, y = 110 }, scale = { x = 1.5, y = 1.5 } },
                sprite = { texture_id = "turret", width = 32, height = 32 },
                projectile_emitter = {
                    repeat_frequency = 700,
                    projectile = { texture_id = "shell", lifetime = 2500 },
                },
                -- Re-aims the emitter at the player every frame.
                script = function(entity)
                    local player = get_entity_by_tag("player")
                    if not player then return end
                    local px, py = get_position(player)
                    local x, y = get_position(entity)
                    local dx, dy = px - x, py - y
                    local length = math.max(1, math.sqrt(dx * dx + dy * dy))
                    set_projectile_velocity(entity, dx / length * 240, dy / length * 240)
                end,
            },
        },
        {
            components = {
                transform = { position = { x = 120, y = 520 }, scale = { x = 1.5, y = 1.5 } },
                rigidbody = {},
                sprite = { texture_id = "truck", width = 32, height = 32 },
                -- Drives right for 3 seconds, then left for 3 seconds.
                script = function(entity, delta_time, elapsed_ms)
                    local right = math.floor(elapsed_ms / 3000) % 2 == 0
                    set_velocity(entity, right and 90 or -90, 0)
                    set_flip(entity, not right)
                end,
            },
        },
        hud.title("LUA SCRIPTING"),
        hud.lines(18, 50, {
            "The bandits orbit, the turret aims at you, the truck patrols: all small Lua functions.",
            "Arrows fly the chopper. F launches fireworks with spawn() and lifetime.",
        }),
        hud.footer("F fireworks      ESC back")
    ),

    on_key = function(key)
        if key == "F" then
            for i = 0, 15 do
                spawn(firework_spark(i * math.pi / 8))
            end
            return true
        end
        return false
    end,
}
