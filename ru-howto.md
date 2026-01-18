# ru
## сначала копируйте репозиторий
```bash
git clone https://github.com/kryyaa/pml
cd pml
```
<br>

## получите байты вашего payload <sub>и замените их в bytes.cpp</sub>
```bash
python exe2hex.py <payload.exe> <out.txt>
```

<br>

# компиляция
## [Zig](https://ziglang.org/download/)
```bash
:: debug
zig.exe c++ -shared -o pml_debug.dll main.cpp load.cpp bytes.cpp -target x86_64-windows -lkernel32 -DDEBUG_CONSOLE -O0 -g

:: release
zig.exe c++ -shared -o pml.dll main.cpp load.cpp bytes.cpp -target x86_64-windows -lkernel32 -O2 -s
```
---

## MSVC
```bash
:: debug
cl /LD /Fepml_debug.dll main.cpp load.cpp bytes.cpp kernel32.lib /DDEBUG_CONSOLE /Od /Zi /MDd

:: release
cl /LD /Fepml.dll main.cpp load.cpp bytes.cpp kernel32.lib /O2 /MD
```
---

## MinGW
```bash
:: debug
x86_64-w64-mingw32-g++ -shared -o pml_debug.dll main.cpp load.cpp bytes.cpp -lkernel32 -DDEBUG_CONSOLE -O0 -g

:: release
x86_64-w64-mingw32-g++ -shared -o pml.dll main.cpp load.cpp bytes.cpp -lkernel32 -O2 -s
```
