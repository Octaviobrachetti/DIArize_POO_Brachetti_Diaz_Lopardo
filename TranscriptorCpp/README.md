# Transcriptor de Audio con IA — Versión C++ / Qt (POO)

App de escritorio en **C++/Qt6** que transcribe audio con diarización de hablantes
y lo procesa con IA (resumen, análisis y Q&A).

## Arquitectura (híbrida)

```
App C++/Qt  ──QProcess──►  cli.py (Python)  ──►  WhisperX (transcripción + diarización)
     │
     └──Qt Network──►  OpenAI API  (resumen / análisis / Q&A)
```

- **Todo el programa y la POO están en C++** (interfaz, clases, OpenAI, grabar/reproducir).
- La transcripción + diarización (WhisperX) sigue corriendo en el **motor Python**
  existente (carpeta `Transcriptor/`), que el C++ ejecuta por detrás. Esto evita
  reescribir la diarización y reusa lo que ya funciona.

### Clases principales (C++)
- `VentanaPrincipal` — interfaz (QMainWindow): cargar/grabar/reproducir audio, tabs.
- `MotorTranscripcion` — lanza `cli.py` con `QProcess`, lee progreso (stderr) y
  resultado JSON (stdout).
- `AsistenteIA` — cliente de OpenAI con `QNetworkAccessManager` (limpiar/resumir/
  analizar/preguntar).

## Requisitos
- Qt 6 (Widgets, Multimedia, Network) + Qt Creator.
- El motor Python (`Transcriptor/`) ya instalado y funcionando, con su `venv` y `.env`
  (HF_TOKEN y, opcional, OPENAI_API_KEY).

## Cómo compilar y correr
1. Abrir `CMakeLists.txt` con Qt Creator (kit Qt 6).
2. Compilar y ejecutar (Ctrl+R).
3. La primera vez, si el motor no está en la ruta por defecto, ir a
   **Configurar → Carpeta del motor Python...** y elegir la carpeta `Transcriptor/`
   (la que contiene `cli.py`, `venv/` y `.env`).

## Notas
- La clave de OpenAI se lee de la variable de entorno `OPENAI_API_KEY` o, si no está,
  del archivo `.env` dentro de la carpeta del motor.
- La transcripción puede tardar varios minutos (corre WhisperX en Python).
- La ruta del motor por defecto está en `ventanaprincipal.cpp` (`CARPETA_MOTOR_DEFAULT`).
