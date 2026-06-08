#pragma once
#include <string>
#include "Transcripcion.h"

namespace DIArize::Core {

// Clase abstracta: contrato que debe cumplir cualquier implementacion
// de transcripcion (local, cloud, HTTP...).
// No se puede instanciar directamente.
class ITranscriptor {
public:
    virtual ~ITranscriptor() = default;

    // Funcion virtual pura: obliga a las subclases a implementarla
    virtual Transcripcion transcribir(const std::string& rutaAudio,
                                      const std::string& idioma       = "es",
                                      int                numHablantes = 0) = 0;

    // Funcion virtual con implementacion por defecto: las subclases pueden sobreescribir
    virtual bool estaDisponible() const { return true; }
};

} // namespace DIArize::Core
