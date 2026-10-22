# nintecemu

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

i have made zero progress on ui so use args:

`./nintecemu rom.nes`

## build

requirements for pc host
- gtk
- libadwaita

then run:

`make`

or use

`make run ARGS="path/to/rom.nes"`

## todo

todo:
- more logging everywhere
- mappers
	- mmc3
- apu
- good ui (first one will be a bad one)
- get it running on a calculator
	- i will segment host.c entirely so it
	  is the only thing interacting with the
	  device io (screen, controller state, etc)
