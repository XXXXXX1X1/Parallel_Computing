# sine_sum
Заполнить масив float/double значениями синуса (один период на всю длину массива), количество элементов 10^7. Посчитать сумму и вывести в терминал.

## Сборка и запуск

### 1) Вариант `float` (по умолчанию)
Тип массива `float` используется, если опция `ARRAY_DOUBLE` выключена.

```bash
mkdir build
cd build
cmake .. -DTYPE=ON
make
./sine_sum
```

## output:
```bash
double: sum = 4.80487e-11
float:  sum = -0.0277862
```