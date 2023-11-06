#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1

function compile() {
	clear
	meson compile -C "$PWD/builddir"
}

compile

while inotifywait -e close_write -qq -r 'src/'; do
	compile
done
