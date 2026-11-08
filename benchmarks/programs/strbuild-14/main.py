import sys

def strbuild(n):
    parts = []
    for i in range(n):
        parts.append(chr(97 + i % 26))
    s = "".join(parts)
    c = 0
    for ch in s:
        if ch == "a":
            c += 1
    return c

def main():
    n = int(sys.argv[1])
    print(strbuild(n))

main()
