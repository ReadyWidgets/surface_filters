local awful = require("awful")
local gears = require("gears")
local wibox = require("wibox")
local beautiful = require("beautiful")

local surface_filters = require("surface_filters.surface_filters")
local Gdk, cairo
do
	local lgi = require("lgi")
	Gdk, cairo = lgi.Gdk, lgi.cairo
end

-- TODO
-- * Add a `color` property to the shadow filter, which supports ARGB

local export = {}

---@generic T1
--- Check if a given parameter is of the expected type, otherwise throw an error
---@param func_name string string The name of the function the parameter belongs to
---@param position integer The integer position of the parameter
---@param wanted_type `T1` The type that the parameter should have
---@param value T1 The actual parameter itself
local function assert_param_type(func_name, position, wanted_type, value)
	local value_type = type(value)

	assert((value_type == wanted_type), ("Wrong type of parameter #%d passed to '%s' (expected %s, got %s)"):format(position, func_name, wanted_type, value_type))
end

--- Create a blurred copy of a `cairo.ImageSurface`
---@param input_surface cairo.ImageSurface The surface you wish to create a blurred copy of
---@param radius integer The blur radius; must be positive (higher = more blurry)
---@return cairo.ImageSurface
local function cairo_image_surface_create_blurred(input_surface, radius)
	assert_param_type("cairo_image_surface_create_blurred", 1, "userdata", input_surface)
	assert_param_type("cairo_image_surface_create_blurred", 2, "number", radius)
	assert(radius > 0, "You must provide a blur radius greater than 0!")

	return cairo.Surface(surface_filters.cairo_image_surface_create_blurred(input_surface._native, radius))
end
export.cairo_image_surface_create_blurred = cairo_image_surface_create_blurred

--- Create a shadow copy of a `cairo.ImageSurface`
---@param input_surface cairo.ImageSurface The surface you wish to create a shadow copy of
---@param radius integer The shadow radius; must be positive (higher = more blurry)
---@return cairo.ImageSurface
local function cairo_image_surface_create_shadow(input_surface, radius)
	assert_param_type("cairo_image_surface_create_shadow", 1, "userdata", input_surface)
	assert_param_type("cairo_image_surface_create_shadow", 2, "number", radius)
	assert(radius > 0, "You must provide a shadow radius greater than 0!")

	return cairo.Surface(surface_filters.cairo_image_surface_create_shadow(input_surface._native, radius))
end
export.cairo_image_surface_create_shadow = cairo_image_surface_create_shadow

--- Inject filter functions as methods into their respective cairo class. This makes them read
--- more like methods exported by LGI itself, as supposed to C-style methods.
local function bind_methods()
	cairo.ImageSurface.create_blurred = cairo_image_surface_create_blurred
	cairo.ImageSurface.create_shadow  = cairo_image_surface_create_shadow
end
export.bind_methods = bind_methods

--- Fill a cairo context with a cairo surface
---@param cr cairo.Context
---@param surface cairo.Surface
local function fill_context_with_surface(cr, surface)
	--- If the source is not reset to what it was originally, drawing anything above
	--- the context will not show up (including using `wibox.layout.stack`)
	local original_source = cr:get_source()

	cr:set_source_surface(surface, 0, 0)
	cr:paint()

	cr:set_source(original_source)

	return surface
end

--- Automatically generate a dynamic property (a getter and a setter function for a `gears.object`)
---@param object table
---@param property_name string The name of the property
---@return table object
local function gen_property(object, property_name)
	assert_param_type("gen_property", 1, "table", object)
	assert_param_type("gen_property", 2, "string", property_name)

	object["get_" .. tostring(property_name)] = function(self)
		return self._private[property_name]
	end

	object["set_" .. tostring(property_name)] = function(self, value)
		self._private[property_name] = value
		self:emit_signal("property::" .. tostring(property_name), value)
		self._private.force_redraw = true
		self:emit_signal("widget::redraw_needed")
	end

	return object
end
--- Create a copy of a table while re-using the same metatable
---@generic T1
---@param tb table
---@param __type__ `T1`
---@return T1 copy_of_tb
local function copy(tb, __type__)
	assert_param_type("copy", 1, "table", tb)

	local copy_of_tb = {}

	for k, v in pairs(tb) do
		copy_of_tb[k] = v
	end

	setmetatable(copy_of_tb, getmetatable(tb))

	return copy_of_tb
end

