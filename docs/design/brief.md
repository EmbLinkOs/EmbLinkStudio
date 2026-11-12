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

**Widened on 2026-11-12, after the owner's feedback on the first draft:**

- **Pin manager:** every I/O pin with its mode (input, output, analog, alternate), pull, output type, speed, start
  level and interrupt edge, each active only where the mode allows it; filters for pins in use, free pins and all
  50; board-wired pins (the LED, the button, the ST-LINK) fixed by the board description.
- **Pin grid,** as in MCC: functions down, the 50 I/O pins across grouped by port, each cell free, assigned, in
  conflict, or fixed and locked. A click moves a function to another pin that can carry it; for the ADC it picks
  channels.
- **Peripheral forms in sections** (basic, advanced, interrupts and DMA, driver): flow control, filters, triggers,
  prescalers, the DMA stream and channel each request uses, interrupt priorities, the EmbLinkRTOS driver class, the
  device name and the devices on a bus. Checks such as an ADC clock above 36 MHz, or a button pull-down fighting the
  board's pull-up.

## 3a. Configuring the RTOS

**Configure › RTOS**, beside **Configure › Hardware**. The same canvas idea, applied to `system.yaml`:

- **Threads sit on a priority axis**, so changing a priority moves the thread. Kernel threads (the system work queue,
  idle) are shown too, set by the kernel settings.
- **Objects** (queues, mutexes, semaphores, event flags, timers) sit between the threads, and lines say who sends,
  receives, locks, takes, waits or runs. **Interrupts** feed them from the right.
- **Computed live:** a ceiling mutex's ceiling follows its highest user (SPEC-005 §3.4), drawn on the axis; stacks,
  queue buffers and the kernel's own memory go into the RAM budget.
- **Kernel settings get a full page of their own: Configure › Kernel** (the owner, 2026-11-12: too important for the
  inspector). Laid out like macOS System Settings:
  - categories on the left (profile and build, scheduler, time, threads, synchronization, communication, timers and
    work queues, devices and drivers, objects, interrupts, faults, observability), each with a count of changed
    settings and a dot when one has a warning;
  - the reference **profile** as large cards (tiny, base, isolated; multicore and native shown, and why they do not
    apply to this chip). Settings not changed by hand follow the profile;
  - every option with its help text from `Kconfig`, its key, its reference, its default, a reset, and why it is
    greyed out when a dependency is missing ("Needs priority inheritance: the bus is held by a mutex");
  - search across all options, and a "changed only" filter;
  - an **effect panel**: the RAM change, the checks (also across screens: the I2C driver that I2C1 needs), and the
    exact `system.yaml` diff, applied only when no check is red.
- **Checks in place:** a priority out of range, a queue item above `EMB_MSGQ_MAX_ITEM`, time slicing with no two
  threads at one priority, a timer period that is not a whole number of ticks, no debug descriptor.
- In the Debug stage the same graph comes alive: threads light up as they run, queues fill and drain.

## 3b. Every board, every chip

EmbLink Studio supports every board EmbLinkRTOS describes: 12 boards of 7 families today, from an 8-bit AVR to
simulated boards with no pins. **No screen is designed for one chip.** The mockups show the NUCLEO-F446RE only as an
example; every screen draws from the board's resolved model (`hw.json`), as the specification requires (§4, §6).

| What changes per chip | How the screens adapt |
| --- | --- |
| The package (`lqfp64`, `lqfp144`, `qfn60`, `pdip28`, none) | The chip is drawn from it: four sides for QFP and QFN (QFN with its pad), two rows for DIP, a ball grid for BGA. A simulated board (MPS2, QEMU) shows its memory map and peripherals instead |
| How a pin gets its function: an AF number per pin (STM32), a function slot per GPIO (RP2350), or fixed by the peripheral (AVR) | The pin grid shows `AF7`, `F3`, or a fixed cell that cannot be chosen. Conflicts and moves work the same way for all three |
| The peripherals and what they offer (DMA, PIO, FPU, MPU) | The catalogue is the chip's; forms show only the sections the chip has, so no DMA section on an ATmega |
| The clock tree | Every readout (baud error, SCL, ADC rate) is computed by the core from the chip's clocks, never in the GUI |
| What the kernel supports on the chip | The Kernel page follows the port's `Kconfig` defaults and limits: `tiny` on AVR, reboot on fault, no debug descriptor |

