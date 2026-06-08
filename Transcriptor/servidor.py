"""
Servidor HTTP local que expone el pipeline de transcripcion via REST.

Endpoints:
    GET  /estado        -> {"status":"ok", "assemblyai": bool, "openai": bool}
    POST /transcribir   -> multipart/form-data con campo "audio" (archivo)
                           params opcionales: idioma, num_hablantes
                        <- {"texto": str, "participacion": dict, "segmentos": list}

Uso:
    python servidor.py           # escucha en localhost:8765
    python servidor.py 9000      # puerto personalizado
"""

import os
import sys
import tempfile
from pathlib import Path

from dotenv import load_dotenv
from flask import Flask, jsonify, request

load_dotenv()

app = Flask(__name__)
PUERTO_DEFAULT = 8765


# ------------------------------------------------------------------ endpoints

@app.get("/estado")
def estado():
    """Healthcheck: informa si las API keys estan configuradas."""
    return jsonify({
        "status": "ok",
        "assemblyai": bool(os.getenv("ASSEMBLYAI_API_KEY", "")),
        "openai":     bool(os.getenv("OPENAI_API_KEY", "")),
    })


@app.post("/transcribir")
def transcribir():
    """
    Recibe un archivo de audio y devuelve la transcripcion con diarizacion.

    Form fields:
        audio          (file, requerido)
        idioma         (str,  opcional, default "es")
        num_hablantes  (int,  opcional)
    """
    if "audio" not in request.files:
        return jsonify({"error": "Falta el campo 'audio' en el form"}), 400

    archivo = request.files["audio"]
    idioma = request.form.get("idioma", "es")

    num_hablantes = None
    if "num_hablantes" in request.form:
        try:
            num_hablantes = int(request.form["num_hablantes"])
        except ValueError:
            pass

    api_key = os.getenv("ASSEMBLYAI_API_KEY", "")
    if not api_key:
        return jsonify({"error": "Sin ASSEMBLYAI_API_KEY en .env"}), 503

    sufijo = Path(archivo.filename).suffix if archivo.filename else ".wav"
    tmp = tempfile.NamedTemporaryFile(delete=False, suffix=sufijo)
    try:
        archivo.save(tmp.name)
        tmp.close()

        from src.pipeline_assemblyai import PipelineAssemblyAI
        pipeline = PipelineAssemblyAI(
            api_key=api_key,
            idioma=idioma,
            num_hablantes=num_hablantes,
        )
        resultado = pipeline.ejecutar(tmp.name)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500
    finally:
        Path(tmp.name).unlink(missing_ok=True)

    segmentos = [
        {
            "speaker": seg.get("speaker"),
            "start":   round(seg.get("start", 0.0), 3),
            "end":     round(seg.get("end", 0.0), 3),
            "texto":   seg.get("text", ""),
        }
        for seg in resultado["resultado"].get("segments", [])
    ]

    return jsonify({
        "texto":        resultado["texto"],
        "participacion": resultado["participacion"],
        "segmentos":    segmentos,
    })


# ------------------------------------------------------------------ main

if __name__ == "__main__":
    puerto = int(sys.argv[1]) if len(sys.argv) > 1 else PUERTO_DEFAULT
    print(f"[DIArize servidor] http://localhost:{puerto}")
    print(f"  AssemblyAI key : {'OK' if os.getenv('ASSEMBLYAI_API_KEY') else 'FALTA'}")
    print(f"  OpenAI key     : {'OK' if os.getenv('OPENAI_API_KEY') else 'no configurada'}")
    app.run(host="127.0.0.1", port=puerto, debug=False)
