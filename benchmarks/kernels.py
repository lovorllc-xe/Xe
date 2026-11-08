"""Benchmark kernels, written once and transpiled to Xe, C++, Go, Rust and Python.

Most kernels use a tiny pseudo-language so every language runs the *same*
algorithm with the same integer arithmetic:

    fn NAME(a, b)            function over 64-bit ints, returns a 64-bit int
    var x = EXPR             int local            varf x = EXPR   float local
    arr a = SIZE             zero-filled int array (freed / dropped at scope end)
    x = EXPR                 assignment           a[i] = EXPR     array store
    while COND {  ...  }     if COND {  ...  } else {  ...  }
    ret EXPR                 return
    endfn

Expressions use `DIV` for integer division and `FL(x)` to convert an int to a
float; everything else is common C-like syntax.  Kernels that need records,
heap objects, strings or threads are written natively for each language
(the `RAW` table).
"""

# name -> (pseudo source, smallest N, largest N).  The largest N is bounded so
# that the Python version still finishes in a fraction of a second.
PSEUDO = {}

def P(name, lo, hi, src):
    PSEUDO[name] = (src.strip("\n"), lo, hi)

P("sumsq", 1, 2000000, """
fn sumsq(n)
var s = 0
var i = 0
while i < n {
s = s + (i * i) % 7
i = i + 1
}
ret s
endfn
""")

P("fib", 3, 27, """
fn fib(n)
if n < 2 {
ret n
}
ret fib(n - 1) + fib(n - 2)
endfn
""")

P("collatz", 2, 25000, """
fn collatz(n)
var total = 0
var i = 1
while i < n {
var x = i
while x != 1 {
if x % 2 == 0 {
x = x DIV 2
} else {
x = 3 * x + 1
}
total = total + 1
}
i = i + 1
}
ret total
endfn
""")

P("sieve", 2, 1000000, """
fn sieve(n)
arr s = n + 1
var i = 2
while i * i <= n {
if s[i] == 0 {
var j = i * i
while j <= n {
s[j] = 1
j = j + i
}
}
i = i + 1
}
var c = 0
i = 2
while i <= n {
if s[i] == 0 {
c = c + 1
}
i = i + 1
}
ret c
endfn
""")

P("matmul", 2, 80, """
fn matmul(n)
arr a = n * n
arr b = n * n
arr c = n * n
var i = 0
while i < n * n {
a[i] = (i * 7 + 3) % 10
b[i] = (i * 5 + 1) % 10
i = i + 1
}
i = 0
while i < n {
var j = 0
while j < n {
var s = 0
var k = 0
while k < n {
s = s + a[i * n + k] * b[k * n + j]
k = k + 1
}
c[i * n + j] = s
j = j + 1
}
i = i + 1
}
var t = 0
i = 0
while i < n * n {
t = (t + c[i]) % 1000003
i = i + 1
}
ret t
endfn
""")

P("mandel", 2, 100, """
fn mandel(n)
var count = 0
var py = 0
while py < n {
var px = 0
while px < n {
varf cx = FL(px) * 3.0 / FL(n) - 2.0
varf cy = FL(py) * 2.0 / FL(n) - 1.0
varf x = 0.0
varf y = 0.0
var it = 0
var inside = 1
while it < 40 && inside == 1 {
varf t = x * x - y * y + cx
y = 2.0 * x * y + cy
x = t
if x * x + y * y > 4.0 {
inside = 0
}
it = it + 1
}
count = count + inside
px = px + 1
}
py = py + 1
}
ret count
endfn
""")

P("sort", 4, 2500, """
fn sort(n)
arr a = n
var x = 12345
var i = 0
while i < n {
x = (x * 1103515245 + 12345) % 2147483648
a[i] = x % 100000
i = i + 1
}
i = 1
while i < n {
var v = a[i]
var j = i - 1
while j >= 0 && a[j] > v {
a[j + 1] = a[j]
j = j - 1
}
a[j + 1] = v
i = i + 1
}
var h = 0
i = 0
while i < n {
h = (h * 31 + a[i]) % 1000000007
i = i + 1
}
ret h
endfn
""")

