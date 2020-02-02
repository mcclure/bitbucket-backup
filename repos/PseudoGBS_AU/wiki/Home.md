# Pseudo GBS

This is a set of AudioUnits that mimic the "Game Boy Sound System", the sound chip from the original Game Boy. I made them to help [Christine Love](http://loveconquersallgam.es/) with a game boy homebrew project. Christine's goal was to be able to compose music in a traditional DAW and hear roughly what the music would sound like after she converted it to Game Boy assembly. The goal was not exact precision, and no code from a conventional emulator was used (although the exact algorithms from the GBS as described in the documentation are in use).

Included are four AudioUnits and the source for each.

## Download

You can download the AudioUnits from [here](https://data.runhello.com/j/au/gbs/1/PseudoGBS_AU.Release1.d890a800892a.zip), or if you have a BitBucket account, from the "Downloads" tab on the left.

## Usage

There are four AudioUnits: "Channel 3", "Channel 3 Drumkit", "Channel 4" and "Channel 4 Drumkit". These mimic the GBS PCM channel (3) and noise channel (4). The square wave channels (1 and 2) can easily be simulated by setting the appropriate square wave as the soundclip.

To install the plugins, copy them to `~/Library/Audio/Plug-Ins/Components/`. Restart your DAW and the AudioUnits will appear under the manufacturer "Meadow".

In our testing, we found that both the Game Boy hardware, and all emulators we tested, apply a slight low pass filter to all sound. These AudioUnits do not attempt this, so by themselves the AudioUnits sound slightly harsher than the real thing. In order to get authentic sound, when using the AudioUnits we applied the "EQ Three" filter from Ableton Live Lite with a FreqHi of 2.50 kHz and a GainHi of -6 or -7 db.

### Channel 3

GBS channel 3, the "PCM channel", allows you to select a soundclip of up to 32 4-bit PCM samples and play it in a loop with a specified speed and volume envelope.

The AudioUnit has standard attack and release sliders; their units are the duration of the attack or release period. The AudioUnit also allows you to select the amount of polyphony (ie the number of simultaneous notes playing). For realism, you should set this number no higher than 3 for a "square wave" type soundclip, or no higher than 1 for a soundclip the square wave couldn't produce. The soundclip is input by 32 sliders taking 4-bit integers, and a "samples" slider for selecting how many of them should be used.

The AudioUnit does a "frequency adjustment" where the soundclip is sped up or slowed down to produce a pitch that matches the actual keyboard note regardless of soundclip length. On a real Game Boy, the longer the soundclip, the lower its effective pitch would be during playback.

#### Channel 3 Presets

Here are some suggested settings for "Channel 3". All of these are available as AudioUnit factory presets, if your DAW supports them (Ableton doesn't...).

* **Square Wave**: Same as square channel with NR11 bits 6-7 set to 10. **Values:** 0.005, 0.1, 3, 2, 0, 15
* **12.5% duty**: Same as square channel with NR11 bits 6-7 set to 00. **Values:** 0.005, 0.1, 3, 8, 0, 15, 15, 15, 15, 15, 15, 15
* **25% duty**: Same as square channel with NR11 bits 6-7 set to 01. **Values:** 0.005, 0.1, 3, 8, 0, 0, 15, 15, 15, 15, 15, 15
* **75% duty**: Same as square channel with NR11 bits 6-7 set to 11. **Values:** 0.005, 0.1, 3, 8, 0, 0, 0, 0, 0, 0, 15, 15
* **Sawtooth**: A 6-sample soundclip of a sawtooth wave. **Values:** 0.005, 0.1, 3, 6, 0, 3, 6, 9, 12, 15
* **Triangle**: An 8-sample soundclip of a triangle wave. **Values:** 0.1, 0.1, 3, 8, 0, 3, 7, 11, 15, 12, 8, 4
* **Weird Harp**: ??? I like it. **Values:** 0.05, 0.1, 3, 8, 0, 2, 4, 6, 8, 0, 0, 15

### Channel 3 Drumkit

The "drumkit" variations of the AudioUnits ignore the note duration and frequency from your MIDI keyboard and instead use a fixed values set by the "hold" and "frequency" sliders. This is useful if you are using the drumpad feature in Ableton.

The Channel 3 Drumkit also has a few extra checkbox options. "Const Velocity" ignores the note velocity also and sets it to a constant 127. "Freq Adjust" allows you to turn off the frequency adjustment mentioned above. (Effectively, with the frequency adjustment on, the AudioUnit mimics the frequency range of a square wave channel; with adjustment off, it mimics the range of the PCM channel. I haven't tested this part exactly.) "Minimum 1 loop" forces the sample to play at least once, even if Attack, Hold and Release are all set to 0.

I don't think the Channel 3 Drumkit is very useful; I couldn't get channel 3 to make any good drum noises.

### Channel 4

GBS Channel 4, the "noise channel", uses an LSFR random number generator to produce white noise. Two registers alter how often the random number generator changes values, allowing you to effectively set the noise "pitch".

In addition to the attack and release sliders, the AudioUnit has two checkboxes. When "Snap" is selected, the AudioUnit will only play those pitches the noise channel is actually capable of producing. When "Bit 3" is selected, the LSFR algorithm will change slightly (in the same way as happens when you set bit 3 on the NR42 register) and the output will sound less like noise and more like kind of a metallic scraping sound.

If you want to know how to turn note frequencies into NR43 register values, run `channel4_helper_script.py` (provided by @eggboycolor from Twitter).

I think the Channel 4 Drumkit is probably a little more useful than the plain version.

### Channel 4 Drumkit

Like with the Channel 3 Drumkit, this is the Channel 4 audiounit but with fixed duration and frequency. It has the same hold, frequency and "const velocity" settings the Channel 3 Drumkit has.

The frequency values are given as strings like "r4s7". This refers to the register values of "r" (bits 0-2 of NR43) and "s" (bits 4-7 of NR43).

#### Channel 4 Drumkit Presets

* **Fire laser**: Some kind of space sound effect. **Values:** 0, 0.1, 0.1, r6s5
* **Tsss**: Sounds like a high hat. **Values:** 0, 0, 0.067, r4s1
* **Chunk**: Sounds like a kick drum. **Values:** 0, 0.07, 0, r4s7
* **Snare**: Sounds like a snare. **Values:** 0, 0, 0.067, r6s4
* **Engine**: Clearly the sound of a spaceship engine. **Values:** 0, 0.75, 0.067, r4s7, [Bit 3]

## Author/License

These were written by Andi McClure <<andi.m.mcclure@gmail.com>>. Big thanks to the following people:

* Christine, for giving me the idea as well as technical information and testing
* The authors of [this document](http://bgb.bircd.org/pandocs.htm#soundoverview) and [this document](http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware)
* @eggboycolor on Twitter, for the `channel4_helper_script.py` script
* TJ Usiyan for help with setting up AudioUnit development.

The contents of this directory, including the source code for the four AudioUnits (and not counting `channel4_helper_script.py`) are made available to you under the following license:

> Copyright (C) 2016 Andi McClure
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

If this license is too restrictive for some reason, feel free to contact me about other arrangements.