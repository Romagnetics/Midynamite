# Romagnetics Midynamite  
*The Open Source MIDI Multi-Effects Pedal*

![alt text](https://github.com/RomainDereu/Midynamite/blob/main/02_Photos/Midynamite.jpg "Romagnetics Midynamite")

The Romagnetics Midynamite is a compact, battery-powered MIDI multi-effects pedal with three physical MIDI DIN 5-pin connectors and USB MIDI device support.  
It can function as a standalone pedal or as a USB MIDI interface for a computer.

All effects can be toggled independently and configured via the onboard screen and encoder.

List of effects as of version 2.0

The detailed manual for the pedal can be consulted here: https://www.romagnetics.com/wiki/doku.php?id=manuals:midynamite:start
Available to buy here: https://www.romagnetics.com/product/romagnetics-midynamite/

---

## 🎛 Features Overview

---

### MIDI Tempo

![MIDI_Tempo](https://www.romagnetics.com/wiki/lib/exe/fetch.php?media=manuals:midynamite:midi_tempo_1.png "MIDI Tempo")

 Send a MIDI clock signal to any OUT you want. 
 [More information about MIDI Tempo. ](https://www.romagnetics.com/wiki/doku.php?id=manuals:midynamite:menus:midi_tempo)

---

###  MIDI Split

![MIDI_Split](https://www.romagnetics.com/wiki/lib/exe/fetch.php?media=manuals:midynamite:midi_split_1.png "MIDI Split")


 MIDI Split allows you to send MIDI notes to two different groups based on the note, the MIDI channel, or the velocity. 
 You can sen send each group either to all the effects, to dry, or to Bus 1 or Bus 2.
 
![MIDI_Split_bus](https://www.romagnetics.com/wiki/lib/exe/fetch.php?media=manuals:midynamite:midi_split_bus.png "Midi Split Bus")

 [More information about MIDI Split. ](https://www.romagnetics.com/wiki/doku.php?id=manuals:midynamite:menus:midi_split)


More info about MIDI Split 


###  MIDI Modify

![MIDI_Modify](https://www.romagnetics.com/wiki/lib/exe/fetch.php?media=manuals:midynamite:midi_modify_1.png " MIDI Modify")

Modify the MIDI Channel and the velocity of incoming MIDI messages.


---

### 🎼 MIDI Transpose

![MIDI_Transpose](https://www.romagnetics.com/wiki/lib/exe/fetch.php?media=manuals:midynamite:midi_transpose_1.png " MIDI Transpose")


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

## Major Version History
August 2025: Version 1.0
- MIDI 

March 2026
