# Xe built-ins and library index

The full descriptions are in `docs/documentation.txt`, sections 13–15. The real signatures are in `libraries/<name>.xel` (installed copy: `/usr/local/lib/xelang/libraries/`). **Grep the `.xel` before calling anything not listed here.**

## Compiler built-ins (no `uses` needed)
- I/O: `write(v...)`, `writeln(v...)` (accepts mixed int, flt, string and pointer values; `writeln()` prints an empty line), `read(intOrFltVar)`, `readln(stringVar)`
- Strings: `str.length(s)`, `str.char(s,i)` (returns a byte as an int), `str.slice(s,lo,hi)`, `str.find(s,sub)` (returns -1 when absent), `str.equal(a,b)`, `str.number(int)`, `str.fromchar(byte)`, `str.builder(init)`, `str.append(b,s)` (returns the builder: `b := str.append(b, x)`)
- Floats: `flt.format(f, decimals)`, `flt.toint(f)` (truncates)
- Program context: `arg.count()` (excludes the program name), `arg.get(i)` (0 is the first user argument; returns "" when out of range), `env.get(name)`
- Files: `file.read(path)` (returns "" on failure), `file.write(path, text)`
- Processes: `process.run(cmd)`, `process.spawn`, `process.wait`
- Threads: `thread.start(&fn, arg)`, `thread.join(id)` (returns the worker's int result)
- Memory: `alloc`/`new`, `free`/`delete`, `aligned_alloc`, `alloc.aligned`, `gc.begin {} gc.end`, `defer`
- Low-level: `runtime.pointer.read/write/cast.string/free/is_null`, `runtime.mem.copy/set/compare`, `runtime.hw.read8/write8/read32/write32`, `atomic.load/store/compare_exchange`, `syscall.invoke3/invoke6`
- Constants: `true`, `false`

## std (automatic)
- `io.print.text(s)`, `io.print.line(s)`, `io.input.integer()`, `io.input.float()`, `io.input.line()`
- `mem.allocate(n)`, `mem.release(p)`

## .xel libraries (`uses name`)
| lib | key API |
|---|---|
| algorithms | `sort.integers(*IntVector)`, `find.integer(*IntVector, x)`, `sum.integers` (uses collections) |
| bitops | `shift_left/right`, `and`, `or`, `xor`, `complement`, `not` |
| cli | `count()`, `get(i)`, `command()`, `has_flag(name)`, `has_option`, `option(name, fallback)`, `option_bool` |
| collections | `Vector` (growable list of `*void`): `vector.create(cap)/push/get/get.string/free`; `Map` (string keys, **128 entries max**): `map.create/put/get/count/key_at/value_at/free`; `IntVector` and `IntStack` (**128 ints**, inline records): `int.vector.push/try.get/try.set/count`, `stack.push/try.pop/count` |
| compression | `compress_rle`, `decompress_rle`, `crc_verify` |
| crypto | `crc32` (a checksum, not real cryptography) |
| fs | `read_file`, `write_file`, `append_file`, `file_exists`, `file_size`, `remove_file`, `make_directory`, `write_all(fd, s)` |
| json / serde | `json.parse(text): *Map`, `json.get_array(text): *Vector`, `json.stringify(*Map)`, `json.stringify_array`, `json.get_raw`, `json.put_raw`, `json.quote`. Values are stored as encoded JSON strings, and numbers stay as strings. `serde.json.*` wraps the same functions. |
| log | `trace/debug/info/warn/error(msg)`, `write(level,msg)`, `write_filtered`, `write_json`, `level_*()` |
| math | `abs_int`, `abs_flt`, `pow(flt, int)`, `sqrt(flt)`, `clamp` |
| math_ext | `sin`, `cos`, `tan`, `reduce_angle`, `trunc`, `floor`, `ceil`, `random_range(min,max)` |
| net | IPv4 only: `socket`, `parse.ipv4`, `make.address`, `connect(fd, ip, port)`, `bind`, `listen`, `accept` |
| process | `exec(cmd): pid`, `wait_pid`, `get_env`, `pipe_create(*int)` |
| string_utils | `trim`, `split(s, delim): *Vector` (free it with `free.parts`), `join`, `to_int`, `is.space`, `add.part` |
| sys | `mem.copy/set/compare`, `hw.read_8/write_8/read_32/write_32`, `proc.pid/fork/exit`, `allocate_buffer`, `free_buffer` |
| syscall | `raw.invoke3/invoke6`, `io.open/read/write/write.string/close` |
| terminal | `write_text`, `set_color(ansi)`, `clear_screen`, `move_cursor(row,col)`, `read_key_raw` |
| test | `assert_true/false/int/string(name, ...)` (return 1 or 0), `summary(passed, failed)` |
| threading | `create_thread(&fn, arg)`, `join_thread`, `mutex_init/lock/unlock/free` (spin lock) |
| time | `now_milliseconds`, `now_seconds`, `sleep_ms`, `clock_ticks` (monotonic ns) |

Usage patterns:
```xe
uses collections
var v: collections.IntVector                // inline record, so pass &v
collections.int.vector.push(&v, 4)
var out: int := 0
if collections.int.vector.try.get(&v, 0, &out) = 1 { writeln(out) }

var vec: *collections.Vector := collections.vector.create(4)
var item: *string := alloc(8)
*item := "hello"
collections.vector.push(vec, item)
writeln(*collections.vector.get.string(vec, 0))
free(item)
collections.vector.free(vec)               // frees the storage, not the elements
```

Most libraries make Linux syscalls directly. There is no HTTP, TLS, DB, GUI, regex, or async support.
