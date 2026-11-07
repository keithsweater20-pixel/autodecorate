# Autodecorate

A Geode mod for Geometry Dash 2.208 (iOS, Android, Windows, macOS). It decorates a layout with one tap.

## How to use

1. Open a level in the editor.
2. Tap **Autodecorate**, next to the **Copy** button.
3. Choose a style, pattern, density and colors.
4. Tap **Apply**. Nothing changes until you do.
5. Undo removes everything from one Apply in a single step.

## What it does

- Finds layout blocks, spikes, slopes and long slopes by their collision type and shape. No object IDs are used.
- Adds decoration only to exposed surfaces. Faces hidden by neighbouring layout objects get nothing.
- Never moves, resizes or edits your layout objects.
- Only uses decoration objects that have no collision and no trigger behaviour. Anything else is removed before it is kept.
- Never places decoration inside layout geometry, and keeps clear of spikes.
- With "Keep play area clear" on, decoration rising from floors is capped at about one block tall.
- Decoration goes on a back z-layer, behind the layout.

## Styles

| Style | Look |
|---|---|
| Grief | Thorns, hanging chains, drifting ash. Violet and crimson. |
| Tidal Wave | Curling waves, bubbles, kelp. Cyan and deep blue. |
| Last Hope | Light rays, sparks, feathers. Gold and pale blue. |
| Limbo | Dark grass and branches, hanging roots, thin mist. |

These are interpretations of the style names. They are data tables in `src/core/Styles.cpp`, so they are easy to retune.

Patterns: **Edge** (along surfaces), **Scatter** (drifting off surfaces), **Rhythm** (evenly spaced), **Cluster** (groups).

## Colors

The mod uses color channels 997 and 998. If "Add color triggers" is on, it adds two Color triggers above the level start so your chosen colors show. Change the channels in `Settings.hpp` if they clash with your level.

## Building the .geode (iOS)

Uses the same workflow setup that built TpllKit (pinned bindings). Push to `main` and the `.geode` appears under Releases as `autodecorate-latest`.

A `.geode` has to be compiled, and iOS builds need a Mac, so GitHub's Mac servers do it.

1. Put this project in a GitHub repo (the `.github/workflows/build.yml` file must be included).
2. Open the **Actions** tab and run **Build Autodecorate (iOS)**. It also runs on every push.
3. When it finishes (about 10 minutes), open the run and download the **Autodecorate-iOS-geode** artifact. The `.geode` is inside.
4. Install it with your usual Geode setup.

The workflow builds iOS only, to keep it fast. To add Windows, Android or macOS, copy the matrix from
https://github.com/geode-sdk/build-geode-mod/blob/main/examples/multi-platform.yml

Targets Geode v5.11.2 and Geometry Dash 2.208.

## Layout

- `src/core/`: engine-independent logic (detection geometry, styles, placement). Has an off-device test in `tests/core_test.cpp`.
- `src/gd/`: Geode glue (button, popup, scanning real objects, creating objects).
- `src/gd/Compat.hpp`: every GD binding call that might differ between Geode versions, in one place.
