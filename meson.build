project(
	'surface_filters',
	'c',
	version: '0.1',
	default_options: [
		'warning_level=3',
	],
)

cc = meson.get_compiler('c')

dependencies = [
	cc.find_library('m', required: true),
	dependency('glib-2.0'),
	dependency('gtk+-3.0'),
	dependency('gdk-3.0'),
	dependency('cairo'),
	# Change this to 'lua5.X' if you want to target another version
	dependency('luajit'),
]

shared_library(
	'surface_filters',
	['src/surface_filters.c', 'src/util.c'],
	name_prefix: '',
	install: true,
	dependencies: dependencies,
)