P("hashtab", 4, 200000, """
fn hashtab(n)
var cap = 16
while cap < n * 2 {
cap = cap * 2
}
arr keys = cap
arr vals = cap
var i = 1
while i <= n {
var k = (i * 2654435761) % 4294967296
var h = k & (cap - 1)
while keys[h] != 0 && keys[h] != k {
h = (h + 1) & (cap - 1)
}
keys[h] = k
vals[h] = vals[h] + i
i = i + 1
}
var total = 0
i = 1
while i <= n {
var k = (i * 2654435761) % 4294967296
var h = k & (cap - 1)
while keys[h] != k {
h = (h + 1) & (cap - 1)
}
total = (total + vals[h]) % 1000000007
i = i + 1
}
ret total
endfn
""")

P("gcdsum", 2, 100000, """
fn gcd(a, b)
var x = a
var y = b
while y != 0 {
var t = x % y
x = y
y = t
}
ret x
endfn
fn gcdsum(n)
var s = 0
var i = 1
while i <= n {
s = s + gcd(i, n - i + 1)
i = i + 1
}
ret s
endfn
""")

P("lcg", 1, 2000000, """
fn lcg(n)
var x = 1
var s = 0
var i = 0
while i < n {
x = (x * 1103515245 + 12345) % 2147483648
s = (s + (x >> 16)) % 1000000007
i = i + 1
}
ret s
endfn
""")

P("binsearch", 2, 100000, """
fn binsearch(n)
arr a = n
var i = 0
while i < n {
a[i] = i * 3
i = i + 1
}
var found = 0
i = 0
while i < n {
var key = (i * 7) % (n * 3)
var lo = 0
var hi = n - 1
while lo <= hi {
var mid = (lo + hi) DIV 2
if a[mid] == key {
found = found + 1
lo = hi + 1
} else {
if a[mid] < key {
lo = mid + 1
} else {
hi = mid - 1
}
}
}
i = i + 1
}
ret found
endfn
""")

P("prefix", 2, 2000000, """
fn prefix(n)
arr a = n + 1
var i = 1
while i <= n {
a[i] = a[i - 1] + (i * i) % 13
i = i + 1
}
var q = 0
i = 1
while i <= n {
var lo = (i * 7) % n
var hi = lo + (i % 50)
if hi > n {
hi = n
}
q = (q + a[hi] - a[lo]) % 1000000007
i = i + 1
}
ret q
endfn
""")

P("popcnt", 2, 200000, """
fn popcnt(n)
var t = 0
var i = 0
while i < n {
var x = i
while x != 0 {
x = x & (x - 1)
t = t + 1
}
i = i + 1
}
ret t
endfn
""")

P("editdist", 2, 400, """
fn editdist(n)
arr a = n
arr b = n
var x = 7
var i = 0
while i < n {
x = (x * 1103515245 + 12345) % 2147483648
a[i] = (x >> 8) % 4
x = (x * 1103515245 + 12345) % 2147483648
b[i] = (x >> 8) % 4
i = i + 1
}
arr prev = n + 1
arr cur = n + 1
i = 0
while i <= n {
prev[i] = i
i = i + 1
}
i = 1
while i <= n {
cur[0] = i
var j = 1
while j <= n {
var cost = 1
if a[i - 1] == b[j - 1] {
cost = 0
}
var best = prev[j - 1] + cost
if prev[j] + 1 < best {
best = prev[j] + 1
}
if cur[j - 1] + 1 < best {
best = cur[j - 1] + 1
}
cur[j] = best
j = j + 1
}
j = 0
while j <= n {
prev[j] = cur[j]
j = j + 1
}
i = i + 1
}
ret prev[n]
endfn
""")

P("queens", 4, 9, """
fn queens_rec(n, row, cols, d1, d2)
if row == n {
ret 1
}
var count = 0
var c = 0
while c < n {
var b1 = 1 << c
var b2 = 1 << (row + c)
var b3 = 1 << (row - c + n)
if (cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0 {
count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3)
}
c = c + 1
}
ret count
endfn
fn queens(n)
ret queens_rec(n, 0, 0, 0, 0)
endfn
""")

