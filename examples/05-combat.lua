-- Projectiles, teams and health. Projectiles join the team of whoever fired
-- them and damage anything with health on another team; solids absorb them.
local hud = require("examples.lib.hud")

local function target(x, y)
    return {
        group = "targets",
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 1.5, y = 1.5 } },
            sprite = { texture_id = "soldiers", width = 32, height = 32 },
            boxcollider = { width = 32, height = 32 },
            health = { max_health = 50 },
            team = { name = "enemies" },
        },
    }
end

local TARGET_SPOTS = { { 620, 150 }, { 680, 260 }, { 620, 370 } }

local entities = {
    {
        tag = "player",
        components = {
            transform = { position = { x = 100, y = 280 }, scale = { x = 1.5, y = 1.5 } },
            rigidbody = {},
            sprite = { texture_id = "chopper", width = 32, height = 32, src_rect_y = 32, z_index = 2 },
            animation = { num_frames = 2, speed_rate = 12 },
            boxcollider = { width = 32, height = 32 },
            keyboard_controller = { speed = 200, sprite_rows = { down = 0, right = 1, left = 2, up = 3 } },
            stay_in_bounds = {},
            health = { max_health = 100 },
            team = { name = "player" },
            projectile_emitter = {
                projectile_velocity = { x = 450, y = 0 },
                trigger_key = "Space",
                aim = "facing",
                projectile = { texture_id = "bullet-friendly", damage = 10, lifetime = 1500 },
            },
        },
    },
    {
        components = {
            transform = { position = { x = 700, y = 470 }, scale = { x = 1.5, y = 1.5 } },
            sprite = { texture_id = "turret", width = 32, height = 32 },
            boxcollider = { width = 32, height = 24, offset = { x = 0, y = 4 } },
            health = { max_health = 200 },
            team = { name = "enemies" },
            projectile_emitter = {
                projectile_velocity = { x = -220, y = 0 },
                repeat_frequency = 1200,
                projectile = { texture_id = "bullet-enemy", damage = 10, lifetime = 3500 },
            },
        },
    },
    {
        components = {
            transform = { position = { x = 250, y = 120 }, scale = { x = 2, y = 2 } },
            sprite = { texture_id = "tree", width = 16, height = 32 },
            boxcollider = { width = 16, height = 32 },
            damage_on_contact = { damage = 1 },
            team = { name = "hazards" },
        },
    },
}
for _, spot in ipairs(TARGET_SPOTS) do
    entities[#entities + 1] = target(spot[1], spot[2])
end
for y = 300, 540, 48 do
    entities[#entities + 1] = {
        components = {
            transform = { position = { x = 420, y = y }, scale = { x = 1.5, y = 1.5 } },
            sprite = { texture_id = "rock", width = 32, height = 32 },
            boxcollider = { width = 32, height = 32 },
            solid = {},
        },
    }
end
for _, label in ipairs(hud.entities(
    hud.title("PROJECTILES, TEAMS AND HEALTH"),
    hud.lines(18, 50, {
        "Arrows fly, Space fires along your facing. Your shots hurt the \"enemies\" team only.",
        "The tank fires on a timer. Rocks are solid and absorb shots from both sides.",
        "The tree has damage_on_contact: touching it hurts every frame. Targets respawn.",
    }),
    hud.status(18, 552, "player-health"),
    hud.footer("R restart      ESC back")
)) do
    entities[#entities + 1] = label
end

Scene = {
    background_color = { r = 36, g = 40, b = 52 },
    assets = hud.with_fonts({
        { type = "texture", id = "chopper", file = "assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "soldiers", file = "assets/images/army-group-2.png" },
        { type = "texture", id = "turret", file = "assets/images/tank-big-left.png" },
        { type = "texture", id = "tree", file = "assets/images/tree-small-7.png" },
        { type = "texture", id = "rock", file = "assets/images/rock-big-3.png" },
        { type = "texture", id = "bullet-friendly", file = "assets/images/bullet-friendly.png" },
        { type = "texture", id = "bullet-enemy", file = "assets/images/bullet-enemy.png" },
    }),
    entities = entities,

    on_update = function()
        local player = get_entity_by_tag("player")
        local status = get_entity_by_tag("player-health")
        if player then
            local health, max_health = get_health(player)
            set_text(status, string.format("your health: %d / %d", health, max_health))
        else
            set_text(status, "you were destroyed - press R")
        end
        -- spawn() builds an entity from the same kind of definition as the scene.
        if #get_entities_in_group("targets") == 0 then
            for _, spot in ipairs(TARGET_SPOTS) do
                spawn(target(spot[1], spot[2]))
            end
        end
    end,

    on_key = function(key)
        if key == "R" then
            reload_scene()
            return true
        end
        return false
    end,
}
