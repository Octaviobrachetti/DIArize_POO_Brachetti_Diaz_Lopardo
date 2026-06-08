/****************************************************************************
** Meta object code from reading C++ file 'VentanaPrincipal.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../app/include/gui/VentanaPrincipal.h"
#include <QtGui/qtextcursor.h>
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VentanaPrincipal.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t {};
} // unnamed namespace

template <> constexpr inline auto DIArize::GUI::VentanaPrincipal::qt_create_metaobjectdata<qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DIArize::GUI::VentanaPrincipal",
        "_seleccionarArchivo",
        "",
        "_quitarAudio",
        "_toggleGrabacion",
        "_tickGrabacion",
        "_toggleReproduccion",
        "_transcribir",
        "_onTranscripcionLista",
        "_limpiar",
        "_resumir",
        "_analizar",
        "_traducir",
        "_preguntar",
        "_onLimpiarListo",
        "_onResumirListo",
        "_onAnalizarListo",
        "_onTraducirListo",
        "_onPreguntarListo",
        "_guardarTodo",
        "_verificarServidor",
        "_cambiarAModoArchivo",
        "_cambiarAModoEnVivo",
        "_procesarWavDeEnVivo",
        "rutaWav"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot '_seleccionarArchivo'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_quitarAudio'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_toggleGrabacion'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_tickGrabacion'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_toggleReproduccion'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_transcribir'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onTranscripcionLista'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_limpiar'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_resumir'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_analizar'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_traducir'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_preguntar'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onLimpiarListo'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onResumirListo'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onAnalizarListo'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onTraducirListo'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_onPreguntarListo'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_guardarTodo'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_verificarServidor'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_cambiarAModoArchivo'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_cambiarAModoEnVivo'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot '_procesarWavDeEnVivo'
        QtMocHelpers::SlotData<void(const QString &)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<VentanaPrincipal, qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DIArize::GUI::VentanaPrincipal::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>.metaTypes,
    nullptr
} };

void DIArize::GUI::VentanaPrincipal::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<VentanaPrincipal *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->_seleccionarArchivo(); break;
        case 1: _t->_quitarAudio(); break;
        case 2: _t->_toggleGrabacion(); break;
        case 3: _t->_tickGrabacion(); break;
        case 4: _t->_toggleReproduccion(); break;
        case 5: _t->_transcribir(); break;
        case 6: _t->_onTranscripcionLista(); break;
        case 7: _t->_limpiar(); break;
        case 8: _t->_resumir(); break;
        case 9: _t->_analizar(); break;
        case 10: _t->_traducir(); break;
        case 11: _t->_preguntar(); break;
        case 12: _t->_onLimpiarListo(); break;
        case 13: _t->_onResumirListo(); break;
        case 14: _t->_onAnalizarListo(); break;
        case 15: _t->_onTraducirListo(); break;
        case 16: _t->_onPreguntarListo(); break;
        case 17: _t->_guardarTodo(); break;
        case 18: _t->_verificarServidor(); break;
        case 19: _t->_cambiarAModoArchivo(); break;
        case 20: _t->_cambiarAModoEnVivo(); break;
        case 21: _t->_procesarWavDeEnVivo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *DIArize::GUI::VentanaPrincipal::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DIArize::GUI::VentanaPrincipal::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7DIArize3GUI16VentanaPrincipalE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "DIArize::Core::IObservador"))
        return static_cast< DIArize::Core::IObservador*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int DIArize::GUI::VentanaPrincipal::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }
    return _id;
}
QT_WARNING_POP
