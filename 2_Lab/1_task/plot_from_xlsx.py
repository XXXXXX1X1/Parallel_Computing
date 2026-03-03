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
    path = DEFAULT_CSV
    if len(sys.argv) >= 2:
        path = sys.argv[1]

    # data[N][p] = {"work":..., "speedup":...}
    data = {}

    with open(path, "r", newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            if row.get("status") != "OK":
                continue

            N = int(row["N"])
            p = int(row["threads_req"])

            work = to_float(row.get("work_time_s"))
            s_work = to_float(row.get("speedup_work"))

            data.setdefault(N, {})[p] = {"work": work, "speedup": s_work}

    # если speedup_work пустой — считаем сами как T1/Tp
    for N, mp in data.items():
        t1 = mp.get(1, {}).get("work")
        if t1 is None or t1 <= 0:
            continue
        for p, rec in mp.items():
            if rec.get("speedup") is None and rec.get("work") not in (None, 0.0):
                rec["speedup"] = t1 / rec["work"]

    plt.figure()
    plt.plot(P_LIST, P_LIST, "--o", label="Linear (S=p)")

    for N in sorted(data.keys()):
        xs, ys = [], []
        for p in P_LIST:
            if p in data[N] and data[N][p].get("speedup") is not None:
                xs.append(p)
                ys.append(data[N][p]["speedup"])
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