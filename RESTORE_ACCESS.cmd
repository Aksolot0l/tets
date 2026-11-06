@echo off
setlocal
rem icacls /restore takes the directory containing the file, not the file path.
set "DLL_PATH=%~dp0GAYClientInternal.dll"
set "BACKUP_FILE=%~dp0DACL_BACKUP.local.txt"

if not exist "%DLL_PATH%" goto missing_dll
if not exist "%BACKUP_FILE%" goto missing_backup

echo Restoring the local DACL backup...
icacls "%~dp0." /restore "%BACKUP_FILE%"
if errorlevel 1 goto restore_failed

echo Restore completed. The backup was kept; archive or remove it after verifying the result.
pause
exit /b 0

:missing_dll
echo ERROR: DLL not found: "%DLL_PATH%"
goto failed
:missing_backup
echo ERROR: Local backup not found: "%BACKUP_FILE%"
echo The bundled DACL_BACKUP.txt belongs to another machine and will not be used.
goto failed
:restore_failed
echo ERROR: Restore failed. The backup was kept.
:failed
pause
exit /b 1
