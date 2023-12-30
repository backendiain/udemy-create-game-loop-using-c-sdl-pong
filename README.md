## PONG
This is an implementation of the classic game Pong using C.

It was originally written as a solution to Gustavo Pezzi's [Create A Game Loop Using C and SDL](https://www.udemy.com/course/game-loop-c-sdl/) Udemy course and [here](https://www.youtube.com/watch?v=XfZ6WrV5Z7Y).

After a few commits, it's not as bare bones as it once was. I wanted to try a few things like [Casey Muratori's](https://github.com/cmuratori) [Handmade Hero](https://handmadehero.org/) input flow in this context.

If it helps someone else tinkering in SDL then great, if not, have fun!

### Controls
I've set this up so that it can be played by two people on the one keyboard.

* `ESC` - Will close the game
* `spacebar` - Will start your game from the title screen
* `F1` - Will reset the game to the title screen at any time
* `w` - Moves the left paddle up
* `s` - Moves the left paddle down
* `up` - Moves the right paddle up
* `down` - Moves the right paddle down

The winner is the first to score 10 points because double digits is extremely taxing on modern hardware.

### Downloading
Check out the [Releases](https://github.com/backendiain/udemy-create-game-loop-using-c-sdl-pong/releases) tab and just download whatever is latest.

I highly doubt there's going to be many people looking at this so I'll just stick up the Windows builds.

### Build
I built this with [SDL2](https://github.com/libsdl-org/SDL) on Windows using the MVSC compiler and Visual Studio.

Gustavo has a good video on that here:
https://www.youtube.com/watch?v=tmGBhM8AEj8

I don't see any reason why compiling and linking with gcc wouldn't work if you've got SDL2 on your distro.

Instructions on how to set that up here:
* https://www.youtube.com/watch?v=XfZ6WrV5Z7Y&t=562s (Linux)
* https://www.youtube.com/watch?v=XfZ6WrV5Z7Y&t=693s (Mac)
* https://wiki.libsdl.org/SDL2/Installation

If you use WSL (Windows Subsystem for Linux) then the Linux install steps should work for you too.

This'll probably work if you've set things up properly on Linux/Mac:
* `gcc src/main.c "sdl2-config --cflags --libs" -o pong`

NOTE: You *might* need to tweak the paths to the image if you're not using Visual Studio or its executable directory is configured differently. I'll tweak the code if it's really an issue.

### Assets
What little graphical assets there is are under the `assets` directory.

The [GIMP](https://www.gimp.org/) .xcf files are in there too, if you want to tweak the source.

### Acknowledgements
I knew movement vectors were the secret sauce for ball movement but wasn't sure how to implement it cleanly.

[flightcrank](https://github.com/flightcrank/pong/tree/master) was kind enough to share their code on github which helped me. Give them a star.

Also special thanks to:
* [Gustavo Pezzi](https://github.com/gustavopezzi) who got a backend dev like me into fun game dev stuff a few years ago with his great build a game for the Atari VCS series on Udemy a few years ago.
* [Casey Muratori](https://github.com/cmuratori) whose Handmade Hero tutorial series has proved fun and invaluable. SDL's API is pretty simple but building an engine along with Casey made it all feel familiar and something I actually understood. I adapted his input flow pattern here for Pong.

Both these guys are worth your time, effort and money.

#### Links
* [Gustavo Pezzi on Udemy](https://www.udemy.com/user/gustavopezzi/)
* [Gustavo Pezzi's Pikuma.com](https://pikuma.com/)
* [Handmade Hero](https://handmadehero.org/)
* [Handmade Hero Playlist](https://www.youtube.com/playlist?list=PLnuhp3Xd9PYTt6svyQPyRO_AAuMWGxPzU).
* [SDL repo](https://github.com/libsdl-org/SDL)