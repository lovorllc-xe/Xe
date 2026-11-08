import sys

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

def main():
    n = int(sys.argv[1])
    print(collatz(n))

main()
