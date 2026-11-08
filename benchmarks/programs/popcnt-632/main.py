import sys

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

def main():
    n = int(sys.argv[1])
    print(popcnt(n))

main()
