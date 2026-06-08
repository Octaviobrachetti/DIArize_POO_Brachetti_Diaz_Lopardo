#include "red/ClienteStreaming.h"

#include <QAudioDevice>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

namespace DIArize::Red {

ClienteStreaming::ClienteStreaming(QObject* parent) : QObject(parent) {}

ClienteStreaming::~ClienteStreaming() {
    detener();
}

bool ClienteStreaming::estaActivo() const { return _activo; }

// ── WAV ─────────────────────────────────────────────────────────────
// Escribimos un .wav PCM 16-bit mono 16kHz mientras streameamos para
// despues poder re-transcribirlo con diarizacion (que no soporta el modo en vivo).

static void _writeLE16(QFile& f, quint16 v) { char b[2] = {char(v & 0xFF), char((v >> 8) & 0xFF)}; f.write(b, 2); }
static void _writeLE32(QFile& f, quint32 v) {
    char b[4] = {char(v & 0xFF), char((v >> 8) & 0xFF), char((v >> 16) & 0xFF), char((v >> 24) & 0xFF)};
    f.write(b, 4);
}

void ClienteStreaming::_abrirWavParaEscritura() {
    QString carpeta = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString nombre  = QString("diarize_envivo_%1.wav")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    _rutaWav = QDir(carpeta).absoluteFilePath(nombre);

    _wavFile.setFileName(_rutaWav);
    if (!_wavFile.open(QIODevice::WriteOnly)) {
        _rutaWav.clear();
        return;
    }
    _bytesPCMEscritos = 0;

    // Cabecera WAV (44 bytes) — campos de tamano se actualizan al cerrar
    _wavFile.write("RIFF", 4);
    _writeLE32(_wavFile, 0);              // chunk size (lo arreglamos al final)
    _wavFile.write("WAVE", 4);
    _wavFile.write("fmt ", 4);
    _writeLE32(_wavFile, 16);             // PCM fmt chunk size
    _writeLE16(_wavFile, 1);              // formato PCM
    _writeLE16(_wavFile, 1);              // canales: mono
    _writeLE32(_wavFile, 16000);          // sample rate
    _writeLE32(_wavFile, 16000 * 2);      // byte rate = sr * canales * bytes/sample
    _writeLE16(_wavFile, 2);              // block align
    _writeLE16(_wavFile, 16);             // bits per sample
    _wavFile.write("data", 4);
    _writeLE32(_wavFile, 0);              // data size (lo arreglamos al final)
}

void ClienteStreaming::_escribirPCMaWav(const QByteArray& pcm) {
    if (!_wavFile.isOpen()) return;
    _wavFile.write(pcm);
    _bytesPCMEscritos += pcm.size();
}

void ClienteStreaming::_cerrarWav() {
    if (!_wavFile.isOpen()) return;
    // Ahora que sabemos el tamano total, actualizamos los dos campos
    _wavFile.seek(4);
    _writeLE32(_wavFile, 36 + _bytesPCMEscritos);   // RIFF chunk size
    _wavFile.seek(40);
    _writeLE32(_wavFile, _bytesPCMEscritos);        // data chunk size
    _wavFile.close();
    // Si grabamos menos de 0.5s no sirve para diarizar
    if (_bytesPCMEscritos < 16000) _rutaWav.clear();
}

void ClienteStreaming::iniciar(const QString& token) {
    if (_activo) return;
    _activo = true;

    // ── Conexion WebSocket a AssemblyAI Streaming v3 ─────────────────
    if (!_ws) {
        _ws = new QWebSocket();
        connect(_ws, &QWebSocket::connected,
                this, &ClienteStreaming::_onConectado);
        connect(_ws, &QWebSocket::textMessageReceived,
                this, &ClienteStreaming::_onMensaje);
        connect(_ws, &QWebSocket::errorOccurred,
                this, &ClienteStreaming::_onError);
        connect(_ws, &QWebSocket::disconnected,
                this, &ClienteStreaming::_onDesconectado);
    }

    QUrl url("wss://streaming.assemblyai.com/v3/ws");
    QUrlQuery q;
    q.addQueryItem("sample_rate", "16000");
    q.addQueryItem("format_turns", "true");
    q.addQueryItem("token", token);
    url.setQuery(q);

    _ws->open(url);
}

void ClienteStreaming::detener() {
    if (!_activo) return;
    _activo = false;

    if (_audio) {
        _audio->stop();
        _audio->deleteLater();
        _audio = nullptr;
        _audioIO = nullptr;
    }
    _buffer.clear();
    _cerrarWav();
    if (_ws && _ws->state() != QAbstractSocket::UnconnectedState) {
        _ws->sendTextMessage(QStringLiteral("{\"type\":\"Terminate\"}"));
        _ws->close();
    }
    emit finalizado();
}

void ClienteStreaming::_onConectado() {
    emit conectado();

    // ── Abrir microfono: 16kHz mono 16-bit PCM (lo que pide AssemblyAI) ──
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice dev = QMediaDevices::defaultAudioInput();
    if (dev.isNull()) {
        emit error("No se detecto microfono.");
        detener();
        return;
    }
    if (!dev.isFormatSupported(fmt)) {
        // Igual lo intentamos: Qt suele resamplear internamente
    }

    _abrirWavParaEscritura();

    _audio = new QAudioSource(dev, fmt, this);
    _audioIO = _audio->start();  // QIODevice de lectura: emite readyRead

    if (!_audioIO) {
        emit error("No se pudo abrir el microfono.");
        detener();
        return;
    }
    connect(_audioIO, &QIODevice::readyRead,
            this, &ClienteStreaming::_onAudioListo);
}

void ClienteStreaming::_onAudioListo() {
    if (!_audioIO || !_ws || _ws->state() != QAbstractSocket::ConnectedState)
        return;
    QByteArray pcmNuevo = _audioIO->readAll();
    _escribirPCMaWav(pcmNuevo);    // grabacion paralela a WAV para diarizar despues
    _buffer.append(pcmNuevo);

    // AssemblyAI exige chunks de 50-1000ms. Acumulamos hasta tener al menos
    // 100ms (3200 bytes a 16kHz mono 16-bit) antes de enviar.
    while (_buffer.size() >= _minChunkBytes) {
        QByteArray chunk = _buffer.left(_minChunkBytes);
        _buffer.remove(0, _minChunkBytes);
        _ws->sendBinaryMessage(chunk);
    }
}

void ClienteStreaming::_onMensaje(const QString& msg) {
    QJsonObject obj = QJsonDocument::fromJson(msg.toUtf8()).object();
    QString tipo = obj.value("type").toString();

    if (tipo == "Turn") {
        // En v3 los Turns traen "transcript" y "end_of_turn"
        QString texto = obj.value("transcript").toString();
        bool finalizado = obj.value("end_of_turn").toBool();
        if (texto.isEmpty()) return;
        if (finalizado) emit textoFinal(texto);
        else            emit textoParcial(texto);
    } else if (tipo == "Begin") {
        // Sesion iniciada, nada que hacer
    } else if (tipo == "Termination") {
        emit finalizado();
    } else if (tipo == "Error") {
        emit error(obj.value("error").toString());
    }
}

void ClienteStreaming::_onError(QAbstractSocket::SocketError) {
    if (_ws) emit error(_ws->errorString());
}

void ClienteStreaming::_onDesconectado() {
    if (_audio) {
        _audio->stop();
        _audio->deleteLater();
        _audio = nullptr;
        _audioIO = nullptr;
    }
    _cerrarWav();
    _activo = false;
}

} // namespace DIArize::Red
