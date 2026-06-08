"""Asistente de IA basado en OpenAI: resumen, limpieza, analisis y Q&A."""

from typing import Optional

from openai import OpenAI


class AsistenteIA:
    """
    Cliente de OpenAI con cuatro operaciones especializadas sobre una transcripcion:

      - resumir(texto)      : resumen breve de la conversacion
      - limpiar(texto)      : sacar muletillas/falsos arranques SIN perder informacion
      - analizar(texto)     : temas tratados, tono, acciones, conclusiones
      - preguntar(texto, q) : Q&A sobre el contenido del audio
    """

    INSTRUCCION_LIMPIEZA = (
        "Sos un editor de texto. Recibis una transcripcion automatica de un audio "
        "con etiquetas tipo 'Persona 1:' o 'Persona 2:'. Tu tarea es UNICAMENTE limpiar "
        "el texto: eliminar muletillas ('eh', 'em', 'este', 'o sea' repetidos), "
        "corregir falsos arranques, arreglar puntuacion y mayusculas, "
        "y unir frases entrecortadas. NO RESUMAS, NO REINTERPRETES, NO AGREGUES INFORMACION. "
        "El contenido y las ideas tienen que quedar identicos. Manteneme las etiquetas de "
        "persona y los timestamps si estan presentes. Devolve solo el texto limpio."
    )

    INSTRUCCION_RESUMEN = (
        "Sos un asistente que resume conversaciones transcritas. Te paso una transcripcion "
        "con etiquetas 'Persona 1', 'Persona 2', etc. Generá un resumen en espanol, claro y conciso, "
        "que incluya: (1) tema principal, (2) puntos clave que mencionó cada persona, "
        "(3) conclusiones o acuerdos si los hay. Maximo 200 palabras."
    )

    INSTRUCCION_ANALISIS = (
        "Sos un analista. Recibis una transcripcion de audio con etiquetas 'Persona 1', etc. "
        "Devolve un analisis estructurado en espanol con estas secciones:\n"
        "- Temas tratados (lista)\n"
        "- Tono general de la conversacion\n"
        "- Postura/rol de cada persona\n"
        "- Acciones a tomar o pendientes (si se mencionan)\n"
        "- Observaciones interesantes\n"
        "Se directo y objetivo, sin inventar."
    )

    INSTRUCCION_QA = (
        "Sos un asistente que responde preguntas sobre una transcripcion de audio. "
        "Usa SOLO la informacion del audio que se te pasa como contexto. Si la respuesta "
        "no esta en el audio, deci 'No se mencionó en el audio'. Responde en espanol, breve."
    )

    def __init__(
        self,
        api_key: str,
        modelo: str = "gemini-2.5-flash",
        base_url: str = "https://generativelanguage.googleapis.com/v1beta/openai/",
    ):
        """
        Cliente LLM. Por defecto usa Google Gemini a traves de su endpoint
        compatible con OpenAI (gratis). Para usar OpenAI real, pasar base_url=None
        y un modelo tipo 'gpt-4o-mini'.
        """
        if not api_key:
            raise ValueError(
                "Se necesita una API key. Para Gemini (gratis) creala en "
                "https://aistudio.google.com/app/apikey y ponela en .env (OPENAI_API_KEY)."
            )
        self.modelo = modelo
        self._cliente = OpenAI(api_key=api_key, base_url=base_url) if base_url else OpenAI(api_key=api_key)

    def _chat(self, instruccion_sistema: str, contenido_usuario: str, temperatura: float = 0.3) -> str:
        respuesta = self._cliente.chat.completions.create(
            model=self.modelo,
            messages=[
                {"role": "system", "content": instruccion_sistema},
                {"role": "user", "content": contenido_usuario},
            ],
            temperature=temperatura,
        )
        return respuesta.choices[0].message.content.strip()

    def limpiar(self, transcripcion: str) -> str:
        """Devuelve la transcripcion limpia, sin perder contenido."""
        return self._chat(self.INSTRUCCION_LIMPIEZA, transcripcion, temperatura=0.1)

    def resumir(self, transcripcion: str) -> str:
        """Devuelve un resumen breve de la conversacion."""
        return self._chat(self.INSTRUCCION_RESUMEN, transcripcion, temperatura=0.3)

    def analizar(self, transcripcion: str) -> str:
        """Devuelve un analisis estructurado (temas, tono, acciones, etc.)."""
        return self._chat(self.INSTRUCCION_ANALISIS, transcripcion, temperatura=0.3)

    def preguntar(self, transcripcion: str, pregunta: str, historial: Optional[list] = None) -> str:
        """
        Responde una pregunta sobre el contenido del audio.

        Args:
            transcripcion: Texto de la transcripcion (contexto)
            pregunta: Pregunta del usuario
            historial: Lista opcional de tuplas (pregunta, respuesta) anteriores
        """
        mensajes = [
            {"role": "system", "content": self.INSTRUCCION_QA},
            {"role": "user", "content": f"--- TRANSCRIPCION DEL AUDIO ---\n{transcripcion}\n--- FIN ---"},
        ]
        if historial:
            for q, a in historial:
                mensajes.append({"role": "user", "content": q})
                mensajes.append({"role": "assistant", "content": a})
        mensajes.append({"role": "user", "content": pregunta})

        respuesta = self._cliente.chat.completions.create(
            model=self.modelo,
            messages=mensajes,
            temperature=0.2,
        )
        return respuesta.choices[0].message.content.strip()
