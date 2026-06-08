#include "nucleo/Hablante.h"

namespace DIArize::Core {

Hablante::Hablante(const std::string& id, const std::string& nombre)
    : _id(id), _nombre(nombre), _tiempoTotal(0.0) {}

const std::string& Hablante::getId()     const { return _id; }
const std::string& Hablante::getNombre() const { return _nombre; }
void Hablante::setNombre(const std::string& n) { _nombre = n; }

double Hablante::getTiempoTotal()          const { return _tiempoTotal; }
void   Hablante::agregarTiempo(double seg)       { _tiempoTotal += seg; }

} // namespace DIArize::Core
