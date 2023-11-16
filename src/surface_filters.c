#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <cairo/cairo.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "surface_filters.h"

#define PROJECT_NAME "surface_filters"

///////////////////////////////////////////////////////////////////////////////

/// NOTE:
/// I would like to include the stack blur algorithm here. It's faster than gaussian blurring
/// while looking almost the same. However, for an unknown reason, this seems to just not work :/
/// See: https://gitlab.gnome.org/Archive/lasem/-/blob/master/src/lsmsvgfiltersurface.c#L159

///////////////////////////////////////////////////////////////////////////////

void cairo_image_surface_create_blurred(cairo_surface_t *restrict input_surface, cairo_surface_t *restrict output_surface, const guint radius) {
	// TODO: Use a gaussian kernel
	//gdouble *kernel = generate_blur_kernel_linear(radius*2, radius);

	gint width     = cairo_image_surface_get_width(input_surface);
	gint height    = cairo_image_surface_get_height(input_surface);
	//gint rowstride = cairo_image_surface_get_stride(input_surface);

	cairo_surface_flush(output_surface);

	ARGBPixel *input_pixels  = (ARGBPixel *)cairo_image_surface_get_data(input_surface);
	ARGBPixel *output_pixels = (ARGBPixel *)cairo_image_surface_get_data(output_surface);

	/*
	g_print("cairo_image_surface_get_format(output_surface) = ");
	switch (cairo_image_surface_get_format(output_surface)) {
		case CAIRO_FORMAT_INVALID:   g_print("INVALID\n"); break;
		case CAIRO_FORMAT_ARGB32:    g_print("ARGB32\n"); break;
		case CAIRO_FORMAT_RGB24:     g_print("RGB24\n"); break;
		case CAIRO_FORMAT_A8:        g_print("A8\n"); break;
		case CAIRO_FORMAT_A1:        g_print("A1\n"); break;
		case CAIRO_FORMAT_RGB16_565: g_print("RGB16_565\n"); break;
		case CAIRO_FORMAT_RGB30:     g_print("RGB30\n"); break;
		case CAIRO_FORMAT_RGB96F:    g_print("RGB96F\n"); break;
		case CAIRO_FORMAT_RGBA128F:  g_print("RGBA128F\n"); break;
	}
	//*/

	/// NOTE:
	/// With gaussian blur, there's a trick you can use to make it faster: Instead of
	/// applying a blurring square to each pixel (which scales O(N^2) (very bad)), do
	/// two passes, one for the horizontal and one for the vertical (which scales O(N)).

	gulong end = width * height;

	gdouble *kernel = generate_blur_kernel_linear(radius*2, radius);

	// Horizontal pass
	for (gulong i = 0; i < end; i++) {
		gulong sum_alpha = 0;
		gulong sum_red   = 0;
		gulong sum_green = 0;
		gulong sum_blue  = 0;

		for (gint offset = -radius; offset < (gint)radius; offset++) {
			guint true_index = int_clamp(i + offset, 0, end);
			gdouble kernel_point = kernel[radius + offset];
			sum_alpha += input_pixels[true_index].alpha * kernel_point;
			sum_red   += input_pixels[true_index].red   * kernel_point;
			sum_green += input_pixels[true_index].green * kernel_point;
			sum_blue  += input_pixels[true_index].blue  * kernel_point;
		}

		output_pixels[i].alpha = sum_alpha;
		output_pixels[i].red   = sum_red;
		output_pixels[i].green = sum_green;
		output_pixels[i].blue  = sum_blue;
	}

	// Vertical pass
	for (gulong i = 0; i < end; i++) {
		gulong sum_alpha = 0;
		gulong sum_red   = 0;
		gulong sum_green = 0;
		gulong sum_blue  = 0;
		guint current_column = i % width;

		for (gint offset = -radius; offset < (gint)radius; offset++) {
			guint true_index = int_clamp(i + (offset * width), current_column, end - current_column);
			gdouble kernel_point = kernel[radius + offset];
			sum_alpha += output_pixels[true_index].alpha * kernel_point;
			sum_red   += output_pixels[true_index].red   * kernel_point;
			sum_green += output_pixels[true_index].green * kernel_point;
			sum_blue  += output_pixels[true_index].blue  * kernel_point;
		}

		output_pixels[i].alpha = sum_alpha;
		output_pixels[i].red   = sum_red;
		output_pixels[i].green = sum_green;
		output_pixels[i].blue  = sum_blue;
	}

	g_free(kernel);

	cairo_surface_mark_dirty(output_surface);
}

DEF(export_cairo_image_surface_create_blurred) {
	cairo_surface_t *input_surface = (cairo_surface_t *)lua_touserdata(L, 1);
	gint radius = lua_tonumber(L, 2);

	cairo_surface_t *output_surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		cairo_image_surface_get_width(input_surface),
		cairo_image_surface_get_height(input_surface)
	);

	cairo_image_surface_create_blurred(input_surface, output_surface, radius);

	lua_pushlightuserdata(L, output_surface);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////

void cairo_image_surface_create_shadow(cairo_surface_t *restrict input_surface, cairo_surface_t *restrict output_surface, const guint radius) {
	//gint width     = cairo_image_surface_get_width(input_surface);
	gint height    = cairo_image_surface_get_height(input_surface);
	gint rowstride = cairo_image_surface_get_stride(input_surface);

	cairo_surface_flush(output_surface);

	guchar *input_pixels  = cairo_image_surface_get_data(input_surface);
	guchar *output_pixels = cairo_image_surface_get_data(output_surface);

	gulong end = rowstride * height;

	gdouble *kernel = generate_blur_kernel_linear(radius*2, radius);

	for (gulong i = 3; i < end; i += 4) {
		gulong sum_horizontal = 0;

		for (gint offset = -radius; offset < (gint)radius; offset++) {
			guint true_index = int_clamp(i + (offset * 4), 0, end);
			sum_horizontal += input_pixels[true_index] * kernel[radius + offset];
		}

		output_pixels[i] = sum_horizontal;
	}

	for (gulong i = 3; i < end; i += 4) {
		guint current_column = i % rowstride;
		gulong sum_vertical = 0;

		for (gint offset = -radius; offset < (gint)radius; offset++) {
			guint true_index = int_clamp(i + (offset * rowstride), current_column, (end - current_column));
			sum_vertical += output_pixels[true_index] * kernel[radius + offset];
		}

		output_pixels[i] = sum_vertical;
	}

	g_free(kernel);

	for (gulong i = 3; i < end; i += 4) {
		output_pixels[i - 3] = 0;
		output_pixels[i - 2] = 0;
		output_pixels[i - 1] = 0;
	}

	cairo_surface_mark_dirty(output_surface);
}

DEF(export_cairo_image_surface_create_shadow) {
	cairo_surface_t *input_surface = (cairo_surface_t *)lua_touserdata(L, 1);
	gint radius = lua_tonumber(L, 2);

	cairo_surface_t *output_surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		cairo_image_surface_get_width(input_surface),
		cairo_image_surface_get_height(input_surface)
	);

	cairo_image_surface_create_shadow(input_surface, output_surface, radius);

	lua_pushlightuserdata(L, output_surface);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////

static const struct luaL_Reg surface_filters[] = {
	{ "cairo_image_surface_create_blurred", export_cairo_image_surface_create_blurred },
	{ "cairo_image_surface_create_shadow",  export_cairo_image_surface_create_shadow  },
	{ NULL, NULL }
};

int luaopen_surface_filters_surface_filters(lua_State* L) {
	luaL_newlib(L, surface_filters);

	return 1;
}

