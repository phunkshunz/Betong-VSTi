@echo off
rem Installerar maskinerna som ligger i den här mappen på Windows. Dubbelklicka på filen.
rem Plugin-mappen för VST3 kräver administratör, så filen ber om det själv.

net session >nul 2>&1
if errorlevel 1 (
  powershell -NoProfile -Command "Start-Process -Verb RunAs -FilePath '%~f0'"
  exit /b
)

cd /d "%~dp0"
set "DEST=%CommonProgramFiles%\VST3"
if not exist "%DEST%" mkdir "%DEST%"

set "FOUND="
for %%f in ("*.vst3") do (
  set "FOUND=1"
  if exist "%DEST%\%%~nxf" rmdir /s /q "%DEST%\%%~nxf"
  xcopy "%%~ff" "%DEST%\%%~nxf\" /e /i /y /q >nul
)

if not defined FOUND (
  echo.
  echo Hittar inga plugins har. Packade du upp zip-filen forst? Hogerklicka pa zip-filen och valj "Extrahera alla".
  pause
  exit /b 1
)

echo.
echo Klart! Maskinerna ligger nu i:
echo   %DEST%
echo Starta om Bitwig, eller tryck Rescan under Settings ^> Locations ^> Plug-ins.
echo Sok pa BETONG. Den ligger under tillverkaren Phunkshunz.
echo.
pause
