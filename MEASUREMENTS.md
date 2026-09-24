# Enclosure Measurements

Fill in the **Measured** column with calipers. Everything is in **millimetres**.
The Reference column is a typical published figure — treat it as a sanity check,
not a substitute. Your parts will vary, and a 0.5 mm error here becomes a part
that doesn't fit.

Leave anything you can't measure blank rather than guessing; a blank tells me to
design clearance around it, a wrong number tells me to design a press fit.

> **Design assumption: the ESP32-CAM-MB programmer board is NOT inside the
> housing.** It plugs into every pin and would block all wiring. The microSD
> card _is_ installed and stays in. See "Wiring" at the bottom.

---

## What is still blocking the model

Everything else is measured. These three are what the housing is waiting on:

| Section | What | Blocks |
| --- | --- | --- |
| **6b** | 4-cell pack as assembled, including prong protrusion | Sled, housing width and height |
| **6e** | The dome PIR's real PCB size | Front plate aperture |
| **6d** | Mini breadboard confirmed size | Interior layout |

Sections 6c (fuse holder) and 7 (converters) are wanted soon but not blocking —
I can leave generous pockets for both.

---

## 1. ESP32-CAM board

| #   | Dimension                                | Reference | Measured | Notes                                              |
| --- | ---------------------------------------- | --------- | -------- | -------------------------------------------------- |
| 1.1 | PCB length                               | ~40.5     | 40.35mm  | Long edge, antenna end to camera end               |
| 1.2 | PCB width                                | ~27.0     | 27.25mm  |                                                    |
| 1.3 | PCB thickness (bare)                     | ~1.6      | 1.25mm   | Just the board                                     |
| 1.4 | **Total thickness incl. microSD socket** | ~5.5      | 6.70mm | Underside socket included — drives standoff height. Not including pins or camera |
| 1.5 | Socket overhang past PCB edge            |           | 0.0mm | Does the socket stick out past the board? The antenna side hangs off a negligible ammount, but ive included this overhang in the PCB length. It might be 0.05mm |
| 1.6 | Corner mounting hole diameter            | ~2.0      | n/a, no mounting holes | If your board has them                             |
| 1.7 | Mounting hole spacing (L × W)            |           | n/a | Centre-to-centre                                   |

## 2. Camera module

| #   | Dimension                       | Reference  | Measured | Notes                                      |
| --- | ------------------------------- | ---------- | -------- | ------------------------------------------ |
| 2.1 | Lens barrel outer diameter      | ~9.0       | 7.92mm | The threaded black barrel                  |
| 2.2 | Barrel height above PCB face    | ~10.0      | 8.9mm | **Critical** — sets front plate standoff   |
| 2.3 | Camera body (square base) L × W | ~8.5 × 8.5 | 8.12mm x 8.12mm | The plastic housing under the barrel       |
| 2.4 | Ribbon cable width              |            | n/a | The ribbon is extremely short, the camera is basically laying on top of the micro SD card module                                          |
| 2.5 | Ribbon slack needed             |            | n/a | How far the camera sits from its connector |
| 2.6 | Camera W x H                    |            | 8.15mm x 8.25 | The base is square |
| 2.7 | Camera cylinder base diameter   |            | 7.92mm | The camera turn cylinder after square base |
| 2.8 | Full ESP32 height w/ camera     |            | 12.35mm | Full height, without pins |
| 2.9 | Full ESP32 height w/ pins and camera |       | 17.61mm | full height inlucing everything |

Note: The camera module lays flat on the micro SD card slot. The camera base is square, then goes up 2.20mm, then turns into a cylinder base, then goes up ~4.5mm. The camera technically gets more narror toward the top, but I think it's neglible and shouldnt go into our design plans. As long as we git the camera base properly, the camera should stick out slightly from the housing.

## 3. Antenna keep-out — measure carefully

The PCB trace antenna is the exposed copper meander at the board edge opposite
the camera. This zone drives the whole layout, so I need its real extent.

| #   | Dimension                                      | Measured | Notes                    |
| --- | ---------------------------------------------- | -------- | ------------------------ |
| 3.1 | Trace antenna length along board edge          | 17.82mm | The copper zigzag region |
| 3.2 | How far it extends inward from the edge        | 7.11m | This part is rectangular though, and the part where the antenna plugs into is further in, 10.5mm |
| 3.3 | Distance from antenna to nearest mounting hole | n/a | no mounting holes |

## 4. Motion sensor — HLK-LD1020

| #   | Dimension                          | Measured | Notes                   |
| --- | ---------------------------------- | -------- | ----------------------- |
| 4.1 | PCB length × width                 | 15.15mm x 15.15mm | square module |
| 4.2 | PCB thickness                      | 1.34mm | 2.46mm is full thickness with the tallest components |
| 4.3 | Tallest component, front face      | 1.13 | The side that faces out |
| 4.4 | Tallest component, back face       | 0 | neglible |
| 4.5 | Mounting holes: diameter + spacing | n/a | If present              |
| 4.6 | Pin header location and pin count  | 3 pins, on front |                         |

