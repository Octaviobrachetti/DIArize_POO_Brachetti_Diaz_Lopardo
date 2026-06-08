@echo off
set PYTHONUTF8=1
cd /d "%~dp0"
start "" "%~dp0venv\Scripts\pythonw.exe" -X utf8 "%~dp0Transcriptor\main_gui.py"