# Units used when several kernels are combined into one monolithic program.
MONO = ["sumsq", "collatz", "sieve", "matmul", "sort", "hashtab", "editdist",
        "gcdsum", "lcg", "binsearch", "prefix", "popcnt", "fib", "queens"]
MONO_UNIT = {"sumsq": 30000, "collatz": 400, "sieve": 15000, "matmul": 1, "sort": 40,
             "hashtab": 3000, "editdist": 6, "gcdsum": 1500, "lcg": 30000,
             "binsearch": 1500, "prefix": 30000, "popcnt": 3000, "fib": 0, "queens": 0}
MONO_FIXED = {"fib": 12, "queens": 6}   # these two ignore the scale
MONO_SCALES = [1, 3, 10, 30]

# ---------------------------------------------------------------- transpiler
import re

def _match_close(text, start):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '[': depth += 1
        elif text[i] == ']':
            depth -= 1
            if depth == 0: return i
    raise ValueError("unbalanced [ in " + text)

def _wrap_index(text, arrays, lang):
    for name in arrays:
        out, pos = "", 0
        pattern = re.compile(r'(?<![A-Za-z0-9_])' + name + r'\[')
        while True:
            m = pattern.search(text, pos)
            if not m: out += text[pos:]; break
            open_at = m.end() - 1
            close = _match_close(text, open_at)
            inner = _wrap_index(text[open_at + 1:close], arrays, lang)
            out += text[pos:m.start()]
            if lang == "xe": out += "(*%s)[%s]" % (name, inner)
            elif lang == "rs": out += "%s[(%s) as usize]" % (name, inner)
            else: out += "%s[%s]" % (name, inner)
            pos = close + 1
        text = out
    return text

def _fl(text, lang):
    out, pos = "", 0
    while True:
        i = text.find("FL(", pos)
        if i < 0: return out + text[pos:]
        j, depth = i + 2, 0
        for k in range(i + 2, len(text)):
            if text[k] == '(': depth += 1
            elif text[k] == ')':
                depth -= 1
                if depth == 0: j = k; break
        inner = _fl(text[i + 3:j], lang)
        wrapped = {"xe": "(%s * 1.0)", "cpp": "(double)(%s)", "go": "float64(%s)",
                   "rs": "(%s) as f64", "py": "float(%s)"}[lang] % inner
        out += text[pos:i] + wrapped
        pos = j + 1

def expr(text, arrays, lang):
    text = _wrap_index(text, arrays, lang)
    text = _fl(text, lang)
    text = text.replace(" DIV ", " // " if lang == "py" else " / ")
    if lang == "xe": text = re.sub(r'(?<![!<>=:])==', '=', text)
    if lang == "py": text = text.replace("&&", " and ").replace("||", " or ")
    return text

def parse_pseudo(src):
    funcs, cur = [], None
    for raw in src.split("\n"):
        line = raw.strip()
        if not line: continue
        if line.startswith("fn "):
            m = re.match(r'fn (\w+)\((.*)\)', line)
            cur = {"name": m.group(1), "params": [p.strip() for p in m.group(2).split(",") if p.strip()], "body": []}
        elif line == "endfn": funcs.append(cur); cur = None
        else: cur["body"].append(line)
    return funcs

