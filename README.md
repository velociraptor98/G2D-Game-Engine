# G2D

A small 2D game engine built on SDL2, with an entity-component-system core and
Lua-scripted levels. The sample game is a helicopter shooter over a jungle map.

## Building

```sh
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer lua pkg-config
make run
```

Run the game from the repository root; asset paths are relative to it.

## Controls

| Key | Action |
|---|---|
| Arrow keys / WASD | Fly |
| Space | Fire |
| C | Toggle collider debug overlay |
| Esc | Quit |

## Levels and scripting

Each level is a Lua file, `assets/scripts/Level<N>.lua`, defining a global
`Level` table with three parts:

- `assets`: textures, fonts and sounds, each with an `id` used elsewhere.
- `tilemap`: a `.map` file (a grid of tile codes, then a blank line, then a grid
  of 0/1 marking solid tiles), the tileset texture, tile size and scale.
- `entities`: each with an optional `tag` (unique, e.g. `"player"`) or `group`
  (e.g. `"enemies"`), and a `components` table.

Components: `transform`, `rigidbody`, `sprite`, `animation`, `boxcollider`,
`health`, `projectile_emitter`, `keyboard_controller`, `camera_follow`,
`text_label`, `audio` and `script`. See `Level1.lua` for every field in use.
Unknown component names and wrongly typed fields are reported on startup.

A `script` is a function called every frame as
`script(entity, delta_time, elapsed_ms)`. It can call `get_position`,
`set_position`, `get_velocity`, `set_velocity`, `set_rotation`,
`set_sprite_row`, `set_flip`, `set_projectile_velocity` and
`get_entity_by_tag`.

Game rules: friendly projectiles damage the `enemies` group, enemy projectiles
damage the `player` tag, `obstacles` absorb both, and enemies bounce off
`obstacles`.

## Project layout

```
src/ECS/          Entity, component pools, System, Registry (tags, groups, deferred kills)
src/Components/   Plain data structs
src/Systems/      All behaviour: movement, rendering, animation, camera, input,
                  collision, projectiles, damage, text, health bars, audio, scripts
src/EventBus/     Synchronous publish/subscribe used for collisions and key presses
src/Scripting/    Lua bindings
src/LevelLoader.* Tilemap parser and Lua level loader
tests/            Headless test suite
```

## Tests

```sh
make test                 # build and run every test
./run_tests Render        # rerun only tests whose name contains "Render"
```

Tests run headless: rendering is checked against SDL's software renderer and
audio against SDL's dummy driver, so no window, display or sound card is needed.
Run them from the repository root, since some tests load the game's real assets.
