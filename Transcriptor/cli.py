"""
Puente CLI para la app C++/Qt (proyecto hibrido).
=================================================
La app C++ ejecuta este script con QProcess, le pasa el audio y la cantidad de
personas, y recibe el resultado en JSON por STDOUT.

Contrato:
    - STDOUT  : EXCLUSIVAMENTE el JSON final (una sola linea). Nada mas.
    - STDERR  : progreso y logs. El progreso va con el prefijo "PROGRESO|pct|etapa"
                para que el C++ lo parsee y mueva la barra.

Uso:
    python cli.py --audio "ruta.mp3" --personas 2 --modelo medium --idioma es

JSON de salida (exito):
    {"ok": true, "texto": "...", "participacion": {"Persona 1": 5, ...}}
JSON de salida (error):
    {"ok": false, "error": "Tipo: mensaje"}
"""

import argparse
import json
import os
import sys

from dotenv import load_dotenv


def main():
    load_dotenv()

    parser = argparse.ArgumentParser(description="Puente CLI del Transcriptor para C++")
    parser.add_argument("--audio", required=True, help="Ruta al archivo de audio")
    parser.add_argument("--personas", type=int, default=None, help="Cantidad exacta de hablantes")
    parser.add_argument("--modelo", default="medium", help="Modelo Whisper")
    parser.add_argument("--idioma", default="es", help="Codigo de idioma")
    parser.add_argument("--dispositivo", default=None, choices=["cpu", "cuda"])
    args = parser.parse_args()

    # Todo lo que el pipeline (whisperx/torch/pyannote) imprima por print() debe ir
    # a STDERR para no ensuciar el JSON. Redirigimos stdout -> stderr durante el
    # procesamiento y solo al final escribimos el JSON en el stdout real.
    stdout_real = sys.stdout
    sys.stdout = sys.stderr

    def emitir_progreso(etapa, pct):
        print(f"PROGRESO|{pct}|{etapa}", file=sys.stderr, flush=True)

    try:
        import torch
        from src.pipeline import PipelineDeTranscripcion

        dispositivo = args.dispositivo or ("cuda" if torch.cuda.is_available() else "cpu")
        hf_token = os.getenv("HF_TOKEN", "")

        pipeline = PipelineDeTranscripcion(
            modelo_whisper=args.modelo,
            idioma=args.idioma,
            hf_token=hf_token,
            dispositivo=dispositivo,
            num_hablantes=args.personas,
            progreso=emitir_progreso,
        )
        salida = pipeline.ejecutar(args.audio)

        resultado = {
            "ok": True,
            "texto": salida["texto"],
            "participacion": salida["participacion"],
        }
        codigo = 0
    except Exception as e:
        import traceback
        traceback.print_exc()  # va a stderr para debug
        resultado = {"ok": False, "error": f"{type(e).__name__}: {e}"}
        codigo = 1

    # Restauramos el stdout real y escribimos SOLO el JSON, en bytes UTF-8 crudos.
    # IMPORTANTE: cuando una app grafica (QProcess) lanza Python, su stdout queda en
    # cp1252, no UTF-8. Si escribieramos en modo texto, los acentos saldrian en cp1252
    # y el C++ (que lee UTF-8) no podria parsear el JSON. Por eso usamos .buffer.write
    # con los bytes ya codificados en UTF-8.
    sys.stdout = stdout_real
    datos = json.dumps(resultado, ensure_ascii=False).encode("utf-8")
    sys.stdout.buffer.write(datos)
    sys.stdout.buffer.flush()
    sys.exit(codigo)


if __name__ == "__main__":
    main()