def emit_function(fn, lang):
    arrays = []
    for line in fn["body"]:
        m = re.match(r'arr (\w+) =', line)
        if m: arrays.append(m.group(1))
    p = fn["params"]; name = fn["name"]
    sig = {
        "xe": "fn %s(%s): int\nbegin" % (name, ", ".join(x + ": int" for x in p)),
        "cpp": "static i64 %s(%s) {" % (name, ", ".join("i64 " + x for x in p)),
        "go": "func %s(%s) int64 {" % (name, ", ".join(x + " int64" for x in p)),
        "rs": "fn %s(%s) -> i64 {" % (name, ", ".join(x + ": i64" for x in p)),
        "py": "def %s(%s):" % (name, ", ".join(p)),
    }[lang]
    out, depth = [sig], 1
    ind = lambda: "    " * depth
    for line in fn["body"]:
        if line == "}":
            depth -= 1
            if lang in ("cpp", "go", "rs", "xe"): out.append(ind() + "}")
            continue
        if line == "} else {":
            depth -= 1
            out.append(ind() + {"py": "else:"}.get(lang, "} else {"))
            depth += 1
            continue
        opens = line.endswith("{")
        m = re.match(r'(while|if) (.*) \{$', line)
        if m:
            kw, cond = m.groups()
            cond = expr(cond, arrays, lang)
            if lang == "py": s = "%s %s:" % (kw, cond)
            elif lang == "xe": s = "%s %s {" % (kw, cond)
            elif lang == "go": s = ("for %s {" if kw == "while" else "if %s {") % cond
            else: s = "%s (%s) {" % (kw, cond) if lang == "cpp" else "%s %s {" % (kw, cond)
            out.append(ind() + s); depth += 1; continue
        m = re.match(r'(var|varf) (\w+) = (.*)$', line)
        if m:
            kind, v, e = m.groups(); e = expr(e, arrays, lang)
            f = kind == "varf"
            out.append(ind() + {
                "xe": "var %s: %s := %s" % (v, "flt" if f else "int", e),
                "cpp": "%s %s = %s;" % ("double" if f else "i64", v, e),
                "go": "var %s %s = %s" % (v, "float64" if f else "int64", e),
                "rs": "let mut %s: %s = %s;" % (v, "f64" if f else "i64", e),
                "py": "%s = %s" % (v, e)}[lang])
            continue
        m = re.match(r'arr (\w+) = (.*)$', line)
        if m:
            v, size = m.groups(); size = expr(size, arrays, lang)
            if lang == "xe":
                out.append(ind() + "var %s: *[268435456]int := alloc((%s) * 8)" % (v, size))
                out.append(ind() + "defer free(%s)" % v)
                out.append(ind() + "var zero_%s: int := 0" % v)
                out.append(ind() + "while zero_%s < %s {" % (v, size))
                out.append(ind() + "    (*%s)[zero_%s] := 0" % (v, v))
                out.append(ind() + "    zero_%s := zero_%s + 1" % (v, v))
                out.append(ind() + "}")
            else:
                out.append(ind() + {
                    "cpp": "std::vector<i64> %s(%s);" % (v, size),
                    "go": "%s := make([]int64, %s)" % (v, size),
                    "rs": "let mut %s: Vec<i64> = vec![0i64; (%s) as usize];" % (v, size),
                    "py": "%s = [0] * (%s)" % (v, size)}[lang])
            continue
        m = re.match(r'ret (.*)$', line)
        if m:
            e = expr(m.group(1), arrays, lang)
            out.append(ind() + ("return %s;" if lang in ("cpp", "rs") else "return %s") % e)
            continue
        m = re.match(r'([^=]+?) = (.*)$', line)
        if m:
            lhs, e = m.groups()
            lhs, e = expr(lhs, arrays, lang), expr(e, arrays, lang)
            out.append(ind() + {"xe": "%s := %s", "cpp": "%s = %s;", "go": "%s = %s", "rs": "%s = %s;", "py": "%s = %s"}[lang] % (lhs, e))
            continue
        raise ValueError("cannot parse: " + line)
    if lang == "xe": out.append("end")
    elif lang != "py": out.append("}")
    return "\n".join(out)

def pseudo_functions(name, lang):
    return "\n\n".join(emit_function(f, lang) for f in parse_pseudo(PSEUDO[name][0]))

# ------------------------------------------------------------ native kernels
RAW = {}

def R(name, lo, hi, xe, cpp, go, rs, py, threaded=False, imports=None):
    RAW[name] = dict(lo=lo, hi=hi, xe=xe.strip("\n"), cpp=cpp.strip("\n"), go=go.strip("\n"),
                     rs=rs.strip("\n"), py=py.strip("\n"), threaded=threaded, imports=imports or {})

