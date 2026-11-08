import sys
import threading

def tprime_worker(lo, hi, out, slot):
    s = 0
    for x in range(lo, hi):
        if x >= 2:
            prime = 1
            d = 2
            while d * d <= x and prime == 1:
                if x % d == 0:
                    prime = 0
                d += 1
            s += prime
    out[slot] = s

def tprime(n):
    out = [0] * 8
    threads = []
    for t in range(8):
        th = threading.Thread(target=tprime_worker, args=(n * t // 8, n * (t + 1) // 8, out, t))
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
    print(tprime(n))

main()
