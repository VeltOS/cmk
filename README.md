Cmk
------

Cmk is a Widget Toolit, and ONLY a Widget Toolkit. It is not have a display
backend, and it has no concept of a window or a stage. It only has minimal
layout capabilities. It is designed for use inside a display backend, such
as GTK+, Clutter, Cocoa, or whatever Windows uses.

Why is this useful? If, for example, you want to create a multi-platform app
written in the default toolkit of each platform so it doesn't look out-of-
place, but you also have a custom-rendered part of the app which displays
some kind of (potentially user-interactive) data. Normally, you might have
to write the event handling and rendering for that widget for each platform.
With Cmk, you can write it once as a CmkWidget subclass, and embed it in any
toolkit.

Cmk was created for the [Graphene Linux Desktop Environment](https://github.com/VeltOs/graphene-desktop),
so that the custom-rendered Material Design Widgets (initially written in
Clutter for use with Mutter) could also be used in desktop applications
shipped with Graphene (written in GTK+ because Clutter has very bad desktop
app support). Hence, Cmk comes with a default set of Material Design-style
widgets.

TODO
-----

	- Support macOS Cocoa

Building
-----

Depends on:

	- cmake (build only)
	- cairo
	- glib
	- pango
	- gtk3 (optional; for building with GTK+ support)
	- clutter-1.0 (optional; for building with Clutter support)

To build, first edit the first few lines of the CMakeLists.txt file to
specify whether or not to build with GTK+ and/or Clutter support. Then:

```bash

    cd path/to/libcmk
    mkdir build && cd build
    cmake .. && make
    sudo make install # Optionally install
```

License
-----

Cmk is under the Apache License 2.0. This means you can freely compile and
and use it in binary form (including static linkage) without restriction, but
derivatives must retain attribution to the original authors. For full details,
visit https://www.apache.org/licenses/LICENSE-2.0 .

Authors
-----

Aidan Shafran <zelbrium@gmail.com>

