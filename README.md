### Установка
```
mkdir build
cd build
cmake ..
make
```

### Использование
1. Для запуска основной утилиты: 

```
./statdump_tool input_a.bin input_b.bin output.bin
``` 

2. Для выполнения тестов: 
```
./test_runner ./statdump_tool
``` 

### Одной командой(билд+тесты)
```
./build_n_test.sh
```
