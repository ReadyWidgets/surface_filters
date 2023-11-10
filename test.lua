local surface_filters = require("surface_filters.surface_filters")
local cairo = require("lgi").cairo

local function apply_blur(self, radius)
	return cairo.Surface(surface_filters.cairo_image_surface_apply_blur(self._native, radius))
end

local function apply_shadow(self, radius)
	return cairo.Surface(surface_filters.cairo_image_surface_apply_shadow(self._native, radius))
end

local input_file = arg[1] or tostring(arg[0]:gsub('/.*$', '') or '.') .. "/test2.png"
local output_file = tostring(input_file:gsub('%.[^%.]+$', '')) .. "_blurred.png"

do
	local file = assert(io.open(output_file, "w"))
	file:write("")
	file:close()
end

do
	print("INFO: Applying blur filter to '" .. tostring(input_file) .. "'!")
	local surface = cairo.ImageSurface.create_from_png(input_file)
	local surface_processed = apply_blur(surface, 10)
	print("INFO: Writing blurred file to '" .. tostring(output_file) .. "'!")
	return print(surface_processed:write_to_png(output_file))
end
