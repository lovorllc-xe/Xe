# Xe library index

174 importable libraries. Add one with `uses name` at the top of a source file.

## Containers

| Library | What it does |
|---|---|
| `intlist` | a growable array of int values. |
| `fltlist` | a growable array of flt values with numeric helpers. |
| `strlist` | a growable array of strings. |
| `ptrlist` | a growable array of opaque pointers. |
| `intmap` | int -> int hash map with open addressing and automatic growth. |
| `strmap` | string -> string hash map (FNV-1a hashing, open addressing, growth). |
| `strintmap` | string -> int hash map, handy for counters and symbol tables. |
| `intset` | a set of int values built on intmap. |
| `strset` | a set of strings built on strintmap. |
| `deque` | double-ended queue of ints backed by a growable ring buffer. |
| `heap` | binary min-heap of (priority, value) int pairs; a priority queue. |
| `bitset` | a fixed-size set of bits with fast counting. |
| `ringbuffer` | a fixed-capacity FIFO of ints that overwrites the oldest value when full. |
| `linkedlist` | a doubly linked list of ints with O(1) insertion at both ends. |
| `stack` | growable LIFO stacks of ints and of strings. |
| `queue` | FIFO queues of ints and strings. |
| `lru` | a least-recently-used cache from string keys to string values. |
| `trie` | prefix tree over bytes for string sets, prefix counts and autocompletion. |
| `unionfind` | disjoint-set union with path compression and union by size. |
| `avl` | ordered int -> int map as a self-balancing AVL tree (floor/ceiling, min/max, in-order walks). |
| `bloom` | Bloom filters for fast "definitely not present / probably present" membership tests. |
| `collections` | vectors, stacks and string maps (the original collection types). |

## Math, numerics and algorithms

| Library | What it does |
|---|---|
| `math` | integer and float math functions. |
| `math_ext` | extra math functions layered on math. |
| `mathx` | double-precision math (exp, log, pow, trigonometry, hyperbolic, rounding). |
| `numeric` | integer number theory and bit tricks. |
| `random` | fast seeded pseudo-random numbers (xoshiro256**) plus OS entropy. |
| `stats` | descriptive statistics and simple regression over fltlist data. |
| `sort` | sorting algorithms for int buffers, intlist and strlist. |
| `search` | binary search helpers over sorted intlist and strlist values. |
| `algorithms` | sorting and searching helpers over int vectors. |
| `bigint` | arbitrary-precision signed integers (base 10^9 limbs). |
| `rational` | exact fractions with normalized sign and lowest terms. |
| `complex` | complex numbers with arithmetic, polar form and exponentials. |
| `geometry` | 2D vectors, distances, polygons and intersections. |
| `matrix` | dense flt matrices with multiplication, inverse, determinant and solving. |
| `linalg` | vector algebra on fltlist values. |
| `interp` | interpolation, easing curves and range mapping. |
| `hashing` | fast non-cryptographic hash functions for strings and ints. |
| `fft` | fast Fourier transform (radix-2) and convolution over fltlist real/imaginary parts. |
| `polynomial` | polynomials with flt coefficients, lowest degree first ([1, 2, 3] = 1 + 2x + 3x^2). |
| `decimal` | exact fixed-point amounts (money) stored as integer minor units, with fair splitting. |
| `units` | convert between length, mass, volume, time, speed, data-size and temperature units. |
| `intervals` | merge, intersect and query closed integer intervals stored as flat [start, end, start, end...] lists. |
| `bitops` | bit counting, rotation, masks and shifts. |
| `graph` | weighted directed graphs over nodes 0..n-1 with BFS, Dijkstra, topological sort, components. |
| `grid` | 2D integer grids with flood fill and shortest paths around walls (cells equal to wall()). |
| `geohash` | encode and decode latitude/longitude as geohash strings; great-circle distance. |

## Text and formats

