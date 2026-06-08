#include <QApplication>
#include "ventanaprincipal.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Transcriptor de Audio con IA - POO");
    app.setOrganizationName("UBP");

    VentanaPrincipal ventana;
    ventana.show();

    return app.exec();
}
