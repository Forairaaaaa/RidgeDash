# RidgeDash Architecture Notes

The game is moving from a fast-iteration single owner shape toward small systems with clear edges.

## Refactor Order

1. Pickups
   - `RidgeDashGame` should talk to one `PickupSystem`.
   - Concrete pickups keep their own spawn, update, collect, and draw behavior.
   - Adding a pickup should not require changing the main game loop.

2. Vehicle
   - Move chassis, wheels, head joint, ground state, input forces, and vehicle rendering behind a `Vehicle` owner.
   - Pickups should apply effects through vehicle methods instead of reaching into Box2D ids directly.

3. UI
   - Move HUD, start tips, pause, game over, and their animations behind a UI/overlay owner.
   - UI should read a compact run snapshot instead of depending on the whole game object.

## Current Guideline

Keep refactors behavior-preserving first. Tune gameplay and art only after each boundary is stable and verified.

## Current Layout

- `game/` keeps the top-level game orchestration, shared config, and rendering bridge.
- `game/pickups/` keeps pickup-specific systems and effects.
- `game/run/` keeps run lifecycle, stats, records, save data, and trick tracking.
- `game/ui/` keeps input mapping, HUD/panels, pause menu behavior, and display-scale helpers.
- `game/vehicle/` keeps vehicle physics, driver state, dust, and vehicle drawing.
- `game/world/` keeps terrain, terrain profiles, and environment/background systems.

## Current Status

- `PickupSystem` owns pickup fan-out for reset, stream, update, collect, trim, and draw.
- `Vehicle` owns Box2D ids, shape identity checks, drive input forces, grounded state, driver expression state, dust effects, vehicle drawing, and vehicle physics operations used by pickups.
- `GameUi` owns HUD, start tips, pause/game-over panel drawing, score popup state, and overlay animations. `RidgeDashGame` now passes compact view data into UI instead of storing UI animation fields itself.
- `TerrainSystem` owns generated terrain points, terrain body lifetimes, biome/profile selection, terrain drawing, friction setup, and terrain shape identity checks.
- `RunStats` owns per-run distance, total score, coin/trick score, coin and flip counts, and new-record state. Pickups and flip logic add score through this object instead of editing scattered fields.
- `RunRecords` owns loading, exposing, submitting, and saving the persisted best run.
- `RunController` owns run state, pause transitions, fuel, head-hit crash state, game-over timing, and end-condition timers.
- `TrickTracker` owns in-run trick detection state, currently the flip angle tracking and bonus scoring.
- `GameInput` owns gameplay, command, and menu key mapping. Other game systems consume input snapshots instead of reading platform keys directly.
- `PauseMenuController` owns pause menu actions and desktop display-scale selection state.
