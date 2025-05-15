# Mandelbrot

A cpu-bound multi-threaded renderer for the Mandelbrot set written in C++17. Serves no purpose besides looking cool. For an explanation of the Mandelbrot set, have a look at this https://en.wikipedia.org/wiki/Mandelbrot_set.

Below you see an example image rendered in 1920x1080 (left) and a low-quality zoom animation rendered in 800x450 (right). For higher quality examples, have a look in /examples.

<p align="center">
  <img src="examples/.example.png" width="48%" />
  <img src="examples/.exampleGif.gif" width="48%" />
</p>

# Dependencies

Requires SFML library. On Ubuntu, install it via:

    sudo apt-get install libsfml-dev

For anything else, the installation paths in the Makefile will probably need to be adjusted.

# Run

Compile the binary by running the Makefile:

    make

and then run the executable:

    ./bin/Mandelbrot

Resolution of rendering (not drawing) is 1280x720 normally. Different resolutions can be given as optional arguments:

    ./bin/Mandelbrot -r 1920 1080

## Controls

- Zoom into the location of the mouse cursor by pressing LMB.
