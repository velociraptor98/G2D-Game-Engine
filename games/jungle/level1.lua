-- Jungle, level 1: fly the chopper, destroy every enemy.
--
-- Game rules live in this file, not the engine: the player and enemies are on
-- different teams, the map's border tiles are solid, enemies bounce off them
-- and projectiles are destroyed by them.

-- Rows of the chopper and bandit spritesheets.
local SPRITE_ROW = { down = 0, right = 1, left = 2, up = 3 }

local function unit(x, y)
    local length = math.sqrt(x * x + y * y)
    if length == 0 then return 0, 0 end
    return x / length, y / length
end

-- Circles (centre_x, centre_y) and keeps its guns trained on the player.
local function circling_bandit(centre_x, centre_y, radius, seconds_per_lap)
    return function(entity, delta_time, elapsed_ms)
        local angle = (elapsed_ms / 1000) * (2 * math.pi / seconds_per_lap)
        set_position(entity, centre_x + radius * math.cos(angle), centre_y + radius * math.sin(angle))

        local heading_x, heading_y = -math.sin(angle), math.cos(angle)
        if math.abs(heading_x) > math.abs(heading_y) then
            set_sprite_row(entity, heading_x > 0 and SPRITE_ROW.right or SPRITE_ROW.left)
        else
            set_sprite_row(entity, heading_y > 0 and SPRITE_ROW.down or SPRITE_ROW.up)
        end

        local player = get_entity_by_tag("player")
        if player then
            local player_x, player_y = get_position(player)
            local own_x, own_y = get_position(entity)
            local aim_x, aim_y = unit(player_x - own_x, player_y - own_y)
            set_projectile_velocity(entity, aim_x * 220, aim_y * 220)
        end
    end
end

-- Drives back and forth, turning around every `seconds` seconds.
local function patrol(speed, seconds)
    return function(entity, delta_time, elapsed_ms)
        local leg = math.floor(elapsed_ms / (seconds * 1000))
        local direction = (leg % 2 == 0) and -1 or 1
        set_velocity(entity, direction * speed, 0)
        set_flip(entity, direction > 0)
    end
end

local function enemy_fire(velocity, every_ms, damage)
    return {
        projectile_velocity = velocity,
        repeat_frequency = every_ms,
        projectile = { texture_id = "bullet-enemy-image", damage = damage or 10, lifetime = 3000 },
    }
end

local function enemy(texture, x, y, velocity, fire)
    return {
        group = "enemies",
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 2, y = 2 } },
            rigidbody = { velocity = velocity },
            sprite = { texture_id = texture, width = 32, height = 32, z_index = 2 },
            boxcollider = { width = 32, height = 24, offset = { x = 0, y = 4 } },
            health = { max_health = 100 },
            team = { name = "enemies" },
            collision_response = { on_solid = "bounce", flip_sprite_on_bounce = true },
            projectile_emitter = enemy_fire(fire.velocity, fire.every_ms),
        },
    }
end

local function scenery(texture, x, y, width, height)
    return {
        components = {
            transform = { position = { x = x, y = y }, scale = { x = 2, y = 2 } },
            sprite = { texture_id = texture, width = width, height = height, z_index = 1 },
        },
    }
end

