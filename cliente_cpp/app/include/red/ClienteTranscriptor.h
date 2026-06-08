#pragma once
#include "red/ClienteHTTP.h"
#include "nucleo/ITranscriptor.h"
#include <utility>
#include <vector>

namespace DIArize::Red {

// Resultado de una operacion de IA: ok=true y texto con la respuesta,
// o ok=false y texto con el mensaje de error.
struct RespuestaIA {
    bool        ok    = false;
    std::string texto;
};

// Herencia multiple: ClienteHTTP (capa de red) + ITranscriptor (interfaz nucleo).
// Implementa transcribir() enviando el audio al servidor Flask local
// y agrega operaciones de IA (limpiar, resumir, analizar, preguntar).
// Demuestra: herencia multiple, override de virtual pura, polimorfismo.
class ClienteTranscriptor : public ClienteHTTP,
                             public DIArize::Core::ITranscriptor {
public:
    explicit ClienteTranscriptor(const QString& urlBase = "http://localhost:8765");

    // ITranscriptor — virtual pura implementada
    DIArize::Core::Transcripcion transcribir(
        const std::string& rutaAudio,
        const std::string& idioma      = "es",
        int                numHablantes = 0) override;

    // ITranscriptor — virtual con cuerpo, sobreescrita
    bool estaDisponible() const override;

    // IA — llaman a /limpiar, /resumir, /analizar en el servidor
    RespuestaIA limpiar (const std::string& texto) const;
    RespuestaIA resumir (const std::string& texto) const;
    RespuestaIA analizar(const std::string& texto) const;

    // Q&A: pregunta sobre el texto, con historial opcional de pares (q,a)
    RespuestaIA preguntar(const std::string& texto,
                          const std::string& pregunta,
                          const std::vector<std::pair<std::string,std::string>>& historial = {}) const;

    // Traduccion: traduce el texto al idioma destino (en, pt, fr, ja, etc.)
    RespuestaIA traducir(const std::string& texto,
                         const std::string& idiomaDestino) const;

protected:
    // ClienteHTTP — virtual sobreescrita: agrega User-Agent
    QNetworkRequest construirRequest(const QUrl& url) const override;

private:
    RespuestaIA _iaSimple(const QString& endpoint, const std::string& texto) const;
};

} // namespace DIArize::Red
