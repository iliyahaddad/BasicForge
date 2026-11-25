@echo off
cd "C:\Users\manshadi.PDF\Desktop\BasicForge"
"C:\Program Files\LLVM\bin\clang-cl.exe" -target x86_64-pc-windows-msvc -std:c++20 -O2 -I "C:\Users\manshadi.PDF\Desktop\BasicForge\compiler\include" -c "C:\Users\manshadi.PDF\Desktop\BasicForge\compiler\src\main.cpp" -Fo:"C:\Users\manshadi.PDF\Desktop\BasicForge\obj\main.obj"
echo Done with main.cpp