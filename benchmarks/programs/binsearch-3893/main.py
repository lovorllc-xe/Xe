import sys

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

def main():
    n = int(sys.argv[1])
    print(binsearch(n))

main()
