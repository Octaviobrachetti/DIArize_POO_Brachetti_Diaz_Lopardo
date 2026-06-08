#include "nucleo/Segmento.h"

namespace DIArize::Core {

Segmento::Segmento(const std::string& speaker,
                   double inicio, double fin,
                   const std::string& texto)
    : _speaker(speaker), _inicio(inicio), _fin(fin), _texto(texto) {}

const std::string& Segmento::getSpeaker()  const { return _speaker; }
double             Segmento::getInicio()   const { return _inicio; }
double             Segmento::getFin()      const { return _fin; }
double             Segmento::getDuracion() const { return _fin - _inicio; }
const std::string& Segmento::getTexto()   const { return _texto; }

} // namespace DIArize::Core
