import sys

class Cell:
    __slots__ = ("value", "other")
    def __init__(self, value, other):
        self.value = value
        self.other = other

def churn(n):
    slots = [None] * 64
    total = 0
    for i in range(n):
        cell = Cell(i, 0)
        k = i % 64
        old = slots[k]
        if old is not None:
            total = (total + old.value) % 1000000007
        slots[k] = cell
    for old in slots:
        if old is not None:
            total = (total + old.value) % 1000000007
    return total

def main():
    n = int(sys.argv[1])
    print(churn(n))

main()
