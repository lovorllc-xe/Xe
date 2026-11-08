import sys

def lcg(n):
    x = 1
    s = 0
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
        i = i + 1
    return s

def main():
    n = int(sys.argv[1])
    print(lcg(n))

main()
