libcmk
=========

A material-design widget toolkit based on Clutter.


Building
---------

libcmk requires:

    - cmake (build only)
    - clutter
    - librsvg

Download/clone this repo and run

```bash

    cd cmk 
    cmake .
    sudo make install
```

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
