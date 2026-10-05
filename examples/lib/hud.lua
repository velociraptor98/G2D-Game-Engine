-- Shared helpers for the example scenes: fonts, a title, lines of text and a
-- footer. Scenes load it with require("examples.lib.hud").
local hud = {}

hud.fonts = {
    { type = "font", id = "hud-title", file = "assets/fonts/charriot.ttf", font_size = 28 },
    { type = "font", id = "hud-menu", file = "assets/fonts/arial.ttf", font_size = 20 },
    { type = "font", id = "hud-text", file = "assets/fonts/arial.ttf", font_size = 15 },
    { type = "font", id = "hud-small", file = "assets/fonts/arial.ttf", font_size = 12 },
}

local TEXT_COLOR = { r = 225, g = 225, b = 225 }

-- `assets` with the HUD fonts added.
function hud.with_fonts(assets)
    local all = {}
    for _, asset in ipairs(assets or {}) do all[#all + 1] = asset end
    for _, font in ipairs(hud.fonts) do all[#all + 1] = font end
    return all
end

function hud.label(x, y, text, options)
    options = options or {}
    return {
        tag = options.tag,
        components = {
            text_label = {
                position = { x = x, y = y },
                text = text,
                font_id = options.font or "hud-text",
                color = options.color or TEXT_COLOR,
                fixed = options.fixed ~= false,
                centred = options.centred or false,
            },
        },
    }
end

function hud.title(text)
    return hud.label(16, 12, text, { font = "hud-title", color = { r = 255, g = 220, b = 120 } })
end

-- One label per line, starting at (x, y).
function hud.lines(x, y, lines, options)
    options = options or {}
    local spacing = options.spacing or 20
    local labels = {}
    for i, line in ipairs(lines) do
        labels[#labels + 1] = hud.label(x, y + (i - 1) * spacing, line, options)
    end
    return labels
end

function hud.footer(text)
    return hud.label(16, 578, text or "ESC back", { font = "hud-small", color = { r = 160, g = 160, b = 160 } })
end

-- A label scripts can update with set_text(get_entity_by_tag(tag), "...").
function hud.status(x, y, tag, text)
    return hud.label(x, y, text or "", { tag = tag, color = { r = 140, g = 220, b = 255 } })
end

-- Flattens any mix of entity definitions and lists of them into one list.
function hud.entities(...)
    local all = {}
    for _, item in ipairs({ ... }) do
        if item.components then
            all[#all + 1] = item
        else
            for _, definition in ipairs(item) do all[#all + 1] = definition end
        end
    end
    return all
end

return hud
