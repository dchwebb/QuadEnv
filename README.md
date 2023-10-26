# QuadEnv/L4O

Overview
--------
![Image](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Graphics/l4o_side.jpg "icon")

QuadEnv is a four-voice polyphonic ADSR envelope generator designed for use with the Eurorack Modular Synthesiser architecture. L4O is a four-voice sine wave LFO and uses the same hardware as QuadEnv.

Different front panels are provided for each mode, while the firmware, component and controls boards are dual mode.

As part of a range of compatilble Mountjoy Modular modules, polyphonic interconnections are made using RJ-45 cables (as used for Ethernet).

A USB serial console is provided for customising the envelope times and fade-in rates of the LFOs.

The third potentiometer (Sustain or Fade-In) has a momentary button which allows switching between the envelope and LFO modes via a long press.

QuadEnv
-------
![Image](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Graphics/quadenv.jpg "icon")

The digital envelopes are modelled on a Roland System 100 system, with logarithmic/exponential attack/decay and release sections. The level of each envelope is displayed in a bank of LEDs at the top of the module.

A short press on the Sustain potentiometer button inverts the envelopes.

The USB serial console allows the envelope times to be scaled up or down for longer or shorter envelopes.

L4O
---
![Image](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Graphics/l4o.jpg "icon")

The four potentiometers control Speed, Spread, Fade-in and Level. Spread adjusts the variation in LFO speed from one voice to another. Fade-in allows the LFO level to be increased gradually. Short pressing the Fade-in button allows the speed to also be gradually increased. Fade-in starts when a gate is received at the input.

The USB serial interface allows the fade-in times of the level and speed to be independently adjusted.


Technical
---------

![Image](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Graphics/l4o_components.jpg "icon")

The module is based around an STM32G431 microcontroller which contains four internal 12 bit DACs used to generate the envelopes at approximately 47kHz.

A CD40109B level shifter provides input protection for the Gate In signals. A TL074 op-amp ampilifies the DAC outputs to around 8 volts at maximum.

Construction is a sandwich of three PCBs with a component board, a controls board and a panel. PCBs designed in Kicad v6.

[Components schematic v2](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Component_Schematic_v2.pdf)

[Controls schematic v2](https://raw.githubusercontent.com/dchwebb/QuadEnv/master/Control_Schematic.pdf)

### Power

The Eurorack +/-12V rails have reverse polarity protection and filtering. 3.3V rails are generated with a linear regulator.

- +12V Maximum Current Draw: 80mA


