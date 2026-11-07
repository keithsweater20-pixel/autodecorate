# Autodecorate

A Geode mod for Geometry Dash 2.208 (iOS, Android, Windows, macOS). It decorates a layout with one tap.

## How to use

1. Open a level in the editor and tap **Autodecorate**, next to the **Copy** button.
2. Pick a style, pattern, density, colors and the options on the three tabs (Look, Colors, More).
3. **Preview** shows colored dots where decoration would go. It adds nothing. Tap Preview again to hide it.
4. **Generate** adds the decoration. Nothing changes until you press it. It is one editor Undo step.
5. **Regenerate** removes the last generation (if it is still your newest editor action) and makes a new one with a new seed.
6. **Undo** removes the last generation under the same condition.

## What it does

- Finds layout blocks, spikes, slopes and long slopes by their collision type and shape. No object IDs are used.
- Adds decoration only to exposed surfaces. Faces hidden by neighbouring layout objects get nothing.
- Never moves, resizes or edits your layout objects.
- Only uses decoration objects that have no collision and no trigger behaviour. Anything else is removed before it is kept.
- Never places decoration inside layout geometry, keeps clear of spikes, and skips spots already taken by existing decoration, so running it twice does not stack duplicates.
- With "Keep play area clear" on, decoration rising from floors is capped at about one block tall.
- Decoration goes on back z-layers, behind the layout.

## Options

| Option | What it does |
|---|---|
| Style | Grief, Tidal Wave, Last Hope, Limbo. Tidal Wave has an **Underwater** and a **Beach** look. |
| Pattern | Edge (along surfaces), Scatter (drifting off surfaces), Rhythm (evenly spaced), Cluster (groups). |
| Density | How much decoration. |
| Layers (1-3) | Back, middle and front depth layers. All three stay behind the layout. 1 puts everything on one layer. |
| Background | Off / Light / Full. Large faint shapes in the empty space around the layout (not in open sky), on the back layer. |
| Lighting | Strength of the detail color's glow (needs "Add color triggers"). |
| Detail | How many small accent objects. |
| Variation | How random sizes and rotations are. |
| Seed | Same seed + settings + layout = same result. Regenerate picks a new seed. |
| Background color | X = leave your level's background alone, A = the style's color, or a swatch. Needs "Add color triggers". |

## Styles

| Style | Look |
|---|---|
| Grief | Hell style. Black and gray, blood-red thorns and chains, white neon glints. |
| Tidal Wave (Underwater) | Curling waves, rising bubbles, kelp in cyan and deep blue. |
| Tidal Wave (Beach) | Sky blue background with orange and purple, wave curls, clouds and sun shapes. |
| Last Hope | Dark sky with luminous gold and pale-blue rays, sparks and feathers. |
| Limbo | A dark void with glowing teal and magenta shapes, strange roots and symbols. |

These are interpretations of the style names (the real levels' block decoration is not documented anywhere I could find). They are data tables in `src/core/Styles.cpp`, so they are easy to retune. Geometry Dash has no ready-made palm trees, pentagrams or demon figures to find by name, so the styles use whatever shapes the game's decoration set offers.

## Colors

The mod uses color channels 997 and 998 (and 1000, the background channel, if you choose a background color). If "Add color triggers" is on, it adds Color triggers above the level start so your chosen colors show. Change the channels in `Settings.hpp` if they clash with your level.

## Building the .geode (iOS)

Uses the same workflow setup that built TpllKit (pinned bindings). Push to `main` and the `.geode` appears under Releases as `autodecorate-latest`.

A `.geode` has to be compiled, and iOS builds need a Mac, so GitHub's Mac servers do it.

1. Put this project in a GitHub repo (the `.github/workflows/build.yml` file must be included).
2. Open the **Actions** tab and run **Build Autodecorate (iOS)**. It also runs on every push.
3. When it finishes (about 10 minutes), open the run and download the **Autodecorate-iOS-geode** artifact. The `.geode` is inside.
4. Install it with your usual Geode setup.

The workflow builds iOS only, to keep it fast. To add Windows, Android or macOS, copy the matrix from
https://github.com/geode-sdk/build-geode-mod/blob/main/examples/multi-platform.yml

Targets Geode 5.11.2 and Geometry Dash 2.208.

## Layout

- `src/core/`: engine-independent logic (detection geometry, styles, placement). Has an off-device test in `tests/core_test.cpp`.
- `src/gd/`: Geode glue (button, popup, scanning real objects, creating objects).
- `src/gd/Compat.hpp`: every GD binding call that might differ between Geode versions, in one place.