| Library | What it does |
|---|---|
| `strings` | searching, replacing, trimming, padding and case conversion for byte strings. |
| `strconv` | conversions between strings and numbers in any base. |
| `string_utils` | trim, split, join, replace and friends. |
| `bytes` | growable byte buffers with little/big-endian integer access. |
| `utf8` | decoding, encoding and validating UTF-8 text. |
| `fmt` | string formatting with {} placeholders, widths and number formats. |
| `glob` | shell-style wildcard matching (* ? [abc] [a-z] [!x] and \ escapes). |
| `textwrap` | word wrapping, indentation and line utilities. |
| `casing` | convert identifiers between snake_case, kebab-case, camelCase, PascalCase and slugs. |
| `textdist` | string similarity metrics (Levenshtein, Hamming, LCS, Jaro-Winkler). |
| `table` | render rows of strings as aligned plain-text or Markdown tables. |
| `csv` | RFC 4180 CSV parsing and writing (quoted fields, embedded commas, quotes and newlines). |
| `ini` | parse INI/config files into a strmap with "section.key" keys. |
| `dotenv` | read KEY=VALUE files (.env) with comments, quotes and ${VAR} expansion. |
| `html` | escaping, unescaping, tag stripping and a small element builder. |
| `url` | parse URLs, percent-encode/decode and parse query strings. |
| `template` | {{name}} substitution, {{#if name}}...{{/if}} sections and HTML-safe output. |
| `scanner` | a cursor over a string for hand-written parsers and tokenizers. |
| `regex` | regular expressions compiled to a backtracking VM with failure memoization |
| `markdown` | convert a practical Markdown subset to HTML (headings, lists, quotes, code, emphasis, links). |
| `toml` | parse a practical TOML subset into a strmap keyed by "table.key" (strings, ints, floats, bools, arrays). |
| `diff` | line-based diffs via longest common subsequence, with unified-style output. |
| `expr` | evaluate arithmetic expressions: + - * / % ^, parentheses, unary minus, functions and variables. |
| `ngram` | character and word n-grams with Jaccard, Dice and cosine similarity. |
| `roman` | Roman numerals for 1..3999. |

## Encoding, hashing and crypto

| Library | What it does |
|---|---|
| `base64` | standard and URL-safe Base64 encoding and decoding. |
| `base32` | RFC 4648 Base32 encoding and decoding. |
| `ascii85` | Ascii85 (Base85) encoding and decoding, used by PDF and PostScript. |
| `hex` | hexadecimal encoding, decoding and dumps. |
| `varint` | LEB128 variable-length integers and zigzag encoding (protobuf compatible). |
| `uuid` | random (v4) UUID generation, validation and parsing. |
| `semver` | semantic version parsing, comparison and range checks. |
| `color` | RGB/HSV/HSL conversions, hex parsing, blending and ANSI terminal colors. |
| `sha256` | SHA-256 digests (FIPS 180-4) for strings and byte buffers. |
| `sha1` | SHA-1 digests (for legacy protocols such as git and WebSocket handshakes). |
| `md5` | MD5 digests (RFC 1321) for checksums and legacy formats; not for security. |
| `hmac` | HMAC-SHA256 message authentication and PBKDF2-HMAC-SHA256 key derivation. |
| `checksum` | CRC-32 (IEEE), CRC-32C, CRC-16/CCITT, Adler-32 and Fletcher-16. |
| `chacha20` | the ChaCha20 stream cipher (RFC 8439) over byte buffers. |
| `securerandom` | cryptographically secure random bytes and numbers from the kernel (getrandom). |
| `crypto` | SHA-style hashing primitives built on bitops. |

## Compression

| Library | What it does |
|---|---|
| `compression` | run-length and simple byte compression. |
| `lzss` | LZ77-family compression (4 KiB window, hash chains) over byte buffers. |
| `huffman` | canonical Huffman coding of byte buffers. |
| `inflate` | DEFLATE decompression (RFC 1951) with zlib (RFC 1950) and gzip (RFC 1952) wrappers. |
| `rle` | run-length encoding for bytes (binary) and text ("3a2b" style). |
| `tar` | create and read POSIX ustar archives in memory. |

## Files and operating system

| Library | What it does |
|---|---|
| `fs` | files and paths over raw syscalls. |
| `path` | POSIX path manipulation (join, split, normalize, extensions). |
| `os` | process and system information (pid, uid, cwd, hostname, environment, exit). |
| `stat` | file metadata (stat) over raw syscalls. |
| `dir` | list, walk, create and remove directories. |
| `clock` | monotonic and wall-clock time in nanoseconds, stopwatches and sleeping. |
| `time` | wall-clock time helpers. |
| `datetime` | calendar dates and times (UTC or fixed offsets), ISO 8601 formatting and parsing. |
| `duration` | parse and format durations such as "1h30m", "250ms", "2.5s" (nanosecond ints). |
| `calendar` | month grids, week numbers, Easter and business-day arithmetic. |
| `cron` | parse five-field cron expressions ("m h dom mon dow") and compute matching times (UTC). |
| `cstr` | bridges between Xe strings, raw byte pointers and C-style argument vectors. |
| `subprocess` | run programs with captured stdout/stderr and exit status (fork/pipe/execve). |
| `process` | spawn and wait for child processes. |
| `tempfile` | create unique temporary files and directories. |
| `signals` | send signals and choose default/ignore dispositions (SIGPIPE, SIGINT, ...). |
| `tty` | terminal size, raw/cooked mode, cursor control and key reading. |
| `sysinfo` | CPU count, memory, load average and uptime from the kernel and /proc. |
| `mmapfile` | map files into memory read-only, and anonymous shared memory regions. |
| `argparse` | declarative command-line parsing with flags, options, positionals and generated help. |
| `cli` | small helpers for command-line programs. |
| `terminal` | ANSI escape sequences, cursor control and colors. |
| `slog` | structured logging (logfmt or JSON lines) with levels and timestamps. |
| `log` | leveled logging that emits JSON lines. |
| `progress` | terminal progress bars and spinners. |
| `style` | ANSI text styles and 16-color terminal output (respects NO_COLOR). |
| `sys` | system information and environment. |
| `syscall` | raw Linux system calls: invoke0..invoke6. |
| `inotify` | watch files and directories for changes (create, modify, delete, move). |
| `epoll` | scalable I/O readiness notification (Linux epoll) for event loops and servers. |
| `kvstore` | a small persistent key-value store. Writes append to a log file; opening replays it. |

## Graphics, GUI and games

| Library | What it does |
|---|---|
| `canvas` | a software pixel surface with 2D drawing - lines, rectangles, circles, triangles, |
| `image` | load and save canvas pixels as PPM, BMP and PNG files (PNG is written uncompressed). |
| `png` | decode PNG files (all color types, bit depths 1-16, non-interlaced) into a canvas. |
| `jpeg` | decode baseline (sequential Huffman) JPEG files - grayscale or YCbCr with any chroma subsampling. |
| `window` | open a window (Linux X11 or Wayland, Windows, macOS: the same code on every platform), draw into its canvas, read the keyboard and mouse. |
| `x11` | a pure-Xe X11 client. It speaks the X protocol over the display's Unix socket (no libX11/xcb): |
| `gui` | an immediate-mode GUI toolkit. Widgets are plain function calls made every frame; |
| `gfx3d` | a software 3D renderer - vectors, 4x4 matrices, meshes (cube, plane, sphere, torus, OBJ files), |
| `game` | building blocks for 2D games - collision tests, tile maps, sprite sheets and animation, |
| `vkscene` | GPU-rendered 3D scenes with Vulkan, ready to use. It bundles lit vertex/fragment shaders, uploads |
| `vulkan` | Vulkan for Xe, on every platform and GPU. The Vulkan loader is opened at run time (libvulkan.so.1 on |
| `ffi` | call C functions in shared libraries and build C structs in raw memory, the same way on every platform. |

## Concurrency

| Library | What it does |
|---|---|
| `threading` | threads, mutexes and thread-local helpers. |
| `atomic` | lock-free integer operations on shared memory cells (*void pointing at 8 bytes). |
| `futex` | thin wrappers over the Linux futex system call for building blocking primitives. |
| `mutex` | blocking futex-based mutexes (uncontended lock/unlock never enter the kernel). |
| `condvar` | condition variables paired with mutex.xel mutexes. |
| `waitgroup` | wait for a collection of threads or tasks to finish (like Go's sync.WaitGroup). |
| `semaphore` | counting semaphores built on futexes. |
| `channel` | bounded, blocking, thread-safe FIFO channels of ints (Go-style send/recv/close). |
| `once` | run initialization exactly once across threads; also a simple spinlock. |
| `parallel` | split integer ranges across threads and combine results (parallel for / reduce). |
| `ratelimit` | token-bucket rate limiter on the monotonic clock. |
| `retry` | exponential backoff schedules with jitter for retrying flaky operations. |

## Networking

| Library | What it does |
|---|---|
| `net` | sockets over raw syscalls. |
| `ip` | IPv4 address parsing/formatting, CIDR ranges and address classification. |
| `tcp` | IPv4 TCP clients and servers with buffered line reading. |
| `udp` | IPv4 datagram sockets. |
| `dns` | resolve hostnames to IPv4 addresses via /etc/hosts and UDP DNS queries. |
| `mime` | content types by file extension and simple sniffing. |
| `http` | HTTP/1.1 client (GET/POST/any method, headers, chunked bodies). Plain http:// only. |
| `httpserver` | a small HTTP/1.1 server. Accept a request, inspect it, send one response. |

## Data and testing

| Library | What it does |
|---|---|
| `json` | the original small JSON value library (see jsonx for a full parser). |
| `jsonx` | a complete JSON parser, document tree, path queries, builder and serializer. |
| `serde` | serialize records to and from text. |
| `fsm` | finite state machines driven by named events. |
| `arena` | bump allocators. Allocate many small objects fast and free them all at once with reset/release. |
| `heapstats` | control and observe Xe's garbage collector and heap. |
| `bench` | measure how long code takes. Wrap the work between start() and stop(). |
| `assert` | fail-fast assertions for scripts and tests; a failure prints the message and exits with status 1. |
| `test` | a tiny unit-test harness. |

## Other

| Library | What it does |
|---|---|
| `audio` | sound for programs and games, the same on every platform. |
| `closure` | a function pointer bundled with the data it works on. |
| `func` | call a function through a pointer, the same way on every platform. |
| `result` | errors as values, and panic for errors that cannot be handled. |
| `vkwindow` | show Vulkan rendering in a `window`, the same way on every platform. |
| `wayland` | a pure-Xe Wayland client. It speaks the Wayland wire protocol over the compositor's Unix |
| `wevent` | the window event record and event kinds shared by every window backend (X11, Wayland, Win32, ...). |
