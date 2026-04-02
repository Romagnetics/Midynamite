# Romagnetics Midynamite  
*The Open Source MIDI Multi-Effects Pedal*

![alt text](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/Midynamite.jpg "Romagnetics Midynamite")

The Romagnetics Midynamite is a compact, battery-powered MIDI multi-effects pedal with three physical MIDI DIN 5-pin connectors and USB MIDI device support.  
It can function as a standalone pedal or as a USB MIDI interface for a computer.

All effects can be toggled independently and configured via the onboard screen and encoder.

Available to buy here: https://www.romagnetics.com/product/romagnetics-midynamite/

---

## 🎛 Features Overview

---

### 🎵 MIDI Tempo

![Romagnetics_Midynamite](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/MIDI_Tempo.png "Midynamite MIDI Tempo")

Send a MIDI clock signal to:

- MIDI OUT 1  
- MIDI OUT 2  
- Both  
- USB MIDI (optional via Settings)

**Adjustable parameters:**

- **Tempo** (BPM)  
- **Output destinations** (including USB)

---

### 🎚 MIDI Modify

![MIDI_Modify](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/MIDI_Modify.png " MIDI Modify")

Modify incoming MIDI messages in two main ways:

#### 🌀 Channel Mode

- **MIDI Change**  
  Redirect all incoming MIDI messages to one or two selected MIDI channels.
  This allows you to play two synths at the same time.
  Also allows very quick channel changes between scenes.

- **MIDI Split**  
  Split the keyboard into two zones:  
  - Notes below a split point go to one MIDI channel.  
  - Notes above go to another.  
  Ideal for controlling two synths with one keyboard.

> The split point and both output channels are fully configurable.

> The switch between MIDI Change and MIDI Split is made by pressing the select button when selecting a MIDI Change or MIDI Split item

> The OUTs are fully configurable between OUT 1, OUT 2, OUT 1 & 2 & Split.
---

#### 🎛 Velocity Mode

- **Velocity Change**  
  Add or subtract velocity from the incoming signal.  
  *(Range: -50 to +50)*  
  Preserves dynamic expression.

- **Fixed Velocity**  
  All incoming notes are set to a fixed velocity.  
  Useful for consistent volume or triggering.

  > The switch between Velocity Change and Fixed Velocity is made by pressing the select button when selecting a velocity item


---

### 🎼 MIDI Transpose

![MIDI_Transpose](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/MIDI_Transpose.png " MIDI Transpose")


Apply real-time note transformations to incoming MIDI notes.

#### 📉 Pitch Shift

- Shift all incoming notes up/down by a specified number of semitones.  
- Optionally **mute the original note**.

#### 🎶 Scale Mode (Harmonizer)

- Select a **scale**, **mode**, and **interval**.  
- The pedal generates a harmonized note based on the input.

> Optionally mute the original note only to hear the harmony.

> The switch between Pitch Shift and Scale Mode is made by pressing the select button in the Transpose menu

---

## ⚙️ Settings

![Midynamite Settings](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/Settings.png "Midynamite Settings")

Accessible in the **Settings** menu:
- **Start Menu**  
  Choose default screen on startup:  
  *Tempo*, *MIDI Modify*, *Transpose*, or *Settings*.

- **USB MIDI**  
  Configure USB MIDI behavior:  
  - *Send to USB*  
  - *No USB*

- **Contrast**  
  Adjust screen contrast in 10 levels (from 10% to 100%).

---

## 🆘 Panic Button

To immediately stop all MIDI output:

**Press Select + Value simultaneously**

This will send all notes off to all 16 MIDI channels.

---