## 5. Fallback sensor — HC-SR501 ("HW-740")

Only needed if we print the PIR front plate. Skip for now unless convenient.

| #   | Dimension                           | Measured | Notes |
| --- | ----------------------------------- | -------- | ----- |
| 5.1 | White dome outer diameter           | 12.2mm | So the dome is dome shaped, but at the very base there is a lip that looks like it would be perfect to allow the sensorto be pressed against the outside and allow the lip to lay flush with the inside of the housing. The lips diameter is 13.42mm |
| 5.2 | Dome height above PCB               | 11.15mm | The dome comes out the top, and the pins are bent. The pins start going vertical from the face, then are bent 90 degrees outward, away from the dome. The end product is a flat module. |
| 5.3 | PCB length × width                  | 8.16mm x 10.34mm |       |
| 5.4 | Total thickness incl. pots and dome | 13.43mm | The dome is so thick its basically the whole thickness. If you measure the full length, with bent pins, PCB, and dome, it's 25.20mm |

## 6. Battery holder — 3-cell (superseded, kept for reference)

| #   | Dimension                          | Measured | Notes                                 |
| --- | ---------------------------------- | -------- | ------------------------------------- |
| 6.1 | Holder outer length                | 76.05mm |                                       |
| 6.2 | Holder outer width                 | 60.67mm |                                       |
| 6.3 | Holder outer height                | 20.80mm |                                       |
| 6.4 | Number of cells it holds           | 3 | I do have other holders, 2, 3, 4. It just changes the dimensions |
| 6.5 | Wire exit position                 | 15.45mm and 22.25mm | With the opening facing us, and the wire output at the bottom, I measured from the left corner. The red output hole is the first number, the second is black output |
| 6.6 | Wire gauge / length                | not sure, but its thin, about the same size as my dupont wires. 123.3mm length, might be variable per housing |                                       |
| 6.7 | Are cells side-by-side or stacked? | sie by side |                                       |

## 6b. Battery holder — 4-cell PARALLEL (the one being built around)

⚠️ **These drive the sled and the housing width. Everything else in the model is
already sized; this is the blocking measurement.**

Measure the pack **as assembled**, with your soldered links and wires in place —
not the bare holder. The prongs and wiring are part of the envelope now.

| #    | Dimension | Measured | Notes |
| ---- | --------- | -------- | ----- |
| 6b.1 | Holder outer length (along the cells) | 78.50mm | |
| 6b.2 | Holder outer width (across the cells) | 79.00mm | |
| 6b.3 | Holder outer height (body only) | 20.90mm | Excluding prongs |
| 6b.4 | **Prong protrusion below the body** | 4.80mm | How far the metal tabs stand proud of the underside |
| 6b.5 | **Prong footprint** | along each top and bottom, wire between each prong | Which area of the underside they occupy — whole face, or just the two ends? |
| 6b.6 | Height over the highest solder joint | 4.80mm | Including heat shrink once applied |
| 6b.7 | Wire exit position and direction | havent solders this part yet, so let me know which orientation is better and ill do that, positive is all on one side and vice versa | Which face, how far from a corner |
| 6b.8 | Free wire length you want to keep | any, whatever you decide | Determines slack space in the sled |

**Insulation is not optional.** A short between the pack's own terminals is
*upstream* of the fuse, so the fuse cannot protect it. Heat shrink every joint,
tape over the prong area, and the sled floor will carry a recess so nothing
metal can reach them. 6b.4 and 6b.5 size that recess.

## 6c. Fuse holder (assembled)

| #    | Dimension | Measured | Notes |
| ---- | --------- | -------- | ----- |
| 6c.1 | Body length | | The barrel that unscrews |
| 6c.2 | Body diameter | | |
| 6c.3 | Overall length including wire tails | | 16 AWG is stiff — this needs bend room |

## 6d. Mini breadboard (170-point)

| #    | Dimension | Reference | Measured | Notes |
| ---- | --------- | --------- | -------- | ----- |
| 6d.1 | Length | ~47 | 48.05mm | it is ~46.5mm but there are protruding little bits on the top and left for a locking mechanism that ive added in |
| 6d.2 | Width | ~35 |36.20mm  | 34.85mm without the locking mechanism bit |
| 6d.3 | Height | ~9 | 9.70mm | Including the adhesive backing if you keep it, should i keep it? |
| 6d.4 | Does it have adhesive backing? | | yes | Changes how it mounts |

## 6e. Sensor — the white dome PIR, re-measure please

Section 5 says the PCB is 8.16 × 10.34 mm, but that is far too small for an
HC-SR501 (normally ~32 × 24 mm) — and you also described two potentiometers and
a jumper block, which only exist on the larger board. One of those is wrong, and
**I cannot cut the front plate aperture until this is settled.**