R("strbuild", 1, 500000,
xe="""
fn strbuild(n: int): int
begin
    var s: string := str.builder("")
    var i: int := 0
    while i < n {
        s := str.append(s, str.fromchar(97 + i % 26))
        i := i + 1
    }
    var c: int := 0
    i := 0
    while i < n {
        if str.char(s, i) = 97 { c := c + 1 }
        i := i + 1
    }
    return c
end""",
cpp="""
static i64 strbuild(i64 n) {
    std::string s;
    for (i64 i = 0; i < n; i++) s.push_back((char)(97 + i % 26));
    i64 c = 0;
    for (i64 i = 0; i < n; i++) if (s[i] == 97) c++;
    return c;
}""",
go="""
func strbuild(n int64) int64 {
    var b strings.Builder
    for i := int64(0); i < n; i++ {
        b.WriteByte(byte(97 + i%26))
    }
    s := b.String()
    var c int64
    for i := int64(0); i < n; i++ {
        if s[i] == 97 {
            c++
        }
    }
    return c
}""",
rs="""
fn strbuild(n: i64) -> i64 {
    let mut s = String::new();
    for i in 0..n {
        s.push((97 + (i % 26) as u8) as char);
    }
    let mut c = 0;
    for b in s.bytes() {
        if b == 97 { c += 1; }
    }
    c
}""",
py="""
def strbuild(n):
    parts = []
    for i in range(n):
        parts.append(chr(97 + i % 26))
    s = "".join(parts)
    c = 0
    for ch in s:
        if ch == "a":
            c += 1
    return c""", imports={"go": ["strings"]})

R("listalloc", 1, 200000,
xe="""
type Node: record
begin
    value: int
    next: *Node
end

fn listalloc(n: int): int
begin
    var head: *Node
    var i: int := 0
    while i < n {
        var node: *Node := alloc(16)
        node.value := i
        node.next := head
        head := node
        i := i + 1
    }
    var total: int := 0
    var cur: *Node := head
    while @cur != 0 {
        total := (total + cur.value) % 1000000007
        var following: *Node := cur.next
        free(cur)
        cur := following
    }
    return total
end""",
cpp="""
struct Node { i64 value; Node* next; };
static i64 listalloc(i64 n) {
    Node* head = nullptr;
    for (i64 i = 0; i < n; i++) head = new Node{i, head};
    i64 total = 0;
    while (head) { total = (total + head->value) % 1000000007; Node* nx = head->next; delete head; head = nx; }
    return total;
}""",
go="""
type Node struct {
    value int64
    next  *Node
}

func listalloc(n int64) int64 {
    var head *Node
    for i := int64(0); i < n; i++ {
        head = &Node{i, head}
    }
    var total int64
    for cur := head; cur != nil; cur = cur.next {
        total = (total + cur.value) % 1000000007
    }
    return total
}""",
rs="""
struct Node { value: i64, next: Option<Box<Node>> }
fn listalloc(n: i64) -> i64 {
    let mut head: Option<Box<Node>> = None;
    for i in 0..n { head = Some(Box::new(Node { value: i, next: head.take() })); }
    let mut total = 0;
    let mut cur = head;
    while let Some(mut node) = cur {
        total = (total + node.value) % 1000000007;
        cur = node.next.take();
    }
    total
}""",
py="""
class Node:
    __slots__ = ("value", "next")
    def __init__(self, value, nxt):
        self.value = value
        self.next = nxt

def listalloc(n):
    head = None
    for i in range(n):
        head = Node(i, head)
    total = 0
    cur = head
    while cur is not None:
        total = (total + cur.value) % 1000000007
        cur = cur.next
    return total""")

