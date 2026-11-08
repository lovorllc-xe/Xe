import sys

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

def main():
    n = int(sys.argv[1])
    print(sieve(n))

main()
