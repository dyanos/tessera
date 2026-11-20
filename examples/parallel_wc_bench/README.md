# parallel_wc benchmark

Compares `examples/parallel_wc.csec` with GNU `wc -l` and with a C++ reference counter
(`refwc.cpp`: memory-mapped file, N threads, vectorised byte compare).

```
python gen.py data1g.txt                      # 1 GiB test file
clang++ -O3 -march=native -std=c++17 refwc.cpp -o refwc.exe
tess -O3 --emit-exe ../parallel_wc.csec -o parallel_wc.exe
tess -O3 -march=native --emit-exe ../parallel_wc.csec -o parallel_wc_native.exe
python bench.py data1g.txt                    # copy System.Native.dll next to the .exe files first
```

`refwc.cpp` uses Win32 file mapping, so the reference builds on Windows only.
