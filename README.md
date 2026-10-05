# G2D

A small 2D game engine built on SDL2: an entity-component-system core, scenes
written in Lua, and a C++ API for games that need more. It ships with a
showcase of focused demos and one complete example game.

## Quick start

```sh
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer lua pkg-config
make                                   # builds g2d and extension_demo
./g2d                                  # the showcase: pick a demo with 1-8
./g2d games/jungle/level1.lua          # the example game (also: make jungle)
./extension_demo                       # a game extending the engine in C++
make test                              # headless test suite
```

`g2d` runs any scene file. Paths inside scenes are relative to the folder
holding the executable, so it works from any working directory.

## Layout

```
engine/            the engine, built as build/libg2d.a
  Core/            Engine: window, main loop, scene loading, Lua engine API
  ECS/             entities, component pools, systems, tags and groups
  Components/      plain data structs
  Systems/         all behaviour
  Scene/           scene loader, tilemap parser, Lua table reader
  Scripting/       Lua bindings for entities
  Assets/ EventBus/ Events/
apps/g2d/          the g2d scene runner
examples/          showcase.lua, one demo scene per feature, and cpp-extension/
games/jungle/      the example game, written entirely as a scene file
assets/            the shared art, fonts, sounds and tilemaps
tests/
```

## Scenes

A scene file defines a global `Scene` table:

```lua
Scene = {
    background_color = { r = 20, g = 20, b = 30 },
    world = { width = 1600, height = 1200 },        -- defaults to the window, or the tilemap's size
    assets = {
        { type = "texture", id = "hero", file = "assets/images/chopper-spritesheet.png" },
        { type = "font", id = "ui", file = "assets/fonts/arial.ttf", font_size = 16 },
        { type = "sound", id = "rotor", file = "assets/sounds/helicopter.wav" },
    },
    tilemap = { map_file = "assets/tilemaps/jungle.map", texture_id = "tiles", tile_size = 32, scale = 2 },
    entities = {
        { tag = "player", group = "heroes", components = { --[[ see below ]] } },
    },
    on_start = function() end,                       -- once, after the scene is built
    on_update = function(delta_time, elapsed_ms) end, -- every frame
    on_key = function(key) return false end,         -- key presses; return true to consume
}
```

Unknown fields, unknown components and wrongly typed values are reported when
the scene loads, naming where they are (e.g. `entities[3].sprite: unknown
field 'z_idx'`). `Esc` returns to the scene that opened this one, or quits,
unless `on_key` consumes it. Scenes can `require` shared Lua modules by
root-relative name, as the examples do with `examples.lib.hud`.

A `.map` file is a grid of two-digit tile codes (row, column in the tileset),
optionally followed by a blank line and a same-sized grid of 0/1 marking solid
tiles.

### Components

| Component | Fields (all optional) |
|---|---|
| `transform` | `position {x,y}`, `scale {x,y}`, `rotation` (degrees) |
| `rigidbody` | `velocity {x,y}` |
| `sprite` | `texture_id`, `width`, `height`, `z_index`, `fixed` (screen space), `src_rect_x`, `src_rect_y`, `flip` (`none`/`horizontal`/`vertical`) |
| `animation` | `num_frames`, `speed_rate` (frames per second), `loop` |
| `boxcollider` | `width`, `height`, `offset {x,y}` (sprite pixels, scaled by the transform) |
| `solid` | marks a collider as a wall |
| `collision_response` | `on_solid` (`none`/`bounce`/`block`/`destroy`), `flip_sprite_on_bounce` |
| `health` | `health`, `max_health`, `show_bar` |
| `team` | `name`; entities on the same team never damage each other |
| `damage_on_contact` | `damage`, `destroy_on_contact` |
| `lifetime` | `duration` (ms); the entity is removed when it runs out |
| `keyboard_controller` | `speed`, `keys {up,down,left,right = {"Up","W"}}`, `sprite_rows {up,down,left,right}` |
| `projectile_emitter` | `projectile_velocity {x,y}`, `repeat_frequency` (ms), `trigger_key`, `aim` (`fixed`/`facing`), `projectile {texture_id, width, height, z_index, damage, lifetime}` |
| `stay_in_bounds` | keeps the sprite inside the world |
| `camera_follow` | the camera centres on this entity |
| `text_label` | `position {x,y}`, `text`, `font_id`, `color {r,g,b,a}`, `fixed`, `centred` |
| `audio` | `sound_id`, `loop`, `volume` (0-128); plays while the entity exists |
| `script` | a function called every frame as `script(entity, delta_time, elapsed_ms)` |

Projectiles join their shooter's team, damage what they hit once, and are
destroyed by solids.

### Lua API

Entities: `get_position`, `set_position`, `get_velocity`, `set_velocity`,
`set_rotation`, `set_sprite_row`, `set_flip`, `set_projectile_velocity`,
`set_text`, `get_health`, `get_entity_by_tag`, `get_entities_in_group`,
`is_alive`, `kill`, and `spawn(definition)`, which takes the same table as an
entry in `entities`. Calls on dead entities are ignored.

Engine: `load_scene(path)`, `reload_scene()`, `back()`, `quit()`,
`set_debug_overlay(on)`, `toggle_debug_overlay()` (draws colliders),
`get_world_size()`.

Keys arrive in `on_key` by SDL name: `"A"`, `"1"`, `"Space"`, `"Escape"`,
`"Left"`.

## Extending from C++

Link against `build/libg2d.a` and include from `engine/`.
`examples/cpp-extension/main.cpp` is a complete example:

```cpp
Engine engine;
engine.Init();
engine.GetSceneLoader().RegisterComponent("spin", [](const TableReader &fields, Entity entity) {
    entity.AddComponent<SpinComponent>(fields.Float("degrees_per_second", 90.0f));
});
auto &spin = engine.GetRegistry().AddSystem<SpinSystem>();
engine.AddUpdateHook([&spin](float deltaTime) { spin.Update(deltaTime); });
engine.Run("examples/cpp-extension/scene.lua");
```

Registering an existing name replaces the built-in loader.

## Tests

```sh
make test                 # build and run every test
./run_tests Scene         # rerun only tests whose name contains "Scene"
```

Tests run headless on SDL's dummy video and audio drivers and its software
renderer, so no window, display or sound card is needed. They include loading
and running every shipped scene, which fails on any scene warning. Run them
from the repository root.