---@class surface_filters.common : wibox.widget.base
---@operator call: surface_filters.common
---@field widget wibox.widget.base|nil The wrapped widget
local surface_filters_common = setmetatable({
	get_widget = function(self)
		return self._private.widget
	end,

	set_widget = function(self, widget)
		local child_redraw_listener = self._private.child_redraw_listener

		do
			local old_widget = self._private.widget

			if old_widget then
				old_widget:disconnect_signal("widget::redraw_needed", child_redraw_listener)

				for _, child in ipairs(old_widget.all_children) do
					child:disconnect_signal("widget::redraw_needed", child_redraw_listener)
				end
			end
		end

		widget:connect_signal("widget::redraw_needed", child_redraw_listener)

		for _, child in ipairs(widget.all_children) do
			child:connect_signal("widget::redraw_needed", child_redraw_listener)
		end

		return wibox.widget.base.set_widget_common(self, widget)
	end,

	get_children = function(self)
		return { self._private.widget }
	end,

	set_children = function(self, children)
		return self:set_widget(children[1])
	end,

	draw = function(self, context, cr, width, height)
		local child = self:get_widget()

		if child == nil then
			return
		end

		if (not self._private.force_redraw) and ((self._private.cached_surface ~= nil)) then
			fill_context_with_surface(cr, self._private.cached_surface)
			return
		end

		local surface
		if self.on_draw == nil then
			surface = wibox.widget.draw_to_image_surface(child, width, height)
		else
			surface = self:on_draw(cr, width, height, child)
		end

		self._private.force_redraw = false
		self._private.cached_surface = surface

		return fill_context_with_surface(cr, surface)
	end,

	__name = "surface_filters.common",
}, {
	__call = function(cls, kwargs)
		if kwargs == nil then
			kwargs = {}
		end

		local self = gears.table.crush(wibox.widget.base.make_widget(nil, cls.__name, {
			enable_properties = true
		}), cls)

		if self._private == nil then
			self._private = {}
		end

		function self._private.child_redraw_listener()
			self._private.force_redraw = true
			return self:emit_signal("widget::redraw_needed")
		end

		if cls.parse_kwargs ~= nil then
			cls.parse_kwargs(kwargs)
		end

		for k, v in pairs(kwargs) do
			self[k] = v
		end

		return self
	end
})

---@class surface_filters.blur : surface_filters.common
---@field radius integer The blur radius
---@field dual_pass boolean If `true`, split the blurring process into two stages (creates a softer look)
local blur
do
	blur = copy(surface_filters_common, "surface_filters.blur")
	gen_property(blur, "radius")
	blur.__name = "surface_filters.blur"

	function blur.on_draw(self, cr, w, h, child)
		assert(type(self.radius) == "number")

		local surface = wibox.widget.draw_to_image_surface(child, w, h)

		---@type cairo.ImageSurface
		local surface_processed
		if self.dual_pass then
			local half = self.radius / 2
			surface_processed = cairo_image_surface_create_blurred(cairo_image_surface_create_blurred(surface, math.ceil(half)), math.floor(half))
		else
			surface_processed = cairo_image_surface_create_blurred(surface, self.radius)
		end

		return fill_context_with_surface(cr, surface_processed)
	end

	function blur.parse_kwargs(kwargs)
		if kwargs.radius == nil then
			kwargs.radius = 10
		end

		if kwargs.opacity == nil then
			kwargs.opacity = 1.0
		end

		if kwargs.dual_pass == nil then
			kwargs.dual_pass = false
		end

		return kwargs
	end
end
export.blur = blur

---@class surface_filters.shadow : surface_filters.common
---@field radius integer The shadow radius
---@field dual_pass boolean If `true`, split the blurring process into two stages (creates a softer look)
local shadow
do
	shadow = copy(surface_filters_common, "surface_filters.shadow")
	gen_property(shadow, "radius")
	shadow.__name = "surface_filters.shadow"

	function shadow.on_draw(self, cr, w, h, child)
		assert(type(self.radius) == "number")

		local surface = wibox.widget.draw_to_image_surface(child, w, h)

		---@type cairo.ImageSurface
		local surface_processed
		if self.dual_pass then
			local half = self.radius / 2
			surface_processed = cairo_image_surface_create_shadow(cairo_image_surface_create_shadow(surface, math.ceil(half)), math.floor(half))
		else
			surface_processed = cairo_image_surface_create_shadow(surface, self.radius)
		end

		return fill_context_with_surface(cr, surface_processed)
	end

	function shadow.parse_kwargs(kwargs)
		if kwargs.radius == nil then
			kwargs.radius = 10
		end

		if kwargs.opacity == nil then
			kwargs.opacity = 1.0
		end

		if kwargs.dual_pass == nil then
			kwargs.dual_pass = false
		end

		return kwargs
	end
end
export.shadow = shadow

return export
