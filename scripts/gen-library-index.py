#!/usr/bin/env python3
"""Writes docs/libraries.md from the header comment of every libraries/*.xel file."""
import os, re
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
manual = {
    "algorithms": "sorting and searching helpers over int vectors.",
    "bitops": "bit counting, rotation, masks and shifts.",
    "cli": "small helpers for command-line programs.",
    "collections": "vectors, stacks and string maps (the original collection types).",
    "compression": "run-length and simple byte compression.",
    "crypto": "SHA-style hashing primitives built on bitops.",
    "fs": "files and paths over raw syscalls.",
    "json": "the original small JSON value library (see jsonx for a full parser).",
    "log": "leveled logging that emits JSON lines.",
    "math": "integer and float math functions.",
    "math_ext": "extra math functions layered on math.",
    "net": "sockets over raw syscalls.",
    "process": "spawn and wait for child processes.",
    "serde": "serialize records to and from text.",
    "stat": "file metadata (stat) over raw syscalls.",
    "string_utils": "trim, split, join, replace and friends.",
    "sys": "system information and environment.",
    "syscall": "raw Linux system calls: invoke0..invoke6.",
    "terminal": "ANSI escape sequences, cursor control and colors.",
    "test": "a tiny unit-test harness.",
    "threading": "threads, mutexes and thread-local helpers.",
    "time": "wall-clock time helpers.",
}
groups = [
 ("Containers", "intlist fltlist strlist ptrlist intmap strmap strintmap intset strset deque heap bitset ringbuffer linkedlist stack queue lru trie unionfind avl bloom collections"),
 ("Math, numerics and algorithms", "math math_ext mathx numeric random stats sort search algorithms bigint rational complex geometry matrix linalg interp hashing fft polynomial decimal units intervals bitops graph grid geohash"),
 ("Text and formats", "strings strconv string_utils bytes utf8 fmt glob textwrap casing textdist table csv ini dotenv html url template scanner regex markdown toml diff expr ngram roman"),
 ("Encoding, hashing and crypto", "base64 base32 ascii85 hex varint uuid semver color sha256 sha1 md5 hmac checksum chacha20 securerandom crypto"),
 ("Compression", "compression lzss huffman inflate rle tar"),
 ("Files and operating system", "fs path os stat dir clock time datetime duration calendar cron cstr subprocess process tempfile signals tty sysinfo mmapfile argparse cli terminal slog log progress style sys syscall inotify epoll kvstore"),
 ("Concurrency", "threading atomic futex mutex condvar waitgroup semaphore channel once parallel ratelimit retry"),
 ("Networking", "net ip tcp udp dns mime http httpserver"),
 ("Data and testing", "json jsonx serde fsm arena heapstats bench assert test"),
]
info = {}
for f in sorted(os.listdir(os.path.join(root, "libraries"))):
    if not f.endswith(".xel"): continue
    name = f[:-4]
    first = open(os.path.join(root, "libraries", f)).readline().strip()
    m = re.match(r'//\s*[\w]+:\s*(.*)', first)
    info[name] = manual.get(name) or (m.group(1) if m else name)
used = set()
out = ["# Xe library index", "",
       "%d importable libraries. Add one with `uses name` at the top of a source file." % len(info), ""]
for title, names in groups:
    out += ["## " + title, "", "| Library | What it does |", "|---|---|"]
    for n in names.split():
        if n in info and n not in used:
            used.add(n); out.append("| `%s` | %s |" % (n, info[n]))
    out.append("")
rest = [n for n in info if n not in used]
if rest:
    out += ["## Other", "", "| Library | What it does |", "|---|---|"] + ["| `%s` | %s |" % (n, info[n]) for n in rest] + [""]
open(os.path.join(root, "docs", "libraries.md"), "w").write("\n".join(out))
print("wrote docs/libraries.md (%d libraries)" % len(info))
