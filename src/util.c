#include <math.h>

#include <glib.h>

#include "util.h"

gdouble clamp(const gdouble number, const gdouble floor, const gdouble ceiling) {
	if (number < floor) {
		return floor;
	} else if (number > ceiling) {
		return ceiling;
	} else {
		return number;
	}
}

gint64 int_clamp(const gint64 number, const gint64 floor, const gint64 ceiling) {
	if (number < floor) {
		return floor;
	} else if (number > ceiling) {
		return ceiling;
	} else {
		return number;
	}
}

static const gdouble pi = 3.1415926535897932384626433832795;
// Based on https://stackoverflow.com/a/8204867
gdouble *generate_blur_kernel(const guint radius, const gint sigma) {
	gdouble *kernel = malloc(sizeof(gdouble) * (radius * radius));
	gdouble mean = radius / 2;
	gdouble sum = 0.0;

	FOR_RANGE (x, radius) {
		FOR_RANGE (y, radius) {
			gint index = x + (y * radius);

			kernel[index] = exp(
				-0.5 * (pow((x - mean) / sigma, 2.0) + pow((y - mean) / sigma, 2.0))
			) / (
				2 * pi * sigma * sigma
			);

			sum += kernel[index];
		}
	}

	FOR_RANGE (x, radius) {
		FOR_RANGE (y, radius) {
			kernel[x + (y * radius)] /= sum;
		}
	}

	return kernel;
}

gdouble *generate_blur_kernel_linear(const guint radius, const gint sigma) {
	gdouble *kernel = g_malloc(sizeof(gdouble) * (radius));
	gdouble mean = radius / 2;
	gdouble sum = 0.0;

	FOR_RANGE (x, radius) {
		kernel[x] = exp(
			-0.5 * (pow((x - mean) / sigma, 2.0) + pow(((radius/2) - mean) / sigma, 2.0))
		) / (
			2 * pi * sigma * sigma
		);

		sum += kernel[x];
	}

	FOR_RANGE (x, radius) {
		kernel[x] /= sum;
	}

	return kernel;
}

RGBAPixel rgba_pixel_new(void) {
	RGBAPixel obj = {
		.red   = 0,
		.green = 0,
		.blue  = 0,
		.alpha = 0,
	};

	return obj;
}

ARGBPixel argb_pixel_new(void) {
	ARGBPixel obj = {
		.alpha = 0,
		.red   = 0,
		.green = 0,
		.blue  = 0,
	};

	return obj;
}
