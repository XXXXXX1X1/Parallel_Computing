import sys
import csv
import matplotlib.pyplot as plt

DEFAULT_CSV = "results.csv"
OUT_PNG = "speedup.png"
P_LIST = [1, 2, 4, 7, 8, 16, 20, 40]

def to_float(x):
    if x is None:
        return None
    s = str(x).strip().replace(",", ".")
    if s == "":
        return None
    try:
        return float(s)
    except ValueError:
        return None

def main():
    path = sys.argv[1] if len(sys.argv) >= 2 else DEFAULT_CSV

    # data[N][p] = work_time
    data = {}

    with open(path, "r", newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            if row.get("status") != "OK":
                continue
            N = int(row["N"])
            p = int(row["threads"])
            work = to_float(row.get("work_time_s"))
            if work is None or work <= 0:
                continue
            data.setdefault(N, {})[p] = work

    plt.figure()
    plt.plot(P_LIST, P_LIST, "--o", label="Linear (S=p)")

    for N in sorted(data.keys()):
        t1 = data[N].get(1)
        if t1 is None:
            continue

        xs, ys = [], []
        for p in P_LIST:
            tp = data[N].get(p)
            if tp is None:
                continue
            xs.append(p)
            ys.append(t1 / tp)

        if xs:
            plt.plot(xs, ys, "-o", label=f"N={N}")

    plt.xlabel("p (threads)")
    plt.ylabel("S(p) = T1 / Tp (work time)")
    plt.title("OpenMP scalability (from results.csv)")
    plt.grid(True)
    plt.legend()
    plt.savefig(OUT_PNG, dpi=200)
    print(f"OK: saved {OUT_PNG}")

if __name__ == "__main__":
    main()