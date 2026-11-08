import sys

def queens_rec(n, row, cols, d1, d2):
    if row == n:
        return 1
    count = 0
    c = 0
    while c < n:
        b1 = 1 << c
        b2 = 1 << (row + c)
        b3 = 1 << (row - c + n)
        if (cols & b1) == 0  and  (d1 & b2) == 0  and  (d2 & b3) == 0:
            count = count + queens_rec(n, row + 1, cols | b1, d1 | b2, d2 | b3)
        c = c + 1
    return count

def queens(n):
    return queens_rec(n, 0, 0, 0, 0)

def main():
    n = int(sys.argv[1])
    print(queens(n))

main()