R("churn", 1, 300000,
xe="""
type Cell: record
begin
    value: int
    other: int
end

fn churn(n: int): int
begin
    var slots: [64]*Cell
    var total: int := 0
    var i: int := 0
    while i < n {
        var cell: *Cell := alloc(16)
        cell.value := i
        var k: int := i % 64
        var old: *Cell := slots[k]
        if @old != 0 {
            total := (total + old.value) % 1000000007
            free(old)
        }
        slots[k] := cell
        i := i + 1
    }
    i := 0
    while i < 64 {
        var old: *Cell := slots[i]
        if @old != 0 {
            total := (total + old.value) % 1000000007
            free(old)
        }
        i := i + 1
    }
    return total
end""",
cpp="""
struct Cell { i64 value; i64 other; };
static i64 churn(i64 n) {
    Cell* slots[64] = {};
    i64 total = 0;
    for (i64 i = 0; i < n; i++) {
        Cell* cell = new Cell{i, 0};
        i64 k = i % 64;
        if (slots[k]) { total = (total + slots[k]->value) % 1000000007; delete slots[k]; }
        slots[k] = cell;
    }
    for (int i = 0; i < 64; i++) if (slots[i]) { total = (total + slots[i]->value) % 1000000007; delete slots[i]; }
    return total;
}""",
go="""
type Cell struct {
    value int64
    other int64
}

func churn(n int64) int64 {
    var slots [64]*Cell
    var total int64
    for i := int64(0); i < n; i++ {
        cell := &Cell{i, 0}
        k := i % 64
        if slots[k] != nil {
            total = (total + slots[k].value) % 1000000007
        }
        slots[k] = cell
    }
    for i := 0; i < 64; i++ {
        if slots[i] != nil {
            total = (total + slots[i].value) % 1000000007
        }
    }
    return total
}""",
rs="""
struct Cell { value: i64, _other: i64 }
fn churn(n: i64) -> i64 {
    let mut slots: Vec<Option<Box<Cell>>> = (0..64).map(|_| None).collect();
    let mut total = 0;
    for i in 0..n {
        let cell = Box::new(Cell { value: i, _other: 0 });
        let k = (i % 64) as usize;
        if let Some(old) = slots[k].take() { total = (total + old.value) % 1000000007; }
        slots[k] = Some(cell);
    }
    for slot in slots.iter_mut() {
        if let Some(old) = slot.take() { total = (total + old.value) % 1000000007; }
    }
    total
}""",
py="""
class Cell:
    __slots__ = ("value", "other")
    def __init__(self, value, other):
        self.value = value
        self.other = other

def churn(n):
    slots = [None] * 64
    total = 0
    for i in range(n):
        cell = Cell(i, 0)
        k = i % 64
        old = slots[k]
        if old is not None:
            total = (total + old.value) % 1000000007
        slots[k] = cell
    for old in slots:
        if old is not None:
            total = (total + old.value) % 1000000007
    return total""")

R("treebuild", 1, 15,
xe="""
type Tree: record
begin
    left: *Tree
    right: *Tree
end

fn treemake(depth: int): *Tree
begin
    var node: *Tree := alloc(16)
    var none: *Tree
    node.left := none
    node.right := none
    if depth > 0 {
        node.left := treemake(depth - 1)
        node.right := treemake(depth - 1)
    }
    return node
end

fn treecount(node: *Tree): int
begin
    var left: *Tree := node.left
    if @left = 0 { return 1 }
    return 1 + treecount(node.left) + treecount(node.right)
end

fn treefree(node: *Tree)
begin
    var left: *Tree := node.left
    if @left != 0 {
        treefree(node.left)
        treefree(node.right)
    }
    free(node)
end

fn treebuild(n: int): int
begin
    var total: int := 0
    var round: int := 0
    while round < 4 {
        var root: *Tree := treemake(n)
        total := total + treecount(root)
        treefree(root)
        round := round + 1
    }
    return total
end""",
cpp="""
struct Tree { Tree* left; Tree* right; };
static Tree* treemake(i64 depth) {
    Tree* t = new Tree{nullptr, nullptr};
    if (depth > 0) { t->left = treemake(depth - 1); t->right = treemake(depth - 1); }
    return t;
}
static i64 treecount(Tree* t) { return t->left ? 1 + treecount(t->left) + treecount(t->right) : 1; }
static void treefree(Tree* t) { if (t->left) { treefree(t->left); treefree(t->right); } delete t; }
static i64 treebuild(i64 n) {
    i64 total = 0;
    for (int r = 0; r < 4; r++) { Tree* root = treemake(n); total += treecount(root); treefree(root); }
    return total;
}""",
go="""
type Tree struct {
    left  *Tree
    right *Tree
}

func treemake(depth int64) *Tree {
    t := &Tree{}
    if depth > 0 {
        t.left = treemake(depth - 1)
        t.right = treemake(depth - 1)
    }
    return t
}

func treecount(t *Tree) int64 {
    if t.left == nil {
        return 1
    }
    return 1 + treecount(t.left) + treecount(t.right)
}

func treebuild(n int64) int64 {
    var total int64
    for r := 0; r < 4; r++ {
        total += treecount(treemake(n))
    }
    return total
}""",
rs="""
struct Tree { left: Option<Box<Tree>>, right: Option<Box<Tree>> }
fn treemake(depth: i64) -> Box<Tree> {
    if depth > 0 { Box::new(Tree { left: Some(treemake(depth - 1)), right: Some(treemake(depth - 1)) }) }
    else { Box::new(Tree { left: None, right: None }) }
}
fn treecount(t: &Tree) -> i64 {
    match (&t.left, &t.right) {
        (Some(l), Some(r)) => 1 + treecount(l) + treecount(r),
        _ => 1,
    }
}
fn treebuild(n: i64) -> i64 {
    let mut total = 0;
    for _ in 0..4 { total += treecount(&treemake(n)); }
    total
}""",
py="""
def treemake(depth):
    if depth > 0:
        return (treemake(depth - 1), treemake(depth - 1))
    return None

def treecount(t):
    if t is None:
        return 1
    return 1 + treecount(t[0]) + treecount(t[1])

def treebuild(n):
    total = 0
    for _ in range(4):
        total += treecount(treemake(n))
    return total""")

