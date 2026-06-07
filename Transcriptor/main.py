"""
Transcriptor de Audio con Diarizacion + IA - CLI
=================================================
Proyecto POO - Universidad Blas Pascal

Uso:
    python main.py audio.mp3
    python main.py audio.wav --modelo medium --hablantes 2
    python main.py audio.mp3 --salida transcripcion.txt --resumir --analizar

Requiere:
    - venv activado con dependencias instaladas
    - Archivo .env con HF_TOKEN y (opcional) OPENAI_API_KEY
"""

import argparse
import os
import sys
from pathlib import Path

import torch
from dotenv import load_dotenv

from src.asistente_ia import AsistenteIA
from src.pipeline import PipelineDeTranscripcion
from src.transcriptor import AudioTranscriber

load_dotenv()


def detectar_dispositivo() -> str:
    if torch.cuda.is_available():
        print(f"GPU detectada: {torch.cuda.get_device_name(0)} -- usando CUDA")
        return "cuda"
    print("Sin GPU -- usando CPU (mas lento)")
    return "cpu"


def parsear_argumentos():
    parser = argparse.ArgumentParser(
        description="Transcriptor de audio con diarizacion + IA",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos:
  python main.py audio.mp3
  python main.py audio.wav --idioma en --modelo small
  python main.py audio.mp3 --hablantes 2 --resumir --analizar
        """,
    )
    parser.add_argument("audio", help="Ruta al archivo de audio (.mp3, .wav, .m4a, ...)")
    parser.add_argument("--modelo", default="medium", choices=AudioTranscriber.MODELOS_DISPONIBLES)
    parser.add_argument("--idioma", default="es", help="Codigo de idioma (default: es)")
    parser.add_argument("--hf-token", default=None, help="Token HuggingFace (o usar HF_TOKEN en .env)")
    parser.add_argument("--hablantes", type=int, default=None, help="Numero exacto de hablantes")
    parser.add_argument("--min-hablantes", type=int, default=None)
    parser.add_argument("--max-hablantes", type=int, default=None)
    parser.add_argument("--salida", default=None, help="Ruta del .txt de salida")
    parser.add_argument("--prefijo", default="Persona")
    parser.add_argument("--dispositivo", choices=["cpu", "cuda"], default=None)
    parser.add_argument("--limpiar", action="store_true", help="Pasar texto por OpenAI para limpiarlo")
    parser.add_argument("--resumir", action="store_true", help="Generar resumen con OpenAI")
    parser.add_argument("--analizar", action="store_true", help="Generar analisis con OpenAI")
    return parser.parse_args()


def main():
    args = parsear_argumentos()

    dispositivo = args.dispositivo or detectar_dispositivo()
    hf_token = args.hf_token or os.getenv("HF_TOKEN", "")
    openai_key = os.getenv("OPENAI_API_KEY", "")

    if not hf_token:
        print("[Aviso] No hay HF_TOKEN: la transcripcion va a salir sin etiquetar hablantes.")

    pipeline = PipelineDeTranscripcion(
        modelo_whisper=args.modelo,
        idioma=args.idioma,
        hf_token=hf_token,
        dispositivo=dispositivo,
        num_hablantes=args.hablantes,
        min_hablantes=args.min_hablantes,
        max_hablantes=args.max_hablantes,
        prefijo_hablante=args.prefijo,
        progreso=lambda etapa, pct: print(f"  ({pct:3d}%) {etapa}"),
    )

    try:
        out = pipeline.ejecutar(args.audio)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    texto = out["texto"]
    print("\n=== TRANSCRIPCION ===\n")
    print(texto)
    print("\n--- Participacion ---")
    for nombre, cantidad in out["participacion"].items():
        print(f"  {nombre}: {cantidad} segmento(s)")

    # Procesamiento con IA si se pidio
    texto_limpio = resumen = analisis = None
    if args.limpiar or args.resumir or args.analizar:
        if not openai_key:
            print("\n[Aviso] No hay OPENAI_API_KEY en .env -- se saltea el procesamiento IA")
        else:
            ia = AsistenteIA(api_key=openai_key, modelo=os.getenv("OPENAI_MODEL", "gpt-4o-mini"))
            if args.limpiar:
                print("\n[IA] Limpiando texto...")
                texto_limpio = ia.limpiar(texto)
                print("\n=== TEXTO LIMPIO ===\n")
                print(texto_limpio)
            if args.resumir:
                print("\n[IA] Generando resumen...")
                resumen = ia.resumir(texto)
                print("\n=== RESUMEN ===\n")
                print(resumen)
            if args.analizar:
                print("\n[IA] Analizando conversacion...")
                analisis = ia.analizar(texto)
                print("\n=== ANALISIS ===\n")
                print(analisis)

    # Guardar
    base = Path(args.salida) if args.salida else Path(args.audio).with_suffix("")
    base_dir = base.parent if args.salida else Path(args.audio).parent
    nombre = base.stem if args.salida else Path(args.audio).stem

    salida_txt = base_dir / f"{nombre}_transcripcion.txt"
    with open(salida_txt, "w", encoding="utf-8") as f:
        f.write(texto)
    print(f"\nGuardado: {salida_txt}")

    if texto_limpio:
        p = base_dir / f"{nombre}_limpio.txt"
        with open(p, "w", encoding="utf-8") as f:
            f.write(texto_limpio)
        print(f"Guardado: {p}")
    if resumen:
        p = base_dir / f"{nombre}_resumen.txt"
        with open(p, "w", encoding="utf-8") as f:
            f.write(resumen)
        print(f"Guardado: {p}")
    if analisis:
        p = base_dir / f"{nombre}_analisis.txt"
        with open(p, "w", encoding="utf-8") as f:
            f.write(analisis)
        print(f"Guardado: {p}")


if __name__ == "__main__":
    main()
