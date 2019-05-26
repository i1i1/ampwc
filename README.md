
ampwc
=====

The repository contains various pieces for source code for
simple tiling manager, which will never be born.

Running
-------

    $ make
    $ WAYLAND_DEBUG=1 ./src/wlserv
    $ WAYLAND_DISPLAY=wayland-0 WAYLAND_DEBUG=1 ./src/wlclient


Dependencies
------------

* Wayland libs
* libdrm
* libudev
