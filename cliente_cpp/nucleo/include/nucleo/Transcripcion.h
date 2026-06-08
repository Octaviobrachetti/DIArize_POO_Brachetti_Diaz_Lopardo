#pragma once
#include <map>
#include <string>
#include <vector>
#include "Segmento.h"

namespace DIArize::Core {

class Transcripcion {
public:
    Transcripcion(const std::string& archivo, const std::string& textoCompleto);

    // Mutadores
    void agregarSegmento(const Segmento& seg);
    void setParticipacion(const std::string& speaker, double segundos);

    // Accessores (const: no modifican el objeto)
    const std::string&                   getArchivo()       const;
    const std::string&                   getTexto()         const;
    const std::string&                   getFecha()         const;
    const std::vector<Segmento>&         getSegmentos()     const;  // vector<T>
    const std::map<std::string, double>& getParticipacion() const;

private:
    std::string                   _archivo;
    std::string                   _texto;
    std::string                   _fecha;           // timestamp de creacion
    std::vector<Segmento>         _segmentos;       // vector<Segmento>
    std::map<std::string, double> _participacion;
};

} // namespace DIArize::Core
