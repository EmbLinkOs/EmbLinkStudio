<!-- SPDX-License-Identifier: Apache-2.0 -->
# Design brief

**Draft · 2026-11-12 · Owner: Junior Deogracias (teo1747)**

How EmbLink Studio should look, feel and behave. The specification (`../specification.md`) says *what* the
product does; this brief says *how it is experienced*. Nothing here is built until the owner accepts it, and
accepted points move into the specification (§7) with their date.

## 1. The feeling

macOS quality: **fluid, natural, elegant, soft on the eyes, and clearly professional.** The owner's reference is
Apple Music. The product has a personality of its own.

To avoid:

- the mechanical feel of Windows and Android interfaces;
- looking like VS Code, or any general-purpose editor;
- a static screen: values, signals and state should visibly live.

## 2. The direction: Instrument and Canvas

Chosen by the owner on 2026-11-12, from three directions (Studio, Instrument, Canvas).

- **Instrument.** Graphite tones, fine precise lines, live data that moves: values recompute as you type, signals
  flow along the lines of the selected peripheral, the connected board pulses. Like a premium measuring
  instrument.
- **Canvas.** The board is the centre. The chip is drawn large with its peripherals wired to their pins; you zoom
  from the board to a peripheral to its registers.
- **Soft, not harsh:** rounded cards, gentle shadows, one accent colour (cyan) with green, amber and red only for
  meaning (valid, warning, error).

The first mockup of this direction is the **Configure** screen (design canvas, artboard "Direction B + C ·
Configure").

## 3. Configuring the board, the MCC way

The owner's reference is Microchip's MPLAB Code Configurator (MCC), its device resources and its pin manager.

- **Peripheral catalogue.** Every peripheral of the chip (ADC, timers and PWM, UART, SPI, I2C, DMA, the external
  memory interface, and so on), grouped. **Add** one, and it moves to *In this project*, is drawn on the canvas
  and opens in the inspector.
- **Configuration form.** Each peripheral's parameters, with sensible defaults. **Readouts computed live** from the
  clock tree, the way an engineer checks them: actual baud rate and its error, SCL frequency, ADC conversion time
  and rate, SPI clock.
- **Conflicts shown and resolved in place.** A pin two peripherals want is drawn in amber on the chip and in the
  list; the inspector explains it and offers the fix (for example, move SPI1 SCK from PA5 to PB3).
- **Pin manager.** All pins in a list:
  - the function each pin carries, and the peripheral it belongs to;
  - a **custom name** (`LED_STATUS`, `SENSOR_SDA`) that the generated code uses;
  - electrical options: mode, pull-up or pull-down, speed, output type, start level;
  - a **lock**, so the generator never moves that pin when it resolves a conflict.
- Every change still goes through the core into `board.yaml` and `system.yaml`, shown as a diff before anything is
  written (specification §5, §6).

**From MCC, still to bring in** (its Pin Manager and Pins view): per-pin *start high*, analog, direction, weak
pull-up, open-drain and interrupt-on-change; and its **pin grid**, functions in rows and pins in columns, each cell a
lock that is free, taken or locked. The first draft's pin and peripheral options are too few: the next round widens
both (owner, 2026-11-12).

## 3a. Configuring the RTOS

**Configure › RTOS**, beside **Configure › Hardware**. The same canvas idea, applied to `system.yaml`:

- **Threads sit on a priority axis**, so changing a priority moves the thread. Kernel threads (the system work queue,
  idle) are shown too, set by the kernel settings.
- **Objects** (queues, mutexes, semaphores, event flags, timers) sit between the threads, and lines say who sends,
  receives, locks, takes, waits or runs. **Interrupts** feed them from the right.
- **Computed live:** a ceiling mutex's ceiling follows its highest user (SPEC-005 §3.4), drawn on the axis; stacks,
  queue buffers and the kernel's own memory go into the RAM budget.
- **Kernel settings**, grouped as EmbLinkRTOS's `Kconfig` groups them (build, scheduler, time, threads,
  synchronization, communication, interrupts, faults, observability). Each shows its option name (`EMB_TICK_NS`)
  and the specification or requirement it comes from.
- **Checks in place:** a priority out of range, a queue item above `EMB_MSGQ_MAX_ITEM`, time slicing with no two
  threads at one priority, a timer period that is not a whole number of ticks, no debug descriptor.
- In the Debug stage the same graph comes alive: threads light up as they run, queues fill and drain.

## 4. A complete debugger

Everything a professional embedded developer expects, plus what only an RTOS-aware tool can show:

| Area | What it has |
| --- | --- |
| Control | Run, halt, step by line or by instruction, step over and out, reset, flash and restart |
| Breakpoints | On a line or a function, conditional, by hit count; data watchpoints (stop when a variable changes) |
| Code | Disassembly, with the source interleaved |
| State | Core and FPU registers; peripheral registers decoded from the SVD file, and editable; memory view (hex and typed) |
| Program | Call stack per thread, local variables, watch list, live expressions |
| RTOS | Threads and their states, objects, stacks and high-water marks, the timeline |
| Trouble | Fault analyzer, crash-record decoding, SWO/ITM console and trace |

## 5. Open questions

1. **Qt Quick or Qt Widgets for the frame.** A fluid, animated macOS feel is far easier in Qt Quick. The
   specification chose Widgets for the frame (§11 q2), and the docking library (Qt Advanced Docking System) is
   Widgets-only. Before deciding, build the same small screen both ways and compare.
2. **Light appearance.** Dark is designed first; a light appearance, as macOS offers, follows the same tokens.
3. **The other screens:** Start, Code, Debug, Verify.
4. **Drawing a connection:** dragging from a thread to an object to say who uses it.

## 6. How the design proceeds

One step at a time, each decision written here before anything is built:

1. The feeling (§1): done.
2. The signature moments: the 3 to 5 interactions people remember.
3. How you move around: stages, the command palette, the inspector, the keyboard.
4. The visual language, as design tokens (colour, type, spacing, radii, motion) that Qt uses everywhere.
5. The screens, each mocked up and iterated: Configure › Hardware and Configure › RTOS (first drafts done; the
   layout is liked, the options need widening), then Start, Code, Debug, Verify.
