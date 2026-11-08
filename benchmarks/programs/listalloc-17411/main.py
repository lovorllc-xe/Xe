import sys

class Node:
    __slots__ = ("value", "next")
    def __init__(self, value, nxt):
        self.value = value
        self.next = nxt

def listalloc(n):
    head = None
    for i in range(n):
        head = Node(i, head)
    total = 0
    cur = head
    while cur is not None:
        total = (total + cur.value) % 1000000007
        cur = cur.next
    return total

def main():
    n = int(sys.argv[1])
    print(listalloc(n))

main()
