# Interp
Interpolates a WAV file doubly using fixed-point linear interpolation.

### Usage:
```
./interp (options) (input wave 1) [(...) (input wav n)]

Options:
  -o : Output file or output directory depending on the input.
```

### Building:
```
gcc -O2 -s -o int24.o -c int24.c
gcc -O2 -s -o interp.o -c interp.c
gcc -o interp interp.o int24.o
```

### Releases Notes:

  * Added command line support for multiple WAV input files.
  * Added support for PCM 24 / 32-bits, IEEE Float 32 / 64-bits.
  * Added support for unlimited number of channels (the maximum WAV files allows it).
