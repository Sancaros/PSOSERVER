@echo off

setlocal

set VERSION_FILE=$(SolutionDir)version.h
set /a VERSION_MAJOR=0
set /a VERSION_MINOR=0
set /a VERSION_PATCH=0

rem 读取当前版本号
for /f "usebackq tokens=1,3" %%i in (`findstr /R "^#define VERSION_MAJOR" "%VERSION_FILE%"`) do (
    set /a VERSION_MAJOR=%%j
)
for /f "usebackq tokens=1,3" %%i in (`findstr /R "^#define VERSION_MINOR" "%VERSION_FILE%"`) do (
    set /a VERSION_MINOR=%%j
)
for /f "usebackq tokens=1,3" %%i in (`findstr /R "^#define VERSION_PATCH" "%VERSION_FILE%"`) do (
    set /a VERSION_PATCH=%%j
)

rem 递增版本号
set /a VERSION_PATCH+=1
if %VERSION_PATCH% gtr 9 (
    set VERSION_PATCH=0
    set /a VERSION_MINOR+=1
)
if %VERSION_MINOR% gtr 9 (
    set VERSION_MINOR=0
    set /a VERSION_MAJOR+=1
)

rem 更新版本号
echo #define VERSION_MAJOR %VERSION_MAJOR% > "%VERSION_FILE%.tmp"
echo #define VERSION_MINOR %VERSION_MINOR% >> "%VERSION_FILE%.tmp"
echo #define VERSION_PATCH %VERSION_PATCH% >> "%VERSION_FILE%.tmp"

rem 将临时文件替换回源文件
move /y "%VERSION_FILE%.tmp" "%VERSION_FILE%"

endlocal