import sys

def matmul(n):
    a = [0] * (n * n)
    b = [0] * (n * n)
    c = [0] * (n * n)
    i = 0
    while i < n * n:
        a[i] = (i * 7 + 3) % 10
        b[i] = (i * 5 + 1) % 10
        i = i + 1
    i = 0
    while i < n:
        j = 0
        while j < n:
            s = 0
            k = 0
            while k < n:
                s = s + a[i * n + k] * b[k * n + j]
                k = k + 1
            c[i * n + j] = s
            j = j + 1
        i = i + 1
    t = 0
    i = 0
    while i < n * n:
        t = (t + c[i]) % 1000003
        i = i + 1
    return t

def main():
    n = int(sys.argv[1])
    print(matmul(n))

main()
