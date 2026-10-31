# Three-stage pipelines

The shell accepts two or three commands, e.g. `cat test.s | grep movs | hexdump`.
A three-command pipeline uses two 256-byte kernel pipes. The shell starts all
children before waiting, closes its unused endpoints, and restores stdin/stdout.
Redirections, background jobs, quoting, and four-stage pipelines remain unsupported.

On RP2040, verify repeated runs and EOF behavior on hardware. This source was
syntax-checked with Clang for armv6m, but not built with Pico SDK or hardware-tested.
