#pragma once
#include <cstddef>
#include <list>
#include "Transcripcion.h"

namespace DIArize::Core {

// Historial de transcripciones en memoria.
// Usa std::list<T>: inserciones O(1) y recorrido secuencial eficiente.
class Repositorio {
public:
    void agregar(const Transcripcion& t)                  { _historial.push_back(t); }
    const std::list<Transcripcion>& getHistorial()  const { return _historial; }
    std::size_t                     tamanio()        const { return _historial.size(); }
    void                            limpiar()              { _historial.clear(); }

private:
    std::list<Transcripcion> _historial;   // list<T>
};

} // namespace DIArize::Core
