import sys
import threading

def tsum_worker(lo, hi, out, slot):
    s = 0
    for i in range(lo, hi):
        s += (i * i) % 7
    out[slot] = s

def tsum(n):
    out = [0] * 4
    threads = []
    for t in range(4):
        th = threading.Thread(target=tsum_worker, args=(n * t // 4, n * (t + 1) // 4, out, t))
        th.start()
        threads.append(th)
    for th in threads:
        th.join()
    total = 0
    for v in out:
        total = (total + v) % 1000000007
    return total

def main():
    n = int(sys.argv[1])
    print(tsum(n))

main()
