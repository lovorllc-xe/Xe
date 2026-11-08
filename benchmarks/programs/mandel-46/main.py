import sys

def mandel(n):
    count = 0
    py = 0
    while py < n:
        px = 0
        while px < n:
            cx = float(px) * 3.0 / float(n) - 2.0
            cy = float(py) * 2.0 / float(n) - 1.0
            x = 0.0
            y = 0.0
            it = 0
            inside = 1
            while it < 40  and  inside == 1:
                t = x * x - y * y + cx
                y = 2.0 * x * y + cy
                x = t
                if x * x + y * y > 4.0:
                    inside = 0
                it = it + 1
            count = count + inside
            px = px + 1
        py = py + 1
    return count

def main():
    n = int(sys.argv[1])
    print(mandel(n))

main()
