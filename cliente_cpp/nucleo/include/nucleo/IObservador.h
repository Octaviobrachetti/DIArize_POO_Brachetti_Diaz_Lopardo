#pragma once
#include <string>

namespace DIArize::Core {

enum class EstadoApp { Inactivo, Subiendo, Procesando, Listo, Error };

// Clase abstracta: patron Observer.
// VentanaPrincipal lo implementara via herencia multiple (Paso 5).
class IObservador {
public:
    virtual ~IObservador() = default;

    // Funciones virtuales puras: toda subclase debe implementarlas
    virtual void onEstadoCambiado(EstadoApp estado, const std::string& detalle) = 0;
    virtual void onProgreso(int porcentaje, const std::string& etapa) = 0;
};

} // namespace DIArize::Core
