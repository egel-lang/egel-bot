# egel-bot

A quick-and-dirty Egel IRC bot to test and showcase dynamic linking
of the Egel runtime.

## Compiling

This is a Linux application which needs the Egel interpreter libaries,
make sure they are compiled and installed.

+ Egel must be installed system wide.
+ Do a default cmake build.

## Running the bot

Type the command `egel-bot` for usage. Type `egel-bot node service channel nick` to
fire up the bot. A script file is provided which you can adopt to your need.

Note that it is trivial to pass an exploding computation to the interpreter.
That's on the TODO list.