# ---- multi-threaded kernels -------------------------------------------------
def threaded(name, hi, workers, body):
    """body: dict with per-language worker bodies computing `s` for the range [lo, hi)."""
    T = workers
    xe = """
type Task: record
begin
    lo: int
    hi: int
    out: int
end

fn %(n)s_worker(arg: *void): int
begin
    var task: *Task := arg
    var s: int := 0
%(xe)s
    task.out := s
    return 0
end

fn %(n)s(n: int): int
begin
    var tasks: [%(T)d]*Task
    var ids: [%(T)d]int
    var t: int := 0
    while t < %(T)d {
        var task: *Task := alloc(24)
        task.lo := n * t / %(T)d
        task.hi := n * (t + 1) / %(T)d
        task.out := 0
        tasks[t] := task
        ids[t] := thread.start(&%(n)s_worker, task)
        t := t + 1
    }
    var total: int := 0
    t := 0
    while t < %(T)d {
        thread.join(ids[t])
        var task: *Task := tasks[t]
        total := (total + task.out) %% 1000000007
        free(task)
        t := t + 1
    }
    return total
end""" % dict(n=name, T=T, xe=body["xe"])
    cpp = """
static void %(n)s_worker(i64 lo, i64 hi, i64* out) {
    i64 s = 0;
%(cpp)s
    *out = s;
}
static i64 %(n)s(i64 n) {
    std::thread th[%(T)d]; i64 out[%(T)d];
    for (int t = 0; t < %(T)d; t++) th[t] = std::thread(%(n)s_worker, n * t / %(T)d, n * (t + 1) / %(T)d, &out[t]);
    i64 total = 0;
    for (int t = 0; t < %(T)d; t++) { th[t].join(); total = (total + out[t]) %% 1000000007; }
    return total;
}""" % dict(n=name, T=T, cpp=body["cpp"])
    go = """
func %(n)s_worker(lo, hi int64, out *int64, wg *sync.WaitGroup) {
    defer wg.Done()
    var s int64
%(go)s
    *out = s
}

func %(n)s(n int64) int64 {
    var wg sync.WaitGroup
    var out [%(T)d]int64
    for t := int64(0); t < %(T)d; t++ {
        wg.Add(1)
        go %(n)s_worker(n*t/%(T)d, n*(t+1)/%(T)d, &out[t], &wg)
    }
    wg.Wait()
    var total int64
    for t := 0; t < %(T)d; t++ {
        total = (total + out[t]) %% 1000000007
    }
    return total
}""" % dict(n=name, T=T, go=body["go"])
    rs = """
fn %(n)s_worker(lo: i64, hi: i64) -> i64 {
    let mut s: i64 = 0;
%(rs)s
    s
}
fn %(n)s(n: i64) -> i64 {
    let mut handles = Vec::new();
    for t in 0..%(T)d as i64 {
        let lo = n * t / %(T)d; let hi = n * (t + 1) / %(T)d;
        handles.push(std::thread::spawn(move || %(n)s_worker(lo, hi)));
    }
    let mut total = 0;
    for h in handles { total = (total + h.join().unwrap()) %% 1000000007; }
    total
}""" % dict(n=name, T=T, rs=body["rs"])
    py = """
def %(n)s_worker(lo, hi, out, slot):
    s = 0
%(py)s
    out[slot] = s

def %(n)s(n):
    out = [0] * %(T)d
    threads = []
    for t in range(%(T)d):
        th = threading.Thread(target=%(n)s_worker, args=(n * t // %(T)d, n * (t + 1) // %(T)d, out, t))
        th.start()
        threads.append(th)
    for th in threads:
        th.join()
    total = 0
    for v in out:
        total = (total + v) %% 1000000007
    return total""" % dict(n=name, T=T, py=body["py"])
    R(name, 1, hi, xe, cpp, go, rs, py, threaded=True,
      imports={"cpp": ["thread"], "go": ["sync"], "py": ["threading"]})

