#pragma once
#include "red/ClienteHTTP.h"
#include "nucleo/ITranscriptor.h"

namespace DIArize::Red {

// Herencia multiple: ClienteHTTP (capa de red) + ITranscriptor (interfaz nucleo).
// Implementa transcribir() enviando el audio al servidor Flask local.
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

protected:
    // ClienteHTTP — virtual sobreescrita: agrega User-Agent
    QNetworkRequest construirRequest(const QUrl& url) const override;
};

} // namespace DIArize::Red
