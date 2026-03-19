# Scriptable Block

A Geode mod that adds a **programmable block** to the Geometry Dash level editor. Write code using a custom scripting language to control game objects, the player, camera, and colors.

## Features

- **Custom DSL** - Simple scripting language designed for GD
- **4 Trigger Modes** - On Collision, On Level Start, Continuous, On Click
- **Object Control** - Move, rotate, scale, toggle, color any group
- **Player Control** - Speed, gravity, pulse, trail, kill, jump
- **Camera Control** - Move, zoom, shake, rotate
- **Color Channels** - Set and pulse any color channel
- **Safe Execution** - Instruction limits prevent freezing

## Quick Start

1. Open the level editor
2. Click the Script button to place a Scriptable Block
3. Select the block and click Edit to open the code editor
4. Write your script and choose a trigger mode
5. Play the level to see your script in action!

## Example Scripts

```
// Move an object group on collision
obj(1).move(100, 50)
camera.shake(0.5, 0.3)
```

```
// Continuous color pulse
color(1).pulse(255, 0, 0, 0.5)
wait(1)
color(1).pulse(0, 255, 0, 0.5)
wait(1)
```

```
// React to player position
if player.y > 300 {
    obj(2).toggle(true)
} else {
    obj(2).toggle(false)
}
```
