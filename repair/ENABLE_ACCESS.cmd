@echo off
setlocal
set "A=%~dp0GAYClientInternal.dll"
set "B=%~dp0GAYClientCore.dll"
set "BA=%~dp0GAYClientInternal.acl.local.txt"
set "BB=%~dp0GAYClientCore.acl.local.txt"
if not exist "%A%" goto missing
if not exist "%B%" goto missing
if exist "%BA%" goto backup_exists
if exist "%BB%" goto backup_exists
icacls "%A%" /save "%BA%"
if errorlevel 1 goto backup_failed
if not exist "%BA%" goto backup_failed
icacls "%B%" /save "%BB%"
if errorlevel 1 goto backup_failed
if not exist "%BB%" goto backup_failed
icacls "%A%" /grant "*S-1-15-2-1:(RX)"
if errorlevel 1 goto grant_failed
icacls "%B%" /grant "*S-1-15-2-1:(RX)"
if errorlevel 1 goto grant_failed
echo Access granted. Keep both local backups for RESTORE_ACCESS.cmd.
pause
exit /b 0
:missing
echo ERROR: Both GAYClientInternal.dll and GAYClientCore.dll must be in this folder.
goto failed
:backup_exists
echo ERROR: A backup already exists. Restore and verify it before removing it.
goto failed
:backup_failed
echo ERROR: Backup failed. No access changes were made.
if exist "%BA%" del "%BA%"
if exist "%BB%" del "%BB%"
goto failed
:grant_failed
echo ERROR: Grant failed. Trying to restore both backups.
icacls "%~dp0." /restore "%BA%"
if errorlevel 1 echo ERROR: Could not restore the first backup.
icacls "%~dp0." /restore "%BB%"
if errorlevel 1 echo ERROR: Could not restore the second backup.
echo Backups were kept. Check permissions before removing them.
:failed
pause
exit /b 1
