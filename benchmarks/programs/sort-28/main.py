import sys

def sort(n):
    a = [0] * (n)
    x = 12345
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345) % 2147483648
        a[i] = x % 100000
        i = i + 1
    i = 1
    while i < n:
        v = a[i]
        j = i - 1
        while j >= 0  and  a[j] > v:
            a[j + 1] = a[j]
            j = j - 1
        a[j + 1] = v
        i = i + 1
    h = 0
    i = 0
    while i < n:
        h = (h * 31 + a[i]) % 1000000007
        i = i + 1
    return h

def main():
    n = int(sys.argv[1])
    print(sort(n))

main()