def ind(text, n): return "\n".join(" " * n + l for l in text.strip("\n").split("\n"))

threaded("tsum", 2000000, 4, {
    "xe": ind("""
var i: int := task.lo
while i < task.hi {
    s := s + (i * i) % 7
    i := i + 1
}""", 4),
    "cpp": ind("for (i64 i = lo; i < hi; i++) s += (i * i) % 7;", 4),
    "go": ind("for i := lo; i < hi; i++ {\n    s += (i * i) % 7\n}", 4),
    "rs": ind("for i in lo..hi { s += (i * i) % 7; }", 4),
    "py": ind("for i in range(lo, hi):\n    s += (i * i) % 7", 4)})

threaded("tprime", 40000, 8, {
    "xe": ind("""
var x: int := task.lo
while x < task.hi {
    if x >= 2 {
        var d: int := 2
        var prime: int := 1
        while d * d <= x && prime = 1 {
            if x % d = 0 { prime := 0 }
            d := d + 1
        }
        s := s + prime
    }
    x := x + 1
}""", 4),
    "cpp": ind("""
for (i64 x = lo; x < hi; x++) {
    if (x >= 2) {
        i64 prime = 1;
        for (i64 d = 2; d * d <= x && prime; d++) if (x % d == 0) prime = 0;
        s += prime;
    }
}""", 4),
    "go": ind("""
for x := lo; x < hi; x++ {
    if x >= 2 {
        prime := int64(1)
        for d := int64(2); d*d <= x && prime == 1; d++ {
            if x%d == 0 {
                prime = 0
            }
        }
        s += prime
    }
}""", 4),
    "rs": ind("""
for x in lo..hi {
    if x >= 2 {
        let mut prime = 1;
        let mut d = 2;
        while d * d <= x && prime == 1 {
            if x % d == 0 { prime = 0; }
            d += 1;
        }
        s += prime;
    }
}""", 4),
    "py": ind("""
for x in range(lo, hi):
    if x >= 2:
        prime = 1
        d = 2
        while d * d <= x and prime == 1:
            if x % d == 0:
                prime = 0
            d += 1
        s += prime""", 4)})

threaded("tlcg", 2000000, 4, {
    "xe": ind("""
var x: int := task.lo % 1000 + 1
var i: int := task.lo
while i < task.hi {
    x := (x * 1103515245 + 12345) % 2147483648
    s := (s + (x >> 16)) % 1000000007
    i := i + 1
}""", 4),
    "cpp": ind("""
i64 x = lo % 1000 + 1;
for (i64 i = lo; i < hi; i++) {
    x = (x * 1103515245 + 12345) % 2147483648;
    s = (s + (x >> 16)) % 1000000007;
}""", 4),
    "go": ind("""
x := lo%1000 + 1
for i := lo; i < hi; i++ {
    x = (x*1103515245 + 12345) % 2147483648
    s = (s + (x >> 16)) % 1000000007
}""", 4),
    "rs": ind("""
let mut x: i64 = lo % 1000 + 1;
for _ in lo..hi {
    x = (x * 1103515245 + 12345) % 2147483648;
    s = (s + (x >> 16)) % 1000000007;
}""", 4),
    "py": ind("""
x = lo % 1000 + 1
for i in range(lo, hi):
    x = (x * 1103515245 + 12345) % 2147483648
    s = (s + (x >> 16)) % 1000000007""", 4)})
