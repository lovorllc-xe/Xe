import sys

def sumsq(n):
    s = 0
    i = 0
    while i < n:
        s = s + (i * i) % 7
        i = i + 1
    return s

def collatz(n):
    total = 0
    i = 1
    while i < n:
        x = i
        while x != 1:
            if x % 2 == 0:
                x = x // 2
            else:
                x = 3 * x + 1
            total = total + 1
        i = i + 1
    return total

def sieve(n):
    s = [0] * (n + 1)
    i = 2
    while i * i <= n:
        if s[i] == 0:
            j = i * i
            while j <= n:
                s[j] = 1
                j = j + i
        i = i + 1
    c = 0
    i = 2
    while i <= n:
        if s[i] == 0:
            c = c + 1
        i = i + 1
    return c

def matmul(n):
    a = [0] * (n * n)
    b = [0] * (n * n)
    c = [0] * (n * n)
    i = 0
    while i < n * n:
        a[i] = (i * 7 + 3) % 10
        b[i] = (i * 5 + 1) % 10
        i = i + 1
    i = 0
    while i < n:
        j = 0
        while j < n:
            s = 0
            k = 0
            while k < n:
                s = s + a[i * n + k] * b[k * n + j]
                k = k + 1
            c[i * n + j] = s
            j = j + 1
        i = i + 1
    t = 0
    i = 0
    while i < n * n:
        t = (t + c[i]) % 1000003
        i = i + 1
    return t

def sort(n):
    a = [0] * (n)
    x = 12345
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345) % 2147483648
        a[i] = x % 100000
        i = i + 1
    i = 1
    while i < n:
        v = a[i]
        j = i - 1
        while j >= 0  and  a[j] > v:
            a[j + 1] = a[j]
            j = j - 1
        a[j + 1] = v
        i = i + 1
    h = 0
    i = 0
    while i < n:
        h = (h * 31 + a[i]) % 1000000007
        i = i + 1
    return h

def hashtab(n):
    cap = 16
    while cap < n * 2:
        cap = cap * 2
    keys = [0] * (cap)
    vals = [0] * (cap)
    i = 1
    while i <= n:
        k = (i * 2654435761) % 4294967296
        h = k & (cap - 1)
        while keys[h] != 0  and  keys[h] != k:
            h = (h + 1) & (cap - 1)
        keys[h] = k
        vals[h] = vals[h] + i
        i = i + 1
    total = 0
    i = 1
    while i <= n:
        k = (i * 2654435761) % 4294967296
        h = k & (cap - 1)
        while keys[h] != k:
            h = (h + 1) & (cap - 1)
        total = (total + vals[h]) % 1000000007
        i = i + 1
    return total

def editdist(n):
    a = [0] * (n)
    b = [0] * (n)
    x = 7
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345) % 2147483648
        a[i] = (x >> 8) % 4
        x = (x * 1103515245 + 12345) % 2147483648
        b[i] = (x >> 8) % 4
        i = i + 1
    prev = [0] * (n + 1)
    cur = [0] * (n + 1)
    i = 0
    while i <= n:
        prev[i] = i
        i = i + 1
    i = 1
    while i <= n:
        cur[0] = i
        j = 1
        while j <= n:
            cost = 1
            if a[i - 1] == b[j - 1]:
                cost = 0
            best = prev[j - 1] + cost
            if prev[j] + 1 < best:
                best = prev[j] + 1
            if cur[j - 1] + 1 < best:
                best = cur[j - 1] + 1
            cur[j] = best
            j = j + 1
        j = 0
        while j <= n:
            prev[j] = cur[j]
            j = j + 1
        i = i + 1
    return prev[n]

def gcd(a, b):
    x = a
    y = b
    while y != 0:
        t = x % y
        x = y
        y = t
    return x

def gcdsum(n):
    s = 0
    i = 1
    while i <= n:
        s = s + gcd(i, n - i + 1)
        i = i + 1
    return s

def lcg(n):
    x = 1
    s = 0
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
        i = i + 1
    return s

def binsearch(n):
    a = [0] * (n)
    i = 0
    while i < n:
        a[i] = i * 3
        i = i + 1
    found = 0
    i = 0
    while i < n:
        key = (i * 7) % (n * 3)
        lo = 0
        hi = n - 1
        while lo <= hi:
            mid = (lo + hi) // 2
            if a[mid] == key:
                found = found + 1
                lo = hi + 1
            else:
                if a[mid] < key:
                    lo = mid + 1
                else:
                    hi = mid - 1
        i = i + 1
    return found

def prefix(n):
    a = [0] * (n + 1)
    i = 1
    while i <= n:
        a[i] = a[i - 1] + (i * i) % 13
        i = i + 1
    q = 0
    i = 1
    while i <= n:
        lo = (i * 7) % n
        hi = lo + (i % 50)
        if hi > n:
            hi = n
        q = (q + a[hi] - a[lo]) % 1000000007
        i = i + 1
    return q

def popcnt(n):
    t = 0
    i = 0
    while i < n:
        x = i
        while x != 0:
            x = x & (x - 1)
            t = t + 1
        i = i + 1
    return t

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def queens_rec(n, row, cols, d1, d2):
    if row == n:
        return 1
    count = 0
    c = 0
    while c < n:
        b1 = 1 << c
        b2 = 1 << (row + c)
        b3 = 1 << (row - c + n)
        if (cols & b1) == 0  and  (d1 & b2) == 0  and  (d2 & b3) == 0:
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3)
        c = c + 1
    return count

def queens(n):
    return queens_rec(n, 0, 0, 0, 0)

def main():
    n = int(sys.argv[1])
    print((sumsq(n * 30000) + collatz(n * 400) + sieve(n * 15000) + matmul(n * 1) + sort(n * 40) + hashtab(n * 3000) + editdist(n * 6) + gcdsum(n * 1500) + lcg(n * 30000) + binsearch(n * 1500) + prefix(n * 30000) + popcnt(n * 3000) + fib(12) + queens(6)) % 1000000007)

main()
