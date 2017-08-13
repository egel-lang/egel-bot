# egel-bot

A quick-and-dirty Egel IRC bot to test and showcase static linking
of the Egel runtime.

## Compiling

This is a Linux application which needs the Egel interpreter sources,
make sure they are installed and compiled.

+ Give the `make archive` command in the Egel interpreter `src` directory to
  create a static library.
+ Go to the `src` directory of the Egel IRC bot distribution.
+ Run `make`. Make sure the Egel interpreter is in a directory named `egel`
  next to Egel bot or adjust the Makefile.

## Running the bot

Type the command `egel-bot` for usage. Type `egel-bot node service channel nick` to
fire up the bot. A script file is provided which you can adopt to your need.

Note that it is trivial to pass an exploding computation to the interpreter.
That's on the TODO list.
