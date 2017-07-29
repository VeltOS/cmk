libcmk
=========

A material-design widget toolkit based on Clutter.

Currently libcmk builds a custom version of Clutter, but only
for a few changes to fix backend-specific issues. It is possible
to build libcmk without the custom Clutter and link it with a
regular shared library.

Building
---------

libcmk requires mostly sub-packages of these packages for building
Clutter. See Clutter's git repo for more info on exact dependencies.

    - cmake (build only)
    - gtk3
    - cogl
    - libinput

Download/clone this repo and run

```bash

    cd cmk 
    cmake .
    git submodule update --init --depth=1
    sudo make install
```

GtkDoc documentation is available if you have the gtk-doc package
installed and you run 'make documentation'. Open the cmkdoc/html/index.html
file in a browser.

Wayland
--------

Clutter currently has a bad Wayland backend, and does not support
window decorations, user resizing, or correct window scaling. Until
this can be fixed, to use Cmk (or Clutter in general) for client
applications, I recommend forcing Clutter to use the Xorg backend
by calling clutter_set_windowing_backend("x11"); at the top of main
or use cmk_init() which does this for you.

Todo
--------

Basically everything

Authors
--------

Aidan Shafran <zelbrium@gmail.com>
