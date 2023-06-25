# tdcaster

`tdcaster1` was in my head a "top-down" raycaster. I then tried to expand it to some more general 3D stuff with `tdcaster2` and `tdcaster3`.

![Picture of tdcaster4](/doc/example.png)

As of writing (I'm writing this a while after I made 1, 2, and 3), I haven't figured out how to compile `tdcaster3` again, but to run `tdcaster2`, you can:

1. `cd` into `tdcaster2`
2. `premake5 gmake` or whatever other premake generation you need.
3. `make config=release_linux`
4. look for the `tdcaster` in the generated `bin` folder under `release_linux` and run it

`tdcaster4` is more recently written, and written in Zig. Run `zig build run -Doptimize=ReleaseFast` on it. It depends on my [`zigpge`](https://github.com/regenerativep/zigpge) repository, and also on the [`zigimg`](https://github.com/zigimg/zigimg) repository. (I've last run this on Zig version `0.11.0-dev.3797+d82ab4fd8`. `zigpge` only runs on Linux+X11 at the time of writing as well.)
