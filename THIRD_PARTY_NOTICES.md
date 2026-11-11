# Third-party notices

This project is original code, but its design borrows heavily from two MIT
licensed projects by ddvk. Their license texts are reproduced below as
required for attribution; see also the "Credits" section of the README.

## remarkable-hacks

Conceptual inspiration: the gesture-hack feature set (two finger swipes,
taps and long presses triggering device actions) and the idea of extending
xochitl with quality-of-life shortcuts. No code was derived from it (that
project ships binary patches, not source).

> MIT License
>
> Copyright (c) 2020 ddvk
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## remarkable2-framebuffer

Technical foundation: the per-version address configuration format
(`rm2fb.conf`), the LD_PRELOAD interposition approach for injecting into
xochitl, calling xochitl-internal functions through runtime resolved
pointers, Qt metacall based framebuffer actions, and the function mapping
methodology from its decompilation tutorial. The PIE load-bias resolver and
the dlsym based QRegion construction in this project are derived from
patches written for remarkable2-framebuffer.

> MIT License
>
> Copyright (c) 2020 ddvk okay
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## Note on reMarkable AS intellectual property

This project contains no reMarkable code and distributes no derivatives of
the xochitl binary. It interoperates with the device through documented
runtime interfaces (dynamic linking, Qt metaobjects) and a small table of
function offsets discovered by reverse engineering for interoperability
purposes. Using it on a device may still be subject to reMarkable's end
user license agreement; that is between you and reMarkable.
