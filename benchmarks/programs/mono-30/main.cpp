#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>
typedef long long i64;

static i64 sumsq(i64 n) {
    i64 s = 0;
    i64 i = 0;
    while (i < n) {
        s = s + (i * i) % 7;
        i = i + 1;
    }
    return s;
}

static i64 collatz(i64 n) {
    i64 total = 0;
    i64 i = 1;
    while (i < n) {
        i64 x = i;
        while (x != 1) {
            if (x % 2 == 0) {
                x = x / 2;
            } else {
                x = 3 * x + 1;
            }
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

static i64 sieve(i64 n) {
    std::vector<i64> s(n + 1);
    i64 i = 2;
    while (i * i <= n) {
        if (s[i] == 0) {
            i64 j = i * i;
            while (j <= n) {
                s[j] = 1;
                j = j + i;
            }
        }
        i = i + 1;
    }
    i64 c = 0;
    i = 2;
    while (i <= n) {
        if (s[i] == 0) {
            c = c + 1;
        }
        i = i + 1;
    }
    return c;
}

static i64 matmul(i64 n) {
    std::vector<i64> a(n * n);
    std::vector<i64> b(n * n);
    std::vector<i64> c(n * n);
    i64 i = 0;
    while (i < n * n) {
        a[i] = (i * 7 + 3) % 10;
        b[i] = (i * 5 + 1) % 10;
        i = i + 1;
    }
    i = 0;
    while (i < n) {
        i64 j = 0;
        while (j < n) {
            i64 s = 0;
            i64 k = 0;
            while (k < n) {
                s = s + a[i * n + k] * b[k * n + j];
                k = k + 1;
            }
            c[i * n + j] = s;
            j = j + 1;
        }
        i = i + 1;
    }
    i64 t = 0;
    i = 0;
    while (i < n * n) {
        t = (t + c[i]) % 1000003;
        i = i + 1;
    }
    return t;
}

static i64 sort(i64 n) {
    std::vector<i64> a(n);
    i64 x = 12345;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[i] = x % 100000;
        i = i + 1;
    }
    i = 1;
    while (i < n) {
        i64 v = a[i];
        i64 j = i - 1;
        while (j >= 0 && a[j] > v) {
            a[j + 1] = a[j];
            j = j - 1;
        }
        a[j + 1] = v;
        i = i + 1;
    }
    i64 h = 0;
    i = 0;
    while (i < n) {
        h = (h * 31 + a[i]) % 1000000007;
        i = i + 1;
    }
    return h;
}

static i64 hashtab(i64 n) {
    i64 cap = 16;
    while (cap < n * 2) {
        cap = cap * 2;
    }
    std::vector<i64> keys(cap);
    std::vector<i64> vals(cap);
    i64 i = 1;
    while (i <= n) {
        i64 k = (i * 2654435761) % 4294967296;
        i64 h = k & (cap - 1);
        while (keys[h] != 0 && keys[h] != k) {
            h = (h + 1) & (cap - 1);
        }
        keys[h] = k;
        vals[h] = vals[h] + i;
        i = i + 1;
    }
    i64 total = 0;
    i = 1;
    while (i <= n) {
        i64 k = (i * 2654435761) % 4294967296;
        i64 h = k & (cap - 1);
        while (keys[h] != k) {
            h = (h + 1) & (cap - 1);
        }
        total = (total + vals[h]) % 1000000007;
        i = i + 1;
    }
    return total;
}

static i64 editdist(i64 n) {
    std::vector<i64> a(n);
    std::vector<i64> b(n);
    i64 x = 7;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        a[i] = (x >> 8) % 4;
        x = (x * 1103515245 + 12345) % 2147483648;
        b[i] = (x >> 8) % 4;
        i = i + 1;
    }
    std::vector<i64> prev(n + 1);
    std::vector<i64> cur(n + 1);
    i = 0;
    while (i <= n) {
        prev[i] = i;
        i = i + 1;
    }
    i = 1;
    while (i <= n) {
        cur[0] = i;
        i64 j = 1;
        while (j <= n) {
            i64 cost = 1;
            if (a[i - 1] == b[j - 1]) {
                cost = 0;
            }
            i64 best = prev[j - 1] + cost;
            if (prev[j] + 1 < best) {
                best = prev[j] + 1;
            }
            if (cur[j - 1] + 1 < best) {
                best = cur[j - 1] + 1;
            }
            cur[j] = best;
            j = j + 1;
        }
        j = 0;
        while (j <= n) {
            prev[j] = cur[j];
            j = j + 1;
        }
        i = i + 1;
    }
    return prev[n];
}

static i64 gcd(i64 a, i64 b) {
    i64 x = a;
    i64 y = b;
    while (y != 0) {
        i64 t = x % y;
        x = y;
        y = t;
    }
    return x;
}

static i64 gcdsum(i64 n) {
    i64 s = 0;
    i64 i = 1;
    while (i <= n) {
        s = s + gcd(i, n - i + 1);
        i = i + 1;
    }
    return s;
}

static i64 lcg(i64 n) {
    i64 x = 1;
    i64 s = 0;
    i64 i = 0;
    while (i < n) {
        x = (x * 1103515245 + 12345) % 2147483648;
        s = (s + (x >> 16)) % 1000000007;
        i = i + 1;
    }
    return s;
}

static i64 binsearch(i64 n) {
    std::vector<i64> a(n);
    i64 i = 0;
    while (i < n) {
        a[i] = i * 3;
        i = i + 1;
    }
    i64 found = 0;
    i = 0;
    while (i < n) {
        i64 key = (i * 7) % (n * 3);
        i64 lo = 0;
        i64 hi = n - 1;
        while (lo <= hi) {
            i64 mid = (lo + hi) / 2;
            if (a[mid] == key) {
                found = found + 1;
                lo = hi + 1;
            } else {
                if (a[mid] < key) {
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
        }
        i = i + 1;
    }
    return found;
}

static i64 prefix(i64 n) {
    std::vector<i64> a(n + 1);
    i64 i = 1;
    while (i <= n) {
        a[i] = a[i - 1] + (i * i) % 13;
        i = i + 1;
    }
    i64 q = 0;
    i = 1;
    while (i <= n) {
        i64 lo = (i * 7) % n;
        i64 hi = lo + (i % 50);
        if (hi > n) {
            hi = n;
        }
        q = (q + a[hi] - a[lo]) % 1000000007;
        i = i + 1;
    }
    return q;
}

static i64 popcnt(i64 n) {
    i64 t = 0;
    i64 i = 0;
    while (i < n) {
        i64 x = i;
        while (x != 0) {
            x = x & (x - 1);
            t = t + 1;
        }
        i = i + 1;
    }
    return t;
}

static i64 fib(i64 n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

static i64 queens_rec(i64 n, i64 row, i64 cols, i64 d1, i64 d2) {
    if (row == n) {
        return 1;
    }
    i64 count = 0;
    i64 c = 0;
    while (c < n) {
        i64 b1 = 1 << c;
        i64 b2 = 1 << (row + c);
        i64 b3 = 1 << (row - c + n);
        if ((cols & b1) == 0 && (d1 & b2) == 0 && (d2 & b3) == 0) {
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3);
        }
        c = c + 1;
    }
    return count;
}

static i64 queens(i64 n) {
    return queens_rec(n, 0, 0, 0, 0);
}

int main(int argc, char** argv) {
    i64 n = atoll(argv[1]);
    printf("%lld\n", (long long)((sumsq(n * 30000) + collatz(n * 400) + sieve(n * 15000) + matmul(n * 1) + sort(n * 40) + hashtab(n * 3000) + editdist(n * 6) + gcdsum(n * 1500) + lcg(n * 30000) + binsearch(n * 1500) + prefix(n * 30000) + popcnt(n * 3000) + fib(12) + queens(6)) % 1000000007));
    return 0;
}
