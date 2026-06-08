#pragma once
#include <QAudioFormat>
#include <QAudioSource>
#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <QWebSocket>
#include <memory>

namespace DIArize::Red {

/**
 * Cliente de transcripcion en vivo via AssemblyAI Streaming v3.
 *
 * Flujo:
 *   1) iniciar(token): conecta el WebSocket usando el token efimero.
 *   2) Al recibir senal conectado(): abre QAudioSource y empieza a stremear
 *      PCM 16-bit mono 16kHz por el WebSocket.
 *   3) Cada vez que llega texto de AssemblyAI emite:
 *        textoParcial(QString)  -> palabras provisorias (parpadean)
 *        textoFinal(QString)    -> frase confirmada
 *   4) detener(): cierra audio + WebSocket, emite finalizado().
 */
class ClienteStreaming : public QObject {
    Q_OBJECT
public:
    explicit ClienteStreaming(QObject* parent = nullptr);
    ~ClienteStreaming() override;

    void iniciar(const QString& token);
    void detener();
    bool estaActivo() const;

    // Ruta del WAV generado mientras streameaba. Vacia si no se grabo nada.
    QString rutaWav() const { return _rutaWav; }

signals:
    void conectado();
    void textoParcial(const QString& texto);
    void textoFinal(const QString& texto);
    void error(const QString& mensaje);
    void finalizado();

private slots:
    void _onConectado();
    void _onMensaje(const QString& msg);
    void _onError(QAbstractSocket::SocketError err);
    void _onDesconectado();
    void _onAudioListo();

private:
    QWebSocket*                    _ws = nullptr;
    QAudioSource*                  _audio = nullptr;
    QIODevice*                     _audioIO = nullptr;
    bool                           _activo = false;
    QByteArray                     _buffer;          // acumula PCM hasta minChunkBytes
    // 100 ms a 16kHz mono 16-bit = 16000 * 0.1 * 2 = 3200 bytes
    static constexpr int           _minChunkBytes = 3200;

    // Grabacion paralela a WAV para diarizacion posterior
    QFile                          _wavFile;
    QString                        _rutaWav;
    quint32                        _bytesPCMEscritos = 0;

    void _abrirWavParaEscritura();
    void _escribirPCMaWav(const QByteArray& pcm);
    void _cerrarWav();
};

} // namespace DIArize::Red
