# Interp

Interpolates a WAV file doubly using fixed-point linear interpolation.

### Usage
```
./interp (options) (input wave 1) [(...) (input wav n)]

Options:
  -o : Output file or output directory depending on the input.
```

### Building

Building *interp* can be done using GNU Make:

```
make
```

### Install

To install *interp*, run the following target:

```
make install PREFIX=(prefix)
```

The variable `PREFIX` defaults to `/usr/local`.

### Uninstall

To uninstall *interp*, run the following target using the same prefix as specified in the install process:

```
make uninstall PREFIX=(prefix)
```

### Releases Notes

  * Added command line support for multiple WAV input files.
  * Added support for PCM 24 / 32-bits, IEEE Float 32 / 64-bits.
  * Added support for unlimited number of channels (the maximum WAV files allows it).
