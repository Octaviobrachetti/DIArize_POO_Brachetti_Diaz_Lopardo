#include "nucleo/Transcripcion.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace DIArize::Core {

Transcripcion::Transcripcion(const std::string& archivo,
                             const std::string& textoCompleto)
    : _archivo(archivo), _texto(textoCompleto)
{
    auto ahora = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(ahora);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M");
    _fecha = ss.str();
}

void Transcripcion::agregarSegmento(const Segmento& seg) {
    _segmentos.push_back(seg);
}

void Transcripcion::setParticipacion(const std::string& speaker, double segundos) {
    _participacion[speaker] = segundos;
}

const std::string& Transcripcion::getArchivo()   const { return _archivo; }
const std::string& Transcripcion::getTexto()     const { return _texto; }
const std::string& Transcripcion::getFecha()     const { return _fecha; }

const std::vector<Segmento>& Transcripcion::getSegmentos() const {
    return _segmentos;
}

const std::map<std::string, double>& Transcripcion::getParticipacion() const {
    return _participacion;
}

} // namespace DIArize::Core
