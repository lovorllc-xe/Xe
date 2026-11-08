import sys

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

def main():
    n = int(sys.argv[1])
    print(prefix(n))

main()
