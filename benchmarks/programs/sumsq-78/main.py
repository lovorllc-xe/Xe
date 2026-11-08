import sys

def sumsq(n):
    s = 0
    i = 0
    while i < n:
        s = s + (i * i) % 7
        i = i + 1
    return s

def main():
    n = int(sys.argv[1])
    print(sumsq(n))

main()
