#include "motortranscripcion.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

// Extrae el primer objeto JSON balanceado de la salida, ignorando cualquier texto
// antes o despues (las librerias nativas como torch/CUDA a veces imprimen mensajes
// al cerrar el proceso, que quedarian pegados al JSON y romperian el parseo).
static QByteArray extraerJson(const QByteArray &salida)
{
    const int inicio = salida.indexOf('{');
    if (inicio < 0)
        return QByteArray();

    int profundidad = 0;
    bool enString = false;
    bool escape = false;
    for (int i = inicio; i < salida.size(); ++i) {
        const char c = salida.at(i);
        if (enString) {
            if (escape)        escape = false;
            else if (c == '\\') escape = true;
            else if (c == '"')  enString = false;
        } else {
            if (c == '"')       enString = true;
            else if (c == '{')  profundidad++;
            else if (c == '}') {
                if (--profundidad == 0)
                    return salida.mid(inicio, i - inicio + 1);
            }
        }
    }
    return QByteArray();
}

MotorTranscripcion::MotorTranscripcion(QObject *parent)
    : QObject(parent)
{
}

void MotorTranscripcion::setCarpetaMotor(const QString &carpeta)
{
    m_carpeta = carpeta;
}

QString MotorTranscripcion::rutaPython() const
{
    return QDir(m_carpeta).filePath("venv/Scripts/python.exe");
}

QString MotorTranscripcion::rutaCli() const
{
    return QDir(m_carpeta).filePath("cli.py");
}

bool MotorTranscripcion::configuracionValida(QString *motivo) const
{
    if (m_carpeta.isEmpty()) {
        if (motivo) *motivo = "No se configuro la carpeta del motor Python.";
        return false;
    }
    if (!QFileInfo::exists(rutaPython())) {
        if (motivo) *motivo = "No se encontro el interprete Python:\n" + rutaPython();
        return false;
    }
    if (!QFileInfo::exists(rutaCli())) {
        if (motivo) *motivo = "No se encontro cli.py en:\n" + rutaCli();
        return false;
    }
    return true;
}

bool MotorTranscripcion::ocupado() const
{
    return m_proceso != nullptr && m_proceso->state() != QProcess::NotRunning;
}

void MotorTranscripcion::transcribir(const QString &rutaAudio, int personas,
                                     const QString &modelo, const QString &idioma)
{
    QString motivo;
    if (!configuracionValida(&motivo)) {
        emit error(motivo);
        return;
    }
    if (ocupado()) {
        emit error("Ya hay una transcripcion en curso.");
        return;
    }

    m_bufferStderr.clear();

    m_proceso = new QProcess(this);
    m_proceso->setWorkingDirectory(m_carpeta);  // para que 'from src...' y .env funcionen
    m_proceso->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_proceso, &QProcess::readyReadStandardError,
            this, &MotorTranscripcion::alLeerStderr);
    connect(m_proceso, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MotorTranscripcion::alTerminarProceso);
    connect(m_proceso, &QProcess::errorOccurred,
            this, &MotorTranscripcion::alErrorProceso);

    QStringList args;
    args << rutaCli()
         << "--audio" << rutaAudio
         << "--modelo" << modelo
         << "--idioma" << idioma;
    if (personas > 0)
        args << "--personas" << QString::number(personas);

    emit progreso("Iniciando motor de transcripcion...", 1);
    m_proceso->start(rutaPython(), args);
}

void MotorTranscripcion::alLeerStderr()
{
    m_bufferStderr.append(m_proceso->readAllStandardError());
    procesarLineasProgreso();
}

void MotorTranscripcion::procesarLineasProgreso()
{
    // El motor emite lineas "PROGRESO|pct|etapa". Procesamos por lineas completas.
    int idx;
    while ((idx = m_bufferStderr.indexOf('\n')) != -1) {
        QString linea = QString::fromUtf8(m_bufferStderr.left(idx)).trimmed();
        m_bufferStderr.remove(0, idx + 1);

        if (linea.startsWith("PROGRESO|")) {
            const QStringList partes = linea.split('|');
            if (partes.size() >= 3) {
                bool ok = false;
                int pct = partes.at(1).toInt(&ok);
                const QString etapa = partes.mid(2).join('|');
                if (ok)
                    emit progreso(etapa, pct);
            }
        }
        // Otras lineas de stderr son logs de las librerias: se ignoran.
    }
}

void MotorTranscripcion::alTerminarProceso(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);

    const QByteArray salida = m_proceso->readAllStandardOutput();
    m_proceso->deleteLater();
    m_proceso = nullptr;

    const QByteArray json = extraerJson(salida);
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit error(QString("El motor no devolvio un JSON valido (codigo %1).\n%2")
                       .arg(exitCode)
                       .arg(QString::fromUtf8(salida.left(500))));
        return;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value("ok").toBool()) {
        emit error("Error del motor:\n" + obj.value("error").toString());
        return;
    }

    const QString texto = obj.value("texto").toString();

    // Participacion: {"Persona 1": 5, ...} -> "Persona 1: 5 | Persona 2: 3"
    QStringList partes;
    const QJsonObject part = obj.value("participacion").toObject();
    for (auto it = part.begin(); it != part.end(); ++it)
        partes << QString("%1: %2").arg(it.key()).arg(it.value().toInt());

    emit terminado(texto, partes.join("  |  "));
}

void MotorTranscripcion::alErrorProceso(QProcess::ProcessError err)
{
    if (err == QProcess::FailedToStart) {
        emit error("No se pudo iniciar el interprete Python:\n" + rutaPython());
        if (m_proceso) {
            m_proceso->deleteLater();
            m_proceso = nullptr;
        }
    }
}
