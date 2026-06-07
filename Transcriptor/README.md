Proyecto final POO - Transcriptor de Audio con Diarizacion con IA

Transcribe archivos de audio identificando que persona habla en cada momento (Persona 1, Persona 2, etc.) y procesa el resultado con ChatGPT para limpiar, resumir, analizar y responder preguntas.

- **WhisperX** (m-bain) - transcripcion + alineacion + diarizacion (PyAnnote)
- **OpenAI GPT-4o-mini** - limpieza, resumen, analisis, Q&A
- **PySide6** (Qt) - interfaz grafica
- **PyTorch + CUDA 12.8** - aceleracion GPU

## Estructura

```
Transcriptor/
|-- src/
|   |-- transcriptor.py      AudioTranscriber (Whisper)
|   |-- alineador.py         AlineadorDeHablantes (wav2vec2)
|   |-- diarizador.py        DiarizadorDeHablantes (PyAnnote)
|   |-- formateador.py       FormateadorDeTranscripcion
|   |-- asistente_ia.py      AsistenteIA (OpenAI)
|   `-- pipeline.py          PipelineDeTranscripcion (orquesta todo)
|-- gui/
|   |-- ventana_principal.py Ventana PySide6
|   `-- worker.py            QThread para no bloquear UI
|-- main.py                  CLI
|-- main_gui.py              Launcher de la GUI
|-- requirements.txt
|-- .env.example             Plantilla de variables de entorno
`-- .gitignore
```

## Setup

1. Crear venv:
   ```
   python -m venv venv
   .\venv\Scripts\Activate.ps1
   ```

2. Instalar torch con CUDA (si tenes GPU NVIDIA):
   ```
   pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu128
   ```

3. Instalar el resto:
   ```
   pip install -r requirements.txt
   ```

4. Copiar `.env.example` a `.env` y completar los tokens.

## Uso

### CLI
```
python main.py audio.mp3 --modelo medium --hablantes 2 --resumir --analizar
```

### GUI
```
python main_gui.py
```
