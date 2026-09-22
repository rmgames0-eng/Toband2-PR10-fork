@echo off
cd /d "%~dp0"
python build-mingw.py
if errorlevel 1 (
  echo Build failed.
  pause
  exit /b 1
)
echo Build complete: build\mingw\TOband.exe
pause
