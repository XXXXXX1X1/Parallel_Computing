set +e
export LC_ALL=C

# ====== настройка ======
SRC="matrix.cpp"          # имя твоего .cpp
OUTCSV="results.csv"
LOGDIR="logs"

Ns=(20000 40000)
Ts=(1 2 4 7 8 16 20 40)

# OpenMP runtime (ты сказал, что всё скачано)
LIBOMP="$(brew --prefix libomp)"
CXX="clang++"
CXXFLAGS=(
  -O3
  -std=c++17
  -Xpreprocessor -fopenmp
  -I"$LIBOMP/include"
  -L"$LIBOMP/lib" -lomp
  -Wl,-rpath,"$LIBOMP/lib"
)

mkdir -p "$LOGDIR"

# CSV header
echo "N,threads_req,threads_used,init_time_s,work_time_s,total_time_s,speedup_work,speedup_total,res,status" > "$OUTCSV"

# ====== helpers ======
num_div() {
  # num_div a b => a/b (float)
  awk -v a="$1" -v b="$2" 'BEGIN{ if (b>0) printf "%.6f", a/b; else printf "" }'
}

for N in "${Ns[@]}"; do
  echo "=============================="
  echo "N=$N"
  echo "=============================="

  # делаем временную копию исходника и подменяем #define N ...
  TMP="__tmp_N${N}.cpp"
  sed -E "s/^([[:space:]]*#define[[:space:]]+N[[:space:]]+)[0-9]+/\1${N}/" "$SRC" > "$TMP"

  exe="matvec_N${N}"
  echo "Compile -> $exe"
  "$CXX" "${CXXFLAGS[@]}" "$TMP" -o "$exe"
  comp_rc=$?

  rm -f "$TMP"

  if [ $comp_rc -ne 0 ]; then
    echo "Compile failed for N=$N"
    for t in "${Ts[@]}"; do
      echo "$N,$t,,,,,,,,,COMPILE_FAIL" >> "$OUTCSV"
    done
    continue
  fi

  base_work=""
  base_total=""

  # шапка таблицы в терминал
  printf "%-7s %-10s %-12s %-12s %-12s %-12s %-14s %-14s %-12s\n" \
    "N" "threads" "used" "init(s)" "work(s)" "total(s)" "S_work" "S_total" "status"

  for t in "${Ts[@]}"; do
    log="$LOGDIR/N${N}_T${t}.log"

    out=$(OMP_DYNAMIC=FALSE OMP_NUM_THREADS="$t" ./"$exe" 2>&1 | tee "$log")
    rc=${PIPESTATUS[0]}

    if [ $rc -ne 0 ]; then
      printf "%-7s %-10s %-12s %-12s %-12s %-12s %-14s %-14s %-12s\n" \
        "$N" "$t" "" "" "" "" "" "" "RUNTIME_FAIL"
      echo "$N,$t,,,,,,,,,RUNTIME_FAIL" >> "$OUTCSV"
      continue
    fi

    threads_used=$(echo "$out" | awk -F'=' '/^threads=/{print $2; exit}')
    init_time=$(echo "$out" | awk -F'Init time' '/Init time/{print $2; exit}')
    work_time=$(echo "$out" | awk -F'Work time' '/Work time/{print $2; exit}')
    res=$(echo "$out" | tail -n 1)

    # total = init + work
    total_time=$(awk -v a="$init_time" -v b="$work_time" 'BEGIN{printf "%.10f", a+b}')

    # базовые времена для ускорения (по work и по total)
    if [ "$t" -eq 1 ] && [ -n "$work_time" ] && [ -n "$total_time" ]; then
      base_work="$work_time"
      base_total="$total_time"
    fi

    speedup_work=""
    speedup_total=""
    if [ -n "$base_work" ] && [ -n "$work_time" ]; then
      speedup_work=$(num_div "$base_work" "$work_time")
    fi
    if [ -n "$base_total" ] && [ -n "$total_time" ]; then
      speedup_total=$(num_div "$base_total" "$total_time")
    fi

    # терминал
    printf "%-7s %-10s %-12s %-12s %-12s %-12s %-14s %-14s %-12s\n" \
      "$N" "$t" "$threads_used" "$init_time" "$work_time" "$total_time" "$speedup_work" "$speedup_total" "OK"

    # CSV
    echo "$N,$t,$threads_used,$init_time,$work_time,$total_time,$speedup_work,$speedup_total,$res,OK" >> "$OUTCSV"
  done
done

echo "Saved CSV: $OUTCSV"
echo "Logs: $LOGDIR/"