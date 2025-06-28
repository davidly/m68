echo off
setlocal enabledelayedexpansion

for %%f in (*.c) do (
    set "filename=%%~nf"
    set "extension=%%~xf"

    REM Convert filename to lowercase
    for /L %%i in (A,1,Z) do (
        call set "filename=%%filename:%%i=%%i%%"
    )

    REM Convert extension to lowercase
    for /L %%i in (A,1,Z) do (
        call set "extension=%%extension:%%i=%%i%%"
    )

    REM Rename the file if the case has changed
    if not "%%f"=="!filename!!extension!" (
        ren "%%f" "!filename!!extension!"
    )
)