A board is chosen when a project is created (the Start screen), and the board pill in the toolbar shows and changes
it for an open project. A board the views do not cover fully still opens: what they cannot show is listed read-only
with its source (specification §1).

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

**First draft, 2026-11-12** (artboard "Direction B + C · Debug"), halted at a breakpoint on the NUCLEO-F446RE:

- the controls in the toolbar (continue, pause, step over, into, out, by instruction, reset, stop) and a status pill
  that says why the target stopped: a breakpoint, a step, a watchpoint, a pause;
- the RTOS threads from the debug descriptor, each with its state and stack high-water mark, and the call stack of
  the selected one, from its live registers or from the context the kernel saved;
- breakpoints and watchpoints, with the hardware they use (Cortex-M4: 6 FPB comparators, 4 DWT comparators);
- the source with the Thumb-2 disassembly interleaved, the current line and instruction, values shown inline;
- variables: locals, a watch list, and live expressions read over SWD while the target runs;
- core and FPU registers, the ones the last step changed in amber; peripheral registers decoded from the SVD, with
  their fields, writable while halted;
- a bottom panel: the console (UART, ITM over SWO, GDB), the RTOS timeline from the trace ring, the kernel objects
  live, and the last crash record decoded, linked back to Configure › RTOS;
- a **memory viewer** as an editor tab, like VS Code's hex editor: go to an address or a symbol, the chip's regions
  (from the board) and a clickable map of the build's sections and thread stacks, a 16-byte grid as bytes, 16-bit,
  32-bit or float values with ASCII, symbols coloured in place, changed bytes marked, a data inspector that decodes a
  pointer to the symbol it points at, writes while halted, and a warning where a read has side effects.

**Code text uses VS Code's metrics on macOS:** a 12 px monospace font on 18 px lines (owner, 2026-11-12).

## 4a. The code editor

First draft, 2026-11-12 (artboard "Direction B + C · Code"). The best of the editors people like, plus what only an
RTOS-aware editor can show:

| From | What |
| --- | --- |
| VS Code | tabs, breadcrumbs, the minimap with problem marks, peek definition, Problems, Terminal, split editor, its line metrics |
| CLion | inlay hints with parameter names, code lenses above functions, quick fixes |
| Xcode | issues shown at the end of their line, each with its fixes |
| Sublime | one box to go to anything: files, symbols, pins, threads, settings, commands |
| GitLens | change bars in the gutter, blame on the current line |

Only EmbLink Studio:

- **Lenses with RTOS facts:** the thread a function runs as, its priority, stack and last high-water mark, its flash
  size from the build; an interrupt handler's IRQ, class and priority.
- **Hardware names explained:** `pin_led`, `dev_bme280`, `log_q` or `EXTI15_10_IRQn` open a card with the pin, the bus,
  the queue or the IRQ they stand for, where they are defined, and their state on the last run.
- **RTOS checks with fixes:** a blocking call in an interrupt handler (an error, with "use EMB_NO_WAIT"); a call chain
  deeper than its thread's stack, from `-fstack-usage` (with "raise the stack in system.yaml").
- **Completion that knows contexts:** each function says whether it is safe in an interrupt.
- **Generated code read-only,** with a banner pointing to Configure; renaming a generated name sends you to its source.

### Every part is a panel (owner, 2026-11-12)

