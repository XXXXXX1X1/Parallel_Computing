import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

OUT_DIR = Path("results")
OUT_DIR.mkdir(exist_ok=True)

def load_one(path):
    df = pd.read_csv(path)
    df = df.sort_values("threads").copy()

    t1 = float(df[df["threads"] == 1]["work_time_s"].iloc[0])
    df["speedup"] = t1 / df["work_time_s"]
    df["efficiency"] = df["speedup"] / df["threads"]

    return df

d1 = load_one(OUT_DIR / "lab2_var1_threads.csv")
d2 = load_one(OUT_DIR / "lab2_var2_threads.csv")

plt.figure(figsize=(8, 5))
plt.plot(d1["threads"], d1["work_time_s"], marker="o", label="Вариант 1")
plt.plot(d2["threads"], d2["work_time_s"], marker="o", label="Вариант 2")
plt.title("Время работы от числа потоков")
plt.xlabel("Число потоков")
plt.ylabel("Время, с")
plt.grid(True)
plt.legend()
plt.savefig(OUT_DIR / "time_vs_threads.png", dpi=150, bbox_inches="tight")
plt.close()

plt.figure(figsize=(8, 5))
plt.plot(d1["threads"], d1["speedup"], marker="o", label="Вариант 1")
plt.plot(d2["threads"], d2["speedup"], marker="o", label="Вариант 2")
plt.plot(d1["threads"], d1["threads"], linestyle="--", label="Линейное ускорение")
plt.title("Ускорение от числа потоков")
plt.xlabel("Число потоков")
plt.ylabel("S(p)")
plt.grid(True)
plt.legend()
plt.savefig(OUT_DIR / "speedup_vs_threads.png", dpi=150, bbox_inches="tight")
plt.close()

plt.figure(figsize=(8, 5))
plt.plot(d1["threads"], d1["efficiency"], marker="o", label="Вариант 1")
plt.plot(d2["threads"], d2["efficiency"], marker="o", label="Вариант 2")
plt.title("Эффективность от числа потоков")
plt.xlabel("Число потоков")
plt.ylabel("E(p)")
plt.grid(True)
plt.legend()
plt.savefig(OUT_DIR / "efficiency_vs_threads.png", dpi=150, bbox_inches="tight")
plt.close()

print("Графики сохранены в папку results.")