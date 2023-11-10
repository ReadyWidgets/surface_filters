# Cairo Surface Filters for Awesome WM

This is a rewrite of [@ReadyWidgets/filters](https://github.com/ReadyWidgets/filters). It is much faster, better structured and uses a proper build system

## Install

To download, compile and install this module, run this in your terminal:

> [!IMPORTANT]
> The default built target is LuaJIT! If you want to target another Lua version, like 5.3, open `meson.build` and edit the marked line.

```sh
git clone 'https://github.com/ReadyWidgets/surface_filters.git' "${XDG_CONFIG_HOME:-$HOME/.config}/awesome/surface_filters"
cd "${XDG_CONFIG_HOME:-$HOME/.config}/awesome/surface_filters"
meson setup --reconfigure builddir
meson compile -C builddir
```

## Usage

To try out this module, you can put something like this in your `rc.lua`:

```lua
local surface_filters = require("surface_filters")

local box = wibox {
    height  = scale(300),
    width   = scale(300),
    visible = true,
    ontop   = true,
    bg      = "#202020",
    widget  = {
        {
            image  = "#{gears.filesystem.get_configuration_dir()}/surface_filters/test1.png",
            halign = "center",
            valign = "center",
            widget = wibox.widget.imagebox,
        },
        dual_pass = true,
        radius    = 5,
        widget    = surface_filters.blur,
    }
}

awful.placement.bottom_left(box, { honor_workarea = true, margins = scale(5) })
```