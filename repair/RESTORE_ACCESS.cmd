@echo off
setlocal
set "A=%~dp0GAYClientInternal.dll"
set "B=%~dp0GAYClientCore.dll"
set "BA=%~dp0GAYClientInternal.acl.local.txt"
set "BB=%~dp0GAYClientCore.acl.local.txt"
if not exist "%A%" goto missing
if not exist "%B%" goto missing
if not exist "%BA%" goto missing
if not exist "%BB%" goto missing
set "FAILED=0"
icacls "%~dp0." /restore "%BA%"
if errorlevel 1 set "FAILED=1"
icacls "%~dp0." /restore "%BB%"
if errorlevel 1 set "FAILED=1"
if "%FAILED%"=="1" goto restore_failed
echo Restore completed. Verify permissions before archiving or removing backups.
pause
exit /b 0
:missing
echo ERROR: Both DLLs and both local backups are required.
goto failed
:restore_failed
echo ERROR: Restore failed for one or more files. Backups were kept.
:failed
pause
exit /b 1
