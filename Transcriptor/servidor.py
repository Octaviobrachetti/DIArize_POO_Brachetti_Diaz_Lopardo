"""
Servidor HTTP local que expone el pipeline de transcripcion y IA via REST.

Endpoints:
    GET  /estado        -> {"status":"ok", "assemblyai": bool, "ia": bool}
    POST /transcribir   -> multipart/form-data con campo "audio" (archivo)
                           params opcionales: idioma, num_hablantes
                        <- {"texto", "participacion", "segmentos"}

    POST /limpiar       -> JSON {"texto": "..."}      <- {"resultado": "..."}
    POST /resumir       -> JSON {"texto": "..."}      <- {"resultado": "..."}
    POST /analizar      -> JSON {"texto": "..."}      <- {"resultado": "..."}
    POST /preguntar     -> JSON {"texto": "...",
                                  "pregunta": "...",
                                  "historial": [[q,a], ...]}
                        <- {"resultado": "..."}

Uso:
    python servidor.py           # escucha en localhost:8765
    python servidor.py 9000      # puerto personalizado
"""

import os
import sys
import tempfile
from pathlib import Path

import requests
from dotenv import load_dotenv
from flask import Flask, jsonify, request

load_dotenv()

app = Flask(__name__)
PUERTO_DEFAULT = 8765

# IA singleton (lazy)
_ia = None


def _get_ia():
    """Devuelve un AsistenteIA inicializado o None si no hay API key."""
    global _ia
    if _ia is not None:
        return _ia

    from src.asistente_ia import AsistenteIA

    gemini_key = os.getenv("GEMINI_API_KEY", "")
    oai_key    = os.getenv("OPENAI_API_KEY", "")
    ia_key = gemini_key or oai_key
    if not ia_key:
        return None

    if gemini_key:
        base_url = "https://generativelanguage.googleapis.com/v1beta/openai/"
        modelo   = "gemini-2.5-flash-lite"
    else:
        base_url = None
        modelo   = os.getenv("OPENAI_MODEL", "gpt-4o-mini")

    _ia = AsistenteIA(api_key=ia_key, modelo=modelo, base_url=base_url)
    return _ia


# ------------------------------------------------------------------ endpoints

@app.get("/estado")
def estado():
    """Healthcheck: informa si las API keys estan configuradas."""
    return jsonify({
        "status": "ok",
        "assemblyai": bool(os.getenv("ASSEMBLYAI_API_KEY", "")),
        "ia":         bool(os.getenv("GEMINI_API_KEY", "") or os.getenv("OPENAI_API_KEY", "")),
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
            if num_hablantes <= 0:
                num_hablantes = None
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


# ------------------------------------------------------------------ streaming

@app.get("/streaming-token")
def streaming_token():
    """
    Pide a AssemblyAI un token temporal para que el cliente C++ se conecte
    al WebSocket de streaming sin exponer la API key del servidor.
    """
    api_key = os.getenv("ASSEMBLYAI_API_KEY", "")
    if not api_key:
        return jsonify({"error": "Sin ASSEMBLYAI_API_KEY en .env"}), 503

    try:
        # Streaming v3: token efimero (10 min)
        r = requests.get(
            "https://streaming.assemblyai.com/v3/token?expires_in_seconds=600",
            headers={"Authorization": api_key},
            timeout=10,
        )
        if r.status_code != 200:
            return jsonify({"error": f"AssemblyAI: {r.status_code} {r.text}"}), 502
        data = r.json()
        return jsonify({"token": data.get("token", "")})
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500


# ------------------------------------------------------------------ IA endpoints

def _ia_simple(metodo_nombre: str):
    """Helper para /limpiar /resumir /analizar: recibe {texto}, llama al metodo, devuelve {resultado}."""
    ia = _get_ia()
    if ia is None:
        return jsonify({"error": "Sin API key de IA (GEMINI_API_KEY u OPENAI_API_KEY)."}), 503

    data = request.get_json(silent=True) or {}
    texto = (data.get("texto") or "").strip()
    if not texto:
        return jsonify({"error": "Falta el campo 'texto'."}), 400

    try:
        metodo = getattr(ia, metodo_nombre)
        resultado = metodo(texto)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500

    return jsonify({"resultado": resultado})


@app.post("/limpiar")
def limpiar():
    return _ia_simple("limpiar")


@app.post("/resumir")
def resumir():
    return _ia_simple("resumir")


@app.post("/analizar")
def analizar():
    return _ia_simple("analizar")


@app.post("/traducir")
def traducir():
    """JSON: {"texto", "idioma_destino"}"""
    ia = _get_ia()
    if ia is None:
        return jsonify({"error": "Sin API key de IA (GEMINI_API_KEY u OPENAI_API_KEY)."}), 503

    data = request.get_json(silent=True) or {}
    texto  = (data.get("texto") or "").strip()
    idioma = (data.get("idioma_destino") or "").strip()
    if not texto or not idioma:
        return jsonify({"error": "Faltan 'texto' o 'idioma_destino'."}), 400

    try:
        resultado = ia.traducir(texto, idioma)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500

    return jsonify({"resultado": resultado})


@app.post("/preguntar")
def preguntar():
    """JSON: {"texto", "pregunta", "historial":[[q,a],...]}"""
    ia = _get_ia()
    if ia is None:
        return jsonify({"error": "Sin API key de IA (GEMINI_API_KEY u OPENAI_API_KEY)."}), 503

    data = request.get_json(silent=True) or {}
    texto    = (data.get("texto") or "").strip()
    pregunta = (data.get("pregunta") or "").strip()
    historial_raw = data.get("historial") or []
    if not texto or not pregunta:
        return jsonify({"error": "Faltan 'texto' o 'pregunta'."}), 400

    # historial llega como lista de pares [q, a] -> tuplas
    historial = [(str(p[0]), str(p[1])) for p in historial_raw if len(p) >= 2]

    try:
        resultado = ia.preguntar(texto, pregunta, historial=historial)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500

    return jsonify({"resultado": resultado})


# ------------------------------------------------------------------ main

if __name__ == "__main__":
    puerto = int(sys.argv[1]) if len(sys.argv) > 1 else PUERTO_DEFAULT
    print(f"[DIArize servidor] http://localhost:{puerto}")
    print(f"  AssemblyAI key : {'OK' if os.getenv('ASSEMBLYAI_API_KEY') else 'FALTA'}")
    ia_key = os.getenv('GEMINI_API_KEY') or os.getenv('OPENAI_API_KEY')
    print(f"  IA key         : {'OK' if ia_key else 'no configurada'}")
    app.run(host="127.0.0.1", port=puerto, debug=False)