As in VS Code, and further: Explorer, Outline, Source Control, Commits, Problems, Build, Terminal, Serial and
Extensions are each a panel of their own, with their own scroll. Any panel can be dragged by its title, or moved from
its menu, to the left or right side bar, the panel area below, the editor area as a tab, or a floating window. Side
bars stack panels and fold them; the panel area shows them as tabs and can be maximized. The editor splits into
groups, and tabs move between them. Splitters resize everything. Layouts can be saved, with presets (Default, Focus,
Review, Terminal), per project (§3, specification §6).

**Polish, 2026-11-12 (owner: "elegant, still pro").** Every area is a card with rounded corners on a darker ground,
6 px apart. Panel titles in normal case, actions that brighten on hover, a slim indicator in the activity bar, and a
quiet status bar. Panels, tabs and floating windows are moved by dragging, with docking guides as in Visual Studio:
release on a guide to dock (left, right, panel area, editor tab, split), anywhere else to float. The Explorer has
type icons, folders that fold, indent guides, git colours on names and folders, ignored and generated folders
marked, and a filter.

### Extensions, Git first

Features that not every user needs are **extensions**, as in VS Code, so they can be turned off: Git, the language
server, the serial monitor, the trace timeline, the EmbSIM runner. **Git** is built in: stage, unstage, commit with
the DCO sign-off on by default, push, branches, the commit graph, diffs in the editor, change bars and blame. Turning
an extension off removes its panels and its decorations in the editor.

## 4b. Start

First draft, 2026-11-12 (artboard "Direction B + C · Start"):

- **The welcome window:** the app's mark and version, four actions (new project, open, clone from Git, run an
  example), the board **connected now** (its probe found on USB) with "new project for this board", recent
  projects with their board and last build, and the examples EmbLinkRTOS ships.
- **New project in three steps,** each one shown in a stepper that keeps the choices made:
  1. **Board:** every board EmbLinkRTOS describes, filtered (connected, hardware, simulated) and searched, each
     drawn from its package and marked connected, draft (not yet curated), simulated, or not available yet (an
     ESP32 whose SoC is not described).
  2. **Template:** empty, blinky, console shell, sensor and logger, SPI device. A template that needs hardware the
     board does not describe is shown with what is missing ("this board has no LED the RTOS can drive").
  3. **Details:** name, location, the EmbLinkRTOS release pinned, the profile from the board's defaults, the
     toolchain found or downloaded, git with a signed-off first commit, build right away.
- **Creating** shows each step as the core runs it: the manifests, the pinned RTOS, generation, CMake, git, the
  first build and its sizes. Then: open in Configure or Code, or flash and run.
- **A cloned project opens untrusted** (specification §9): its files are shown, nothing in it runs until trusted.

## 5. Open questions

1. **Qt Quick or Qt Widgets for the frame.** A fluid, animated macOS feel is far easier in Qt Quick. The
   specification chose Widgets for the frame (§11 q2), and the docking library (Qt Advanced Docking System) is
   Widgets-only. Before deciding, build the same small screen both ways and compare.
2. **Light appearance.** Dark is designed first; a light appearance, as macOS offers, follows the same tokens.
3. **The other screens:** Start, Code, Debug, Verify.
4. **Drawing a connection:** dragging from a thread to an object to say who uses it.
5. **The word for IDE plug-ins.** "Module" already means a firmware code package (EmbLinkRTOS-parts, specification §5,
   §8). The mockups say **extension** for IDE plug-ins such as Git, to keep the two apart; the owner decides.

## 6. How the design proceeds

One step at a time, each decision written here before anything is built:

1. The feeling (§1): done.
2. The signature moments: the 3 to 5 interactions people remember.
3. How you move around: stages, the command palette, the inspector, the keyboard.
4. The visual language, as design tokens (colour, type, spacing, radii, motion) that Qt uses everywhere.
5. The screens, each mocked up and iterated: Configure › Hardware and Configure › RTOS (first drafts done; the
   layout is liked, the options need widening), then Start, Code, Debug, Verify.
