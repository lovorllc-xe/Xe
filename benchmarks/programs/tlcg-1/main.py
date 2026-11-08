import sys
import threading

def tlcg_worker(lo, hi, out, slot):
    s = 0
    x = lo % 1000 + 1
    for i in range(lo, hi):
        x = (x * 1103515245 + 12345) % 2147483648
        s = (s + (x >> 16)) % 1000000007
    out[slot] = s

def tlcg(n):
    out = [0] * 4
    threads = []
    for t in range(4):
        th = threading.Thread(target=tlcg_worker, args=(n * t // 4, n * (t + 1) // 4, out, t))
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
    print(tlcg(n))

main()
