#pragma once
#include <string>

namespace DIArize::Core {

// Encapsulamiento: atributos privados, interfaz publica con getters/setters.
class Hablante {
public:
    Hablante(const std::string& id, const std::string& nombre = "");

    const std::string& getId()     const;
    const std::string& getNombre() const;
    void               setNombre(const std::string& nombre);

    double getTiempoTotal()          const;
    void   agregarTiempo(double seg);

private:
    std::string _id;
    std::string _nombre;
    double      _tiempoTotal { 0.0 };
};

} // namespace DIArize::Core
