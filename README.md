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

requirements for gtk host
- gtk
- libadwaita

then run:

`make`

or use

`make run ARGS="path/to/rom.nes"`

## todo

todo:
- more logging everywhere
- more mappers
	- mmc3
- apu
- hosts
  - gtk host
    - add gui for cool features
    - maybe debugger?
    - at least some more options like that
  - sdl3 host
    - simple and fast
    - low on features
  - calculator host??
    - ti nspire cx ii cas
