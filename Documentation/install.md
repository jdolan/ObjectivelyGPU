Installing ObjectivelyGPU {#install}
=========================

Dependencies, building, and linking against ObjectivelyGPU.

[TOC]

## Releases

Tagged releases are published on the [GitHub releases page](https://github.com/jdolan/ObjectivelyGPU/releases). To build the latest from source, follow the steps below.

## Dependencies

* [Objectively](https://github.com/jdolan/Objectively) >= 2.0.0
* [SDL3](https://github.com/libsdl-org/SDL) >= 3.2.0

## Building

```sh
autoreconf -i
./configure
make && sudo make install
```

## Linking

Compile and link against ObjectivelyGPU with `pkg-config`:

```sh
gcc `pkg-config --cflags --libs ObjectivelyGPU` -o myprogram *.c
```
