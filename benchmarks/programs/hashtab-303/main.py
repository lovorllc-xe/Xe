import sys

def hashtab(n):
    cap = 16
    while cap < n * 2:
        cap = cap * 2
    keys = [0] * (cap)
    vals = [0] * (cap)
    i = 1
    while i <= n:
        k = (i * 2654435761) % 4294967296
        h = k & (cap - 1)
        while keys[h] != 0  and  keys[h] != k:
            h = (h + 1) & (cap - 1)
        keys[h] = k
        vals[h] = vals[h] + i
        i = i + 1
    total = 0
    i = 1
    while i <= n:
        k = (i * 2654435761) % 4294967296
        h = k & (cap - 1)
        while keys[h] != k:
            h = (h + 1) & (cap - 1)
        total = (total + vals[h]) % 1000000007
        i = i + 1
    return total

def main():
    n = int(sys.argv[1])
    print(hashtab(n))

main()
