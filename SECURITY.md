<!-- SPDX-License-Identifier: Apache-2.0 -->
# Security policy

## Supported versions

EmbLink Studio has not had a release yet. Security fixes go to the `main` branch; once releases
start, this table will list the versions that receive them.

| Version | Supported |
|---|---|
| `main` | Yes |

## Reporting a vulnerability

**Do not open a public issue for a security problem.** Report it privately through GitHub:
[Report a vulnerability](https://github.com/EmbLinkOs/EmbLinkStudio/security/advisories/new)
(the *Security* tab, then *Report a vulnerability*). Only the maintainers can see the report.

Please include:

- what the problem is and what an attacker could do with it;
- the steps to reproduce it, or a proof of concept;
- the version or commit, and your operating system;
- whether you want to be credited, and under what name.

## What happens next

The report is acknowledged within a week. Once the problem is confirmed, a fix is prepared in
a private fork and released together with a GitHub security advisory that credits the reporter,
unless they ask not to be named. Please keep the problem private until the advisory is out.

## Scope

In scope: the EmbLink Studio desktop application, EmbLink Studio Core and the protocol between
them. Problems in a third-party component (listed in `third_party/components.json`) are best
reported to that project as well.
