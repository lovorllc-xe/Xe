import sys

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

def main():
    n = int(sys.argv[1])
    print(gcdsum(n))

main()