| #    | Dimension | Measured | Notes |
| ---- | --------- | -------- | ----- |
| 6e.1 | PCB length × width | 8.20x10.20mm | The green/black board, ignoring the dome |
| 6e.2 | PCB thickness | 1.75mm | |
| 6e.3 | Dome outer diameter | 12.20mm | |
| 6e.4 | Dome lip diameter | 13.45mm | The flange that would sit against the inside of the plate |
| 6e.5 | Dome height above the PCB | 10.50mm | |
| 6e.6 | Tallest component on the back face | 1.50mm | Pots, jumper, pins |
| 6e.7 | Pin positions relative to the PCB edge | the pins come out and bend 90 degrees down, look at HW-740.jpeg reference image | For the dupont clearance |
| 6e.8 | Is there a mounting hole anywhere? | no mounting holes | |

## 7. Power board (TPS63020 buck-boost, i have this now)

Module purchased: "XL63020 / TPS63020 5V Lithium Battery USB Auto Buck-Boost".

| #   | Dimension                              | Measured | Notes                       |
| --- | -------------------------------------- | -------- | --------------------------- |
| 7.1 | PCB length × width                     | 26.5x17.6mm | Confirm with calipers when it arrives |
| 7.2 | Max component height                   | 2.90mm | Rest of board is thin |
| 7.3 | Mounting holes: diameter + spacing     | 2 holes in each of 4 corners | Measure hole dia + centre spacing |
| 7.4 | Input/output pad or terminal positions |          |                             |
| 7.5 | **Output voltage: fixed 5 V or adjustable?** |    | Decides whether we feed the 5V pin or the 3V3 pin — see below |

> **Open question that changes the power design:** the listing says "5V", which
> would mean feeding the ESP32-CAM's 5V pin and going through its onboard
> AMS1117 — costing ~5 mA of sleep current and roughly halving runtime. If the
> module has a trim pot or a solder jumper for output voltage, setting it to
> 3.3 V and feeding the 3V3 pin directly avoids that. Check the board for an
> adjustment when it arrives, and measure the actual output before connecting
> it to anything.

## 6b. Battery holder wiring — SERIES or PARALLEL?

**This is unresolved and it determines the entire power design.** Most cheap
multi-cell 18650 holders with flying leads are wired in **series** internally,
not parallel — the metal strips daisy-chain the cells.

Measure it: install charged cells and read the holder's output with a
multimeter.

| Reading | Wiring | Implication |
| --- | --- | --- |
| ~3.7–4.2 V | Parallel | Buck-boost to 3.3 V as planned |
| ~11–12.6 V (3 cells) | **Series (3S)** | Needs a step-*down* converter, and explains why a buck worked on this project before |
| ~14.8–16.8 V (4 cells) | Series (4S) | Same, higher input |

## 8. Wiring — dupont connectors

**Decision: dupont, not soldered.** Connections stay serviceable and the board
can be swapped without a soldering iron.

The cost is depth: a dupont shell stands roughly 14 mm off the header, plus wire
bend radius, versus about 3 mm for soldered-and-heatshrunk wire. Indoors there
is no real size constraint, so this is a fine trade — but I need the real
numbers, because the housing gets designed around them.

To recover most of that depth, the design will route the wires **sideways out
of the connector into a channel** rather than straight back. That needs 8.2 and
8.3 to be accurate.

| #   | Dimension                             | Reference | Measured | Notes                                     |
| --- | ------------------------------------- | --------- | -------- | ----------------------------------------- |
| 8.1 | Dupont shell height above the pin tip | ~14       | 14.13mm | Measure an assembled one on a header      |
| 8.2 | Dupont shell width (single)           | ~2.5      | 2.7mm | Determines how many sit side by side      |
| 8.3 | Minimum bend radius of your wire      |           | idk, not important | Bend it until it kinks, measure the arc   |
| 8.4 | Wire outer diameter incl. insulation  |           | 1.44mm |                                           |
| 8.5 | Number of wires leaving the board     |           | 4 | 5V, GND, sensor signal, plus reflash pins |
| 8.6 | Are your shells single or ganged?     |           | 1x1 shells | 1x1 shells, or 4-way blocks?              |

## 9. Printer / process

| #   | Item                    | Value | Notes                                                 |
| --- | ----------------------- | ----- | ----------------------------------------------------- |
| 9.1 | Nozzle diameter         | 0.4  | Confirmed                                               |
| 9.2 | Layer height you'll use | default |                                                       |
| 9.3 | Filament                | PLA   | Colour matters for heat if near a window              |
| 9.4 | Measured hole shrinkage |       | Print a test coupon; holes usually come out undersize |

## 10. Mounting location

| #    | Item                                | Value | Notes                                       |
| ---- | ----------------------------------- | ----- | ------------------------------------------- |
| 10.1 | Wall material                       | drywall | Drywall, brick, wood — sets the anchor type |
| 10.2 | Mounting height                     | 5ft - 9ft | Affects camera tilt angle                   |
| 10.3 | Desired tilt range                  | ability to tilt up/down 45 degrees, side/side 90 degrees each way | How far down does it need to aim?           |
| 10.4 | Approximate distance to WiFi router | roughly 25 feet | For the Phase 5 RSSI comparison             |