Scene = {
    assets = {
        { type = "texture", id = "jungle-tiles", file = "./assets/tilemaps/jungle.png" },
        { type = "texture", id = "chopper-image", file = "./assets/images/chopper-spritesheet.png" },
        { type = "texture", id = "bandit-image", file = "./assets/images/bandit-spritesheet.png" },
        { type = "texture", id = "tank-right-image", file = "./assets/images/tank-big-right.png" },
        { type = "texture", id = "tank-down-image", file = "./assets/images/tank-big-down.png" },
        { type = "texture", id = "tank-small-left-image", file = "./assets/images/tank-small-left.png" },
        { type = "texture", id = "truck-left-image", file = "./assets/images/truck-left.png" },
        { type = "texture", id = "army-group-image", file = "./assets/images/army-group-1.png" },
        { type = "texture", id = "heliport-image", file = "./assets/images/heliport.png" },
        { type = "texture", id = "radar-image", file = "./assets/images/radar.png" },
        { type = "texture", id = "tree-image", file = "./assets/images/tree-small-6.png" },
        { type = "texture", id = "bush-image", file = "./assets/images/tree-small-1.png" },
        { type = "texture", id = "rock-image", file = "./assets/images/rock-big-1.png" },
        { type = "texture", id = "bullet-friendly-image", file = "./assets/images/bullet-friendly.png" },
        { type = "texture", id = "bullet-enemy-image", file = "./assets/images/bullet-enemy.png" },
        { type = "font", id = "charriot-font", file = "./assets/fonts/charriot.ttf", font_size = 20 },
        { type = "font", id = "charriot-font-large", file = "./assets/fonts/charriot.ttf", font_size = 48 },
        { type = "font", id = "hint-font", file = "./assets/fonts/arial.ttf", font_size = 14 },
        { type = "sound", id = "helicopter-sound", file = "./assets/sounds/helicopter.wav" },
    },

    tilemap = {
        map_file = "./assets/tilemaps/jungle.map",
        texture_id = "jungle-tiles",
        tile_size = 32,
        scale = 2.0,
    },

    entities = {
        {
            tag = "player",
            components = {
                transform = { position = { x = 470, y = 410 }, scale = { x = 2, y = 2 } },
                rigidbody = { velocity = { x = 0, y = 0 } },
                sprite = { texture_id = "chopper-image", width = 32, height = 32, z_index = 3, src_rect_y = 32 },
                animation = { num_frames = 2, speed_rate = 15 },
                boxcollider = { width = 32, height = 32 },
                health = { max_health = 100 },
                team = { name = "player" },
                stay_in_bounds = {},
                projectile_emitter = {
                    projectile_velocity = { x = 500, y = 0 },
                    trigger_key = "Space",
                    aim = "facing",
                    projectile = { texture_id = "bullet-friendly-image", damage = 25, lifetime = 1500 },
                },
                keyboard_controller = { speed = 250, sprite_rows = SPRITE_ROW },
                camera_follow = {},
                audio = { sound_id = "helicopter-sound", loop = true, volume = 42 },
            },
        },

        enemy("tank-right-image", 700, 300, { x = 30, y = 0 }, { velocity = { x = 0, y = -150 }, every_ms = 2000 }),
        enemy("tank-down-image", 1300, 230, { x = 0, y = 0 }, { velocity = { x = 0, y = 200 }, every_ms = 1500 }),
        enemy("tank-small-left-image", 1150, 950, { x = -40, y = 0 }, { velocity = { x = -200, y = 0 }, every_ms = 1800 }),
        enemy("army-group-image", 260, 900, { x = 0, y = 0 }, { velocity = { x = 140, y = -140 }, every_ms = 2200 }),
        (function()
            local truck = enemy("truck-left-image", 900, 700, { x = 0, y = 0 }, { velocity = { x = 0, y = 150 }, every_ms = 2500 })
            truck.components.script = patrol(40, 5)
            return truck
        end)(),
        {
            group = "enemies",
            components = {
                transform = { position = { x = 1000, y = 650 }, scale = { x = 2, y = 2 } },
                sprite = { texture_id = "bandit-image", width = 32, height = 32, z_index = 3 },
                animation = { num_frames = 2, speed_rate = 15 },
                boxcollider = { width = 32, height = 32 },
                health = { max_health = 100 },
                team = { name = "enemies" },
                projectile_emitter = enemy_fire({ x = 0, y = 0 }, 1200, 5),
                script = circling_bandit(1000, 650, 180, 8),
            },
        },

        scenery("heliport-image", 470, 410, 32, 32),
        scenery("tree-image", 620, 520, 16, 32),
        scenery("tree-image", 660, 540, 16, 32),
        scenery("bush-image", 420, 560, 16, 16),
        scenery("rock-image", 1220, 760, 32, 32),
        scenery("tree-image", 180, 1000, 16, 32),
        scenery("bush-image", 1400, 1050, 16, 16),

        {
            components = {
                transform = { position = { x = 726, y = 10 } },
                sprite = { texture_id = "radar-image", width = 64, height = 64, z_index = 10, fixed = true },
                animation = { num_frames = 8, speed_rate = 5 },
            },
        },
        {
            components = {
                text_label = { position = { x = 12, y = 10 }, text = "JUNGLE - LEVEL 1", font_id = "charriot-font" },
            },
        },
        {
            components = {
                text_label = {
                    position = { x = 12, y = 576 },
                    text = "ARROWS/WASD fly   SPACE fire   C colliders   R restart   ESC back",
                    font_id = "hint-font",
                    color = { r = 220, g = 220, b = 220 },
                },
            },
        },
        {
            tag = "status-label",
            components = {
                text_label = {
                    position = { x = 400, y = 300 },
                    text = "",
                    font_id = "charriot-font-large",
                    color = { r = 255, g = 230, b = 120 },
                    centred = true,
                },
            },
        },
    },

    on_update = function(delta_time, elapsed_ms)
        local status = get_entity_by_tag("status-label")
        if not get_entity_by_tag("player") then
            set_text(status, "GAME OVER")
        elseif #get_entities_in_group("enemies") == 0 then
            set_text(status, "MISSION COMPLETE")
        end
    end,

    on_key = function(key)
        if key == "R" then
            reload_scene()
            return true
        elseif key == "C" then
            toggle_debug_overlay()
            return true
        end
        return false
    end,
}
