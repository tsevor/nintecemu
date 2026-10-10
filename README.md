# nintencemu

## why

i have made two other nes emulators before.
i'm making another to get myself more familiar
with object-oriented concepts in c.

i hate c++ but i like classes. the extent to
which that i like classes is limited to them
being structs with implicit functions, so i
want to practice using structs instead.

i'm doing an nes emulator again because it is
something with which i am familiar, and to
prepare me for my next big project:

a snes emulator

## use

i have made zero progress so ui/args is in the
future and how it will work is yet undecided

probably this for command line:

`./nintecemu rom.nes`

## build

requirements
- sdl3

then it should just be

`make`

## todo

todo:
- unofficial opcodes
- more logging everywhere
- video output
- ppu
- loopy registers maybe (i might do that from
  the start instead of patching it in)
- mappers
	- mmc3
- apu
- good ui (first one will be a bad one)
