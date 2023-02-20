@echo off
setlocal
set LUA_VERSION_NAME=""   %5.4.4%
rd build
mkdir build
cd build
cl /O2 /W3 /c /DLUA_BUILD_AS_DLL ../src/l*.c
del lua.obj luac.obj
link /DLL /out:lua%LUA_VERSION%.dll l*.obj
cl /O2 /W3 /c /DLUA_BUILD_AS_DLL ../src/lua.c ../src/luac.c
link /out:lua.exe lua.obj lua%LUA_VERSION%.lib
del lua.obj
link /out:luac.exe l*.obj
del *.obj
cd ..
endlocal
