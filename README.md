# Hair Color Sync - NG

An SKSE plugin that copies each actor's hair color onto every equipped mesh that uses the hair-tint shader. Built with CommonLibSSE NG, so the same plugin runs on Skyrim Special Edition, Anniversary Edition, GOG, and VR.

## What it is

Hair Color Sync watches the moments when an actor's body is built or rebuilt, then paints that actor's hair color onto the new geometry.

The usual targets are the hair and long-hair slots. Any other biped slot is included when its mesh opts into the same shader: custom hair packs, some circlets, tails, ears, and armor or body parts that tint with the hair color. Slots that do not use the shader are left alone.

The plugin is `SyncHairColor.dll`. It needs SKSE. RaceMenu can be present or absent; the tint path is the same either way.

## Why it's needed

Hair color is stored on the NPC record. The engine is supposed to push that color onto hair-tint geometry when the mesh is created. That push is easy to miss:

- An actor attaches to a cell, or a save finishes loading, before the 3D exists.
- Equipping or unequipping a helmet, wig, or piece of armor rebuilds the biped parts, and a burst of those updates can finish before the first tint sticks.
- A magic effect that changes the actor applies or wears off, and the mesh comes back untinted. On removal the mesh is still fading out when the event fires.
- RaceMenu rebuilds the actor's node graph after the engine has already had its chance to apply the color.

What you see is hair, or any other hair-tinted mesh, drawn in the color authored on the mesh. Looking away and back, or equipping something, sometimes fixes it by forcing another update. This plugin applies the actor's color at the moments it gets lost.

Hooking the engine functions that apply the tint collides with RaceMenu, which already patches biped attach. This plugin leaves those functions alone and reapplies the color after the rebuild.

## How it works

After the game data loads, the plugin listens for four notifications:

- the actor equips or unequips something
- the actor attaches to a cell
- an active effect is applied to or removed from the actor
- SKSE reports that the actor's 3D node was updated

The node-update event is the one that covers RaceMenu and other mesh rebuilds. After a save loads, the plugin also schedules a tint for the player and waits longer, because the player mesh is not ready when the load event fires.

Each sync reads the hair color from the actor base (`headRelatedData->hairColor`) and walks the actor's current biped. Every slot whose cloned mesh has the hair-tint shader gets `UpdateHairColor` with that color. An actor with no hair color record is skipped.

The notification usually arrives before the mesh can take the color, or another system rebuilds the mesh immediately afterward. A background timer waits, then runs the tint on the game thread. If the 3D is still missing, it tries again a few times and then stops. An equip gets a second pass so a burst of equipment changes cannot wipe the first tint. While a sync for one actor is already in flight, further requests for that actor are ignored.

A populated cell can ask for a player refresh once per NPC. Those requests are folded into one: the plugin waits for a short quiet gap, then tints the player a single time.
