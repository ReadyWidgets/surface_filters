#ifndef HEADER_surface_filters_util
#define HEADER_surface_filters_util

#include <glib.h>

#define FOR_RANGE(LOOPVAR, STOP) for (glong LOOPVAR = 0; LOOPVAR < (STOP); ++LOOPVAR)

#define DEF(NAME) static int NAME(lua_State* L)

#define ASSERT(VALUE, MESSAGE) if (!(VALUE)) { lua_pushstring((L), (MESSAGE)); lua_error(L); }

gdouble clamp(const gdouble number, const gdouble floor, const gdouble ceiling);
gint64 int_clamp(const gint64 number, const gint64 floor, const gint64 ceiling);

gdouble *generate_blur_kernel(const guint radius, const gint sigma);
gdouble *generate_blur_kernel_linear(const guint radius, const gint sigma);

typedef struct _RGBAPixel {
	guint8 red, green, blue, alpha;
} __attribute__((packed)) RGBAPixel;
RGBAPixel rgba_pixel_new(void);

typedef struct _ARGBPixel {
	guint8 alpha, red, green, blue;
} __attribute__((packed)) ARGBPixel;
ARGBPixel argb_pixel_new(void);

#endif