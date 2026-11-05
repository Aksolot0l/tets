@echo off
setlocal
rem Save this machine's original DACL before granting AppContainer access.
set "DLL_PATH=%~dp0GAYClientInternal.dll"
set "BACKUP_FILE=%~dp0DACL_BACKUP.local.txt"

if not exist "%DLL_PATH%" goto missing_dll
if exist "%BACKUP_FILE%" goto backup_exists

echo Saving current DACL to "%BACKUP_FILE%"...
icacls "%DLL_PATH%" /save "%BACKUP_FILE%"
if errorlevel 1 goto backup_failed
if not exist "%BACKUP_FILE%" goto backup_failed

echo Granting read and execute access to ALL APPLICATION PACKAGES...
icacls "%DLL_PATH%" /grant "*S-1-15-2-1:(RX)"
if errorlevel 1 goto grant_failed

echo Done. Keep the local backup and use RESTORE_ACCESS.cmd to restore it.
pause
exit /b 0

:missing_dll
echo ERROR: DLL not found: "%DLL_PATH%"
goto failed
:backup_exists
echo ERROR: A local backup already exists: "%BACKUP_FILE%"
echo Restore it first. After verifying the restore, archive or remove the backup before running this script again.
goto failed
:backup_failed
echo ERROR: DACL backup failed. No access changes were made.
if exist "%BACKUP_FILE%" del "%BACKUP_FILE%"
goto failed
:grant_failed
echo ERROR: Grant failed. The original DACL was saved; use RESTORE_ACCESS.cmd if needed.
:failed
pause
exit /b 1
