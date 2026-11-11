# rmhacks — preload-based hacks for reMarkable 2 (firmware 3.29.0.153)

A prototype replacement for [remarkable-hacks](https://github.com/ddvk/remarkable-hacks)
style binary patching. Instead of `bsdiff`-patching `/usr/bin/xochitl`,
this is an `LD_PRELOAD` library that hooks into the running process:
| Technique | Used for |
|---|---|
| PLT interposition (`QCoreApplication::exec`) | safe bootstrap after the Qt app object exists |
| global Qt event filter | sees every touch/pen/key event → gesture recognition |
| runtime-resolved internal calls (`getInstance`, `updateRequest`) | actions inside xochitl without touching its code |

Nothing modifies `/usr/bin/xochitl` — uninstalling is just removing the
preload. The old remarkable-hacks cannot be ported this way for free, but
every hack becomes ordinary, maintainable C++.

## Current hacks

- **two finger tap → full screen refresh** (the vertical-slice demo:
  input → gesture → internal updateRequest call)
- set `RMHACKS_DEBUG=1` for verbose event/gesture logging (first 400 events)

## Address table

`rmhacks.conf` (rm2fb.conf compatible format) carries the per-firmware
offsets. Only **3.29.0.153** is populated; the parser also reads
`/etc/rm2fb.conf` / `/opt/etc/rm2fb.conf` so a shared conf works.

xochitl ≥ 3.29 is PIE, so all offsets are rebased at runtime with the load
bias computed from `/proc/self/maps` + the ELF program headers.

## Building

Cross-compiling needs the official reMarkable SDK (armhf GCC + the device
Qt 6 sysroot). The [Containerfile](Containerfile) in this repo wraps it in
Docker; reMarkable publishes the toolchain for x86_64 and aarch64 hosts,
so it builds and runs natively on both Intel and Apple Silicon.

**Build the image once** (~390 MB SDK download, ~2.7 GB installed, ~4 GB
image):

```bash
docker build -f Containerfile -t rmhacks-build .
```

It is based on `debian:bookworm` and installs the reMarkable SDK
`5.8.203` (firmware 3.28 lineage — the newest published toolchain) to
`/opt/codex/rm2/5.8.203`, and exports its environment script as
`$SDK_ENV`.

**Then compile**, mounting this repo at `/work`:

```bash
docker run --rm -v "$PWD":/work -w /work rmhacks-build bash -c '\
  . "$SDK_ENV" && qmake && make -j"$(nproc)"'
```

Or keep a persistent container around (like a local dev box):

```bash
docker run -dit --name rmhacks-build -v "$PWD":/work -w /work rmhacks-build
docker exec rmhacks-build bash -c '. "$SDK_ENV" && qmake && make -j"$(nproc)"'
```

Output: `librmhacks.so.0.1.0` (armhf, links the device Qt 6).

## Installing / testing

```bash
scp librmhacks.so.0.1.0 rmhacks.conf root@10.11.99.1:/home/root/
ssh root@10.11.99.1

# try it (stops the normal UI, runs xochitl with the hack):
systemctl stop xochitl
cd /home/root && LD_PRELOAD=/home/root/librmhacks.so.0.1.0 /usr/bin/xochitl
# two-finger tap anywhere -> full refresh; Ctrl-C when done, then:
systemctl start xochitl

# make it permanent:
install -m 0644 rmhacks.conf /etc/rmhacks.conf
mkdir -p /etc/systemd/system/xochitl.service.d
printf '[Service]\nEnvironment=LD_PRELOAD=/home/root/librmhacks.so.0.1.0\n' \
  > /etc/systemd/system/xochitl.service.d/rmhacks.conf
systemctl daemon-reload && systemctl restart xochitl

# undo:
rm /etc/systemd/system/xochitl.service.d/rmhacks.conf /etc/rmhacks.conf
systemctl daemon-reload && systemctl restart xochitl
```

If it coexists with rm2fb, list both in `LD_PRELOAD`
(`LD_PRELOAD=librm2fb_server.so.1.0.3:librmhacks.so.0.1.0`); rmhacks only
submits requests through xochitl's own pipeline and does not fight over
the framebuffer.

## Adding a hack

1. implement the action in `src/actions.cpp` (call internals via
   `xochitl::runtime()`, or plain Qt APIs)
2. recognize the gesture in `src/gestures.cpp` and call the action
3. if new xochitl internals are needed, add their offsets to
   `rmhacks.conf` (see the
   [remarkable2-framebuffer tutorial](https://github.com/ddvk/remarkable2-framebuffer/blob/master/tutorial/README.md)
   for how to locate functions in the binaries)

## Credits / licensing

This project is MIT licensed (see [LICENSE](LICENSE)). It is original
code, but it stands on two MIT licensed projects by **ddvk**:

- [remarkable-hacks](https://github.com/ddvk/remarkable-hacks) — the
  gesture-hack concept and feature inspiration
- [remarkable2-framebuffer](https://github.com/ddvk/remarkable2-framebuffer) —
  the per-version address conf format, the preload interposition approach,
  and the function-mapping methodology (some utility code here is derived
  from patches written for it)

Full license texts: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
No reMarkable code is contained in or derived into this project.

## Status / caveats

- prototype: gesture heuristics are deliberately minimal
- pen input on the rM2 arrives as touch/tablet events; both are visible to
  the filter — the demo only reacts to two-finger touch taps
- only the 3.29.0.153 xochitl (BuildID
  `c1a6698e…`) has verified offsets
