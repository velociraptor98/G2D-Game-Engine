-- Text labels and sound. A sound plays when an entity with an audio component
-- appears and stops when that entity goes away.
local hud = require("examples.lib.hud")

local LOOP_ON = "looping sound: ON  (L to stop)"
local LOOP_OFF = "looping sound: off  (L to start)"

Scene = {
    background_color = { r = 30, g = 26, b = 40 },
    assets = hud.with_fonts({
        { type = "font", id = "big", file = "assets/fonts/charriot.ttf", font_size = 44 },
        { type = "sound", id = "rotor", file = "assets/sounds/helicopter.wav" },
    }),
    entities = hud.entities(
        hud.title("TEXT AND AUDIO"),
        hud.label(400, 130, "CENTRED, LARGE, COLOURED", {
            font = "big", centred = true, color = { r = 255, g = 140, b = 90 },
        }),
        hud.label(400, 180, "Text labels pick a font asset, a colour and a position.", { centred = true }),
        hud.label(400, 202, "Scripts can change them while the scene runs.", {
            centred = true, font = "hud-small", color = { r = 150, g = 150, b = 170 },
        }),
        hud.label(400, 250, "", { tag = "clock", centred = true, color = { r = 140, g = 220, b = 255 } }),
        hud.lines(160, 330, {
            "P   play a 1.5 second burst: an entity with audio + lifetime { duration = 1500 }",
            "L   start or stop a looping sound by spawning or killing its entity",
        }),
        hud.status(160, 400, "loop-state", LOOP_OFF),
        hud.footer()
    ),

    on_update = function(delta_time, elapsed_ms)
        set_text(get_entity_by_tag("clock"), string.format("this label is updated every frame: %.1f s", elapsed_ms / 1000))
    end,

    on_key = function(key)
        if key == "P" then
            spawn({ components = { audio = { sound_id = "rotor", loop = true, volume = 90 }, lifetime = { duration = 1500 } } })
            return true
        elseif key == "L" then
            local loop = get_entity_by_tag("loop")
            if loop then
                kill(loop)
                set_text(get_entity_by_tag("loop-state"), LOOP_OFF)
            else
                spawn({ tag = "loop", components = { audio = { sound_id = "rotor", loop = true, volume = 50 } } })
                set_text(get_entity_by_tag("loop-state"), LOOP_ON)
            end
            return true
        end
        return false
    end,
}
