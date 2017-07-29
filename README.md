libcmk
=========

A material-design widget toolkit based on Clutter.

Currently libcmk builds a custom version of Clutter, but only
for a few changes to fix backend-specific issues. It is possible
to build libcmk without the custom Clutter and link it with a
regular shared library.

Building
---------

libcmk requires basically everything Clutter requires and CMake

    - cmake (build only)
	- autotools (build only)
	- atk >= 2.5.3
	- cairo-gobject >= 1.14.0
	- cogl-1.0 >= 1.21.2
	- cogl-pango-1.0
	- cogl-path-1.0
	- gdk-3.0
	- gdk-pixbuf-2.0
	- gio-2.0 >= 2.44.0
	- json-glib-1.0 >= 0.12.0
	- libinput >= 0.19.0
	- librsvg-2.0
	- libudev >= 136
	- pangocairo >= 1.30
	- pangoft2
	- wayland-client
	- wayland-cursor
	- x11
	- xcomposite >= 0.4
	- xdamage
	- xext
	- xkbcommon
	- xi

Download/clone this repo and run

```bash

    cd cmk 
    cmake .
    sudo make install
```

GtkDoc documentation is available if you have the gtk-doc package
installed and you run 'make documentation'. Open the cmkdoc/html/index.html
file in a browser.

Wayland
--------

Clutter currently has a bad Wayland backend, and does not support
window decorations, user resizing, or correct window scaling. Untill
this can be fixed, to use Cmk (or Clutter in general) for client
applications, I recommend forcing Clutter to use the Xorg backend
by calling clutter_set_windowing_backend("x11"); at the top of main.

Todo
--------

Basically everything

Authors
--------

Aidan Shafran <zelbrium@gmail.com>
