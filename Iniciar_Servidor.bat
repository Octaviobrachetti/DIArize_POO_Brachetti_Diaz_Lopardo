@echo off
set PYTHONUTF8=1
cd /d "%~dp0"
echo [DIArize] Iniciando servidor en http://localhost:8765 ...
echo [DIArize] Deja esta ventana abierta mientras usas el cliente C++.
echo.
"%~dp0venv\Scripts\python.exe" -X utf8 "%~dp0Transcriptor\servidor.py"
pause
