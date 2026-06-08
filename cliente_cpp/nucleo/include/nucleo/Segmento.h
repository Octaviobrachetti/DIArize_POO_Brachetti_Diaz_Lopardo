#pragma once
#include <string>

namespace DIArize::Core {

class Segmento {
public:
    Segmento(const std::string& speaker,
             double inicio,
             double fin,
             const std::string& texto);

    const std::string& getSpeaker()  const;
    double             getInicio()   const;
    double             getFin()      const;
    double             getDuracion() const;   // fin - inicio
    const std::string& getTexto()    const;

private:
    std::string _speaker;
    double      _inicio;
    double      _fin;
    std::string _texto;
};

} // namespace DIArize::Core
