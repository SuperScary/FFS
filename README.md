# FFS — *For Fuck’s Sake*

A spiritual successor to [Brainfuck](https://esolangs.org/wiki/Brainfuck), designed for when you want the **raw power of direct memory fuckery** but with a little less masochism.

FFS keeps the *"one-character op"* vibe, but cleans up the parts that made you scream in BF: mismatched loops, undefined EOF, no debug tools, no repeat counts.
Now you can write esoteric masterpieces without hurling your keyboard across the room.

---

## Features

* **10 core ops** (`><+-.,[]?!`) — plus a real debug op `!`
* **Sugar** for sanity:

    * Repeat counts: `+x10`, `>x4`, `.x3`
    * Inline constants: `=42`, `=0x2A`, `=b101010`
    * Labeled loops: `[@loop … ]@loop`
    * Real comments: `# line` and `/* block */`
* **Safe memory model**:

    * Default 30k tape (`uint8_t`)
    * Clamp at edges, or use `--elastic` to grow
    * Arithmetic always wraps mod 256
* **No silent nonsense**:

    * Defined EOF → stores `255`
    * Use `?` to normalize EOF back to `0`
    * Strict mode to turn edge cases into errors
* **Debugging built-in**:

    * `!` prints current pointer and a cell slice
    * `--trace` flag dumps every executed op

---

## Core Ops

| Op  | Name           | Description             |
| --- | -------------- | ----------------------- |
| `>` | inc_ptr        | pointer++               |
| `<` | dec_ptr        | pointer--               |
| `+` | inc            | cell++ (wraps 255→0)    |
| `-` | dec            | cell-- (wraps 0→255)    |
| `.` | out            | write byte at cell      |
| `,` | in             | read byte (EOF → 255)   |
| `[` | jmp_if_zero    | jump forward if cell==0 |
| `]` | jmp_if_nonzero | jump back if cell!=0    |
| `?` | zero_if_eof    | if cell==255 → 0        |
| `!` | dbg            | dump pointer + N cells  |

---

## Sugar Examples

```ffs
+x10       # increment 10 times
> x4       # move right 4
=65        # set current cell to ASCII 'A'
[@loop     # start labeled loop
  .        # print
]@loop     # end
# comment
/* block
   comment */
```

---

## Examples

### Hello, World!

```ffs
=72 . =101 . =108 . . =111 . =32 . =87 . =111 . =114 . =108 . =100 . =33 . =10 .
```

### Echo until EOF

```ffs
, ? [ . , ? ]
```

### Decimal counter (00–99)

```ffs
# prints 00..99 each on its own line
=0 > =0 > =0 > =10 > =10 > =48 <x5
>x3 [@tens
  < =10 << =0 >> [@ones
    < +x48 . -x48
    < +x48 . -x48
    >x4 . <x4
    + >> -
  ]@ones
  < + >> -
]@tens
```

Output:

```
00
01
02
...
98
99
```

---

## Build & Run

```bash
g++ -std=c++17 -O2 -o ffs src/ffs.cpp

# Run Hello World
./ffs -f examples/hello.ffs

# Run counter with debug
./ffs --dbg 16 --trace -f examples/counter.ffs
```

---

## Flags

* `--cells N` → tape size (default 30000)
* `--elastic` → allow tape to grow rightward
* `--strict` → crash on pointer under/overflow
* `--dbg N` → number of cells shown by `!` (default 8)
* `--trace` → dump every executed op

---

## Philosophy

Brainfuck was fun, but it was built to be **pain**.
FFS is built to be **fun** — it keeps the extreme minimalism but adds just enough sugar and safety that you can actually **build cool stuff without crying**.

If BF is a punishment, FFS is a *reward*.

---

## License

FFS is released under the MIT License.
Go forth and hack — and for f*ck’s sake, have fun.