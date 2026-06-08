#ifndef MOTORTRANSCRIPCION_H
#define MOTORTRANSCRIPCION_H

#include <QObject>
#include <QProcess>
#include <QString>

/**
 * @brief Orquesta la transcripcion llamando al motor Python (cli.py) por QProcess.
 *
 * Esta clase es el "puente" entre la app C++ y el pipeline de WhisperX (que sigue
 * en Python). Lanza el interprete del venv, le pasa el audio y la cantidad de
 * personas, lee el progreso por STDERR y el resultado (JSON) por STDOUT.
 *
 * No bloquea la interfaz: trabaja con senales asincronicas de QProcess.
 */
class MotorTranscripcion : public QObject
{
    Q_OBJECT

public:
    explicit MotorTranscripcion(QObject *parent = nullptr);

    /** Carpeta del proyecto Python (contiene cli.py, src/, venv/, .env). */
    void setCarpetaMotor(const QString &carpeta);
    QString carpetaMotor() const { return m_carpeta; }

    /** Rutas derivadas de la carpeta del motor. */
    QString rutaPython() const;   // venv/Scripts/python.exe
    QString rutaCli() const;      // cli.py
    bool configuracionValida(QString *motivo = nullptr) const;

    bool ocupado() const;

    /** Arranca la transcripcion. Emite progreso/terminado/error. */
    void transcribir(const QString &rutaAudio, int personas,
                     const QString &modelo, const QString &idioma);

signals:
    void progreso(const QString &etapa, int pct);
    void terminado(const QString &texto, const QString &participacion);
    void error(const QString &mensaje);

private slots:
    void alLeerStderr();
    void alTerminarProceso(int exitCode, QProcess::ExitStatus status);
    void alErrorProceso(QProcess::ProcessError err);

private:
    QString m_carpeta;
    QProcess *m_proceso = nullptr;
    QByteArray m_bufferStderr;

    void procesarLineasProgreso();
};

#endif // MOTORTRANSCRIPCION_H
