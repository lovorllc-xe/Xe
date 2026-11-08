import sys

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

def main():
    n = int(sys.argv[1])
    print(editdist(n))

main()
