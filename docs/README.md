<!-- SPDX-License-Identifier: Apache-2.0 -->
# EmbLink Studio documentation

| Document | What it holds | Read it when |
| --- | --- | --- |
| [Specification](specification.md) | What EmbLink Studio is, the decisions taken, the architecture, the core's JSON-RPC interface, the project format, the views, the stages S0 to S9 and the engineering rules | You want to know what to build, in what order, and why |
| [Design brief](design/brief.md) | How EmbLink Studio looks, feels and behaves: the direction, configuring the MCC way, the debugger. A draft, decided step by step | You design or build anything the user sees |
| [Development](development.md) | Setting up a machine and an editor, the build and test commands, and how a change reaches `main` | You are about to change the code |
| [Protocol schemas](../protocol/README.md) | One JSON Schema per method's parameters and result | You change a method of the core's interface |
| [Contributing](../CONTRIBUTING.md) | The CLA and the DCO, the file header, formatting and licence rules | Before your first pull request |

## The specification is the source of truth

When the code says "spec section 5" or "§11 q4", it means [specification.md](specification.md), section 5 or
question 4 of section 11. The specification is changed by pull request and agreed with the owner. A decision
is added to §2 with its date, and a pull request that changes behaviour updates the specification with it.

## Where the project stands

Stage S0 (setup) is done: the application starts the core, initializes it, shows its log and offers a restart
if it stops. CI builds and tests it on Windows, Linux and macOS. S1, the core's foundation, is next (§8).
