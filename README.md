# hetarch
#### Сборка:
1. Собрать llvm 3.7 с бекендами MSP430, X86 (в зависимости от того, для локального тестирования или для тестрования на железе хочется собрать). Патченая для MSP430 версия здесь: https://github.com/melentyev/llvm/tree/release_37 (важно - именно ветка release_37)
```
cmake -DLLVM_TARGETS_TO_BUILD=MSP430;X86 ... 
make ...
```
2. Установить в CMakeLists.txt пути к LLVM, путь к asio (non-boost версия)
3. Запустить cmake. Теперь можно собирать cmake таргеты

#### Локальное тестирование
1. Для локального тестирования собрать таргеты localLoader, hostSample1
2. localLoader - эмулирует удалнное устройство, его запустить первым
3. hostSample1 - подключается к localLoader, из dsl-описания с помощью llvm генерирует x86_64 код, загружает на localLoader, исполняет
