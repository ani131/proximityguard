# ProximityGuard · ECE 362

Two Purdue Proton boards (RP2350B) make a tabletop collision-warning demo. The sensor board measures distance and simulated speed. The display board receives both over UART1, decides the risk, updates the MSP2202 over SPI1, and plays PWM warnings. The Google Doc specifies a passive piezo. The approved Iris checkoff specifies prerecorded audio through an LM386 and speaker. Both outputs run simultaneously and may be connected independently.

## Build and flash

1. Open the folder containing `platformio.ini` in VS Code with PlatformIO, just as in Lab 0.
2. Connect only the sensor Proton's debug USB. Select **Project Tasks → sensor → Upload**. Label this board A.
3. Disconnect A's debugger USB and connect B's. Select **Project Tasks → display → Upload**. Label this board B.
4. Disconnect USB power before wiring. After wiring, power each board from its own USB connection. Join the two grounds and the single UART data wire. Keep the two USB-derived 5 V rails and the two 3V3 rails separate. GP0 and GP1 remain available for the lab debugger.

The download also includes ready-built UF2 files. To flash without compiling, hold BOOTSEL while plugging the target Proton into its native USB port. When the RP2350 drive appears, release BOOTSEL and copy only the matching file from the firmware folder: sensor.uf2 for A, display.uf2 for B. The board reboots after copying.

Terminal equivalents from the project folder are `pio run` to build both, `pio run -e sensor -t upload` on A, and `pio run -e display -t upload` on B.

## Board A · sensor

Use the GP labels printed on the Proton. The 5 V and 3V3 pins are separate.

| From | To |
|---|---|
| A 5V | HC-SR04 VCC |
| A GND | HC-SR04 GND and potentiometer low outer leg |
| A GP6 | HC-SR04 TRIG |
| HC-SR04 ECHO | 2.2 kΩ resistor → junction → A GP7 |
| Same GP7 junction | 3.3 kΩ resistor → A GND |
| A 3V3 | Potentiometer high outer leg |
| Potentiometer wiper (center) | A GP45, ADC channel 5 |
| A GP4 (UART1 TX) | B GP5 (UART1 RX) |
| A GND | B GND |

The echo divider brings a 5 V pulse to approximately 3.0 V at GP7. Wire both resistors before powering the sensor. The pot ends can be swapped to reverse knob direction. No return UART wire is needed.

## Board B · MSP2202

Use the nine-pin LCD header. The SD header stays open.

| LCD pin | Board B |
|---|---|
| 1 VCC | 3V3 |
| 2 GND | GND |
| 3 CS | GP9 |
| 4 nRESET | GP13 |
| 5 DC | GP12 |
| 6 SDI/MOSI | GP11 (SPI1 TX) |
| 7 SCK | GP10 (SPI1 SCK) |
| 8 LED | 3V3 |
| 9 SDO/MISO | Leave open |

Place 0.1 µF ceramic and 10 µF capacitors in parallel across LCD pins 1 and 2 near the module. If the 10 µF capacitor is polarized, its positive lead goes to pin 1.

## Board B · warning outputs

**Piezo from the proposal.** GP18 → 330 Ω → passive piezo positive. Piezo negative → B GND. Caution gives a short beep every 800 ms, danger every 200 ms, and link loss a longer beep every two seconds.

**Speaker for the Iris checkoff.** GP16 carries 8 kHz unsigned PCM as a PWM duty cycle. Three stored clips say “Caution,” “Danger,” and “Connection lost.” Use an LM386N-1 and an 8 Ω speaker. Follow the pin numbers viewed from the top of the DIP package. Place the notch at the top. Pin 1 is upper left, pin 4 lower left, pin 5 lower right, and pin 8 upper right.

| From | To |
|---|---|
| B GP16 | 1 kΩ → filter junction |
| Filter junction | 47 nF ceramic → B GND |
| Filter junction | 1 µF nonpolar coupling capacitor → top of a separate 10 kΩ audio volume pot |
| Volume pot | Bottom → B GND, wiper → LM386 pin 3 (+ input) |
| LM386 pins 2 and 4 | B GND |
| LM386 pin 6 | B 5V. Place 0.1 µF ceramic and 100 µF from pin 6 to pin 4, positive toward pin 6 |
| LM386 pin 5 | Positive lead of 220 µF capacitor → negative lead → speaker positive |
| Speaker negative | B GND |
| LM386 pin 5 | 10 Ω in series with 47 nF → B GND |
| LM386 pins 1, 7, 8 | Leave open (default gain 20) |

Start with the volume wiper at the grounded end. Turn it up slowly until speech is clear. If using an assembled LM386 module, use its labeled signal input, ground, supply and speaker terminals according to its own schematic. The DIP pin map above is for the bare chip.

## Bench check

Put a rigid, flat target perpendicular to the sensor. Measure from the front of the transducers. The transmitter takes a reading about every 100 ms. Bench thresholds are deliberately scaled to centimeters for a tabletop demonstration.

| Test | Expected on B |
|---|---|
| Set knob to 0 mph. Move target to 50 cm, 20 cm, then 7 cm | SAFE, CAUTION, then DANGER. Caution and danger have distinct speech and beep rates |
| Hold target at 30 cm. Sweep the knob from 0 to 60 mph | SAFE at 0, CAUTION at 20, DANGER at 60. Speed increases while distance stays near 30 cm |
| Unplug only A GP4 → B GP5 | Within about half a second the LCD replaces both values with dashes and shows CONNECTION ERROR. The speaker announces the lost link |
| Reconnect the UART wire | Live values and normal warnings resume after the next valid frame |
| Disconnect ECHO at the sensor end, keeping the GP7 pulldown in place | NO ECHO, blank distance, live speed. Reconnect ECHO to recover |

The danger boundary in centimeters is 10 + floor(speed / 2). The caution boundary is 25 + speed. Both include equality. Outside those boundaries the status is safe. A missing or out-of-range echo gets its own status. UART uses 115200 baud, 8N1. A two-byte sync word and CRC-8 let the receiver recover after a partial or corrupted packet.

## Code and checks

- `src/sensor.c` measures echo width, averages eight ADC readings, and transmits.
- `src/display.c` receives, checks freshness and updates the LCD. `src/protocol.c` contains the wire format and risk thresholds.
- `src/audio.c` streams the prerecorded samples in `src/clips.inc` using a timer interrupt. It also drives the passive piezo on a separate PWM slice.
- `include/pins.h` is the wiring reference used by both firmware targets and the display driver.
- Run the host test with `cc -std=c11 -Iinclude tests/protocol_test.c src/protocol.c -o protocol_test` followed by `./protocol_test`. Physical timing, LCD operation and sound require the breadboard checks above.

The display driver and fonts in `src/lcd.c` and `include/lcd.h` are adapted from Purdue ECE 362's MSP2202 project example. The original course license is in `LCD-LICENSE.txt`. The three audio clips were synthesized offline using Flite and are stored on the display board.

[Course LCD guide](https://github.com/ece362-purdue/proton-labs/tree/main/project-lcd) · [Proton pinout](https://ece362-purdue.github.io/proton-labs/assets/Proton%20Pinout%20Diagram.pdf) · [HC-SR04 datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) · [TI LM386 datasheet](https://www.ti.com/lit/ds/symlink/lm386.pdf)
