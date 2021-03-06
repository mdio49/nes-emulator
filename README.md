# NES Emulator
An NES emulator written in C, developed for educational purposes. Code is written for clarity and understanding rather than performance and cycle-perfect accuracy. It is by no means perfect nor 100% faithful to the original hardware, but it is nonetheless capable of running a decent set of games.

## Compilation Instructions
The emulator must be compiled from source, which requires `gcc` and `make` to be available on your platform. Has only been tested on a Windows 10 PC but it should work on other platforms.
1. Download the repository into a new folder.
2. Download the latest version of [SDL2](https://www.libsdl.org/) and place the SDL2.dll in the root of the working directory.
3. Open a terminal in the same directory as the emulator and run the `make` command.
4. If the build succeeded, then the output should be `emu`, potentially with a file extension that depends on your operating system.

## Usage Instructions
Once you have a copy of the emulator, you can run it through the terminal by providing a ROM as the first command line argument, (i.e. `./emu <ROM>`). The emulator provides support for as many ROMs up to as many mappers as it has implemented. If a particular mapper is not supported, then a message will be printed in the terminal and the emulator will not attempt to execute the ROM.

There are other flags that may be included:
- `-l`: Logs all CPU instructions to a file named `emu.log`. Will significantly slow down the emulator. Useful only for debugging purposes.
- `-t`: Runs the emulator in test mode, printing output to the terminal based on memory at $6004 in accordance with the standard tests. The emulator will automatically halt once the test is complete (i.e. it has a status at $6000 that isn't $80 or $81).
- `-x`: Runs the specific sequence of bytes given after the argument rather than executing a binary file. Does not produce a GUI and will instead print each instruction exeucted and halt once the `BRK` instruction is called. Only interacts with the 6502 CPU implementation and is only useful for very primitive testing.

During emulation, the emulator accepts the following keys as controller input for P1:
- `D-Pad`: Arrow Keys.
- `Start`: Enter.
- `Select`: Right-Shift.
- `A`: Spacebar.
- `B`: Left-Control.

The following hotkeys may also be used to control the emulator:
- `R`: Reset.
- `P`: Pause/resume.
- `F4`: Toggle fullscreen.
- `M`: Toggle audio.
- `L`: Start/stop logger.
- `Esc`: Exit emulator.

## Possible Future Plans
- Add more mappers.
- Add P2 controller support.
- Improve cycle accuracy.
- Implement PAL and Dendy support.
- Implement support for the NES 2.0 file format.
- Use harmonics for better quality audio.

## References
1. NESdev Wiki, https://www.nesdev.org/wiki/Nesdev_Wiki
2. 6502 Instruction Set, https://www.masswerk.at/6502/6502_instruction_set.html
3. Easy 6502 (for playing around and learning 6502), https://skilldrick.github.io/easy6502/
4. Collection of test ROMs, https://github.com/christopherpow/nes-test-roms
