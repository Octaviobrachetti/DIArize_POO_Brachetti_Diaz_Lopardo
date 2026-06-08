#include "red/ClienteTranscriptor.h"
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace DIArize::Red {

ClienteTranscriptor::ClienteTranscriptor(const QString& urlBase)
    : ClienteHTTP(urlBase) {}

QNetworkRequest ClienteTranscriptor::construirRequest(const QUrl& url) const {
    QNetworkRequest req = ClienteHTTP::construirRequest(url); // llama a la base
    req.setRawHeader("User-Agent", "DIArize-CppClient/1.0");
    return req;
}

bool ClienteTranscriptor::estaDisponible() const {
    QByteArray resp = get("/estado");
    if (resp.isEmpty()) return false;
    QJsonDocument doc = QJsonDocument::fromJson(resp);
    return doc.object().value("status").toString() == "ok";
}

DIArize::Core::Transcripcion ClienteTranscriptor::transcribir(
    const std::string& rutaAudio,
    const std::string& idioma,
    int                numHablantes)
{
    DIArize::Core::Transcripcion resultado(rutaAudio, "");

    auto* archivo = new QFile(QString::fromStdString(rutaAudio));
    if (!archivo->open(QIODevice::ReadOnly)) {
        delete archivo;
        return resultado;
    }

    auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Parte 1: archivo de audio
    QHttpPart filePart;
    QString filename = QFileInfo(archivo->fileName()).fileName();
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"audio\"; filename=\"%1\"").arg(filename));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
        QString("audio/mpeg"));
    filePart.setBodyDevice(archivo);
    archivo->setParent(multipart);
    multipart->append(filePart);

    // Parte 2: idioma
    QHttpPart idiomaPart;
    idiomaPart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"idioma\""));
    idiomaPart.setBody(QByteArray::fromStdString(idioma));
    multipart->append(idiomaPart);

    // Parte 3: numero de hablantes (opcional)
    if (numHablantes > 0) {
        QHttpPart habPart;
        habPart.setHeader(QNetworkRequest::ContentDispositionHeader,
            QString("form-data; name=\"num_hablantes\""));
        habPart.setBody(QByteArray::number(numHablantes));
        multipart->append(habPart);
    }

    QByteArray resp = post("/transcribir", multipart);
    if (resp.isEmpty()) return resultado;

    QJsonDocument doc = QJsonDocument::fromJson(resp);
    QJsonObject   obj = doc.object();

    resultado = DIArize::Core::Transcripcion(
        rutaAudio,
        obj["texto"].toString().toStdString());

    for (const QJsonValue& v : obj["segmentos"].toArray()) {
        QJsonObject s = v.toObject();
        resultado.agregarSegmento(DIArize::Core::Segmento(
            s["speaker"].toString().toStdString(),
            s["start"].toDouble(),
            s["end"].toDouble(),
            s["texto"].toString().toStdString()));
    }

    QJsonObject part = obj["participacion"].toObject();
    for (auto it = part.begin(); it != part.end(); ++it) {
        resultado.setParticipacion(it.key().toStdString(), it.value().toDouble());
    }

    return resultado;
}

// ── IA: limpiar / resumir / analizar / preguntar ──────────────────────────

RespuestaIA ClienteTranscriptor::_iaSimple(const QString& endpoint,
                                           const std::string& texto) const
{
    QJsonObject body;
    body["texto"] = QString::fromStdString(texto);
    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QByteArray resp = postJson(endpoint, json);
    if (resp.isEmpty())
        return {false, "Sin respuesta del servidor."};

    QJsonObject obj = QJsonDocument::fromJson(resp).object();
    if (obj.contains("resultado"))
        return {true, obj["resultado"].toString().toStdString()};
    if (obj.contains("error"))
        return {false, obj["error"].toString().toStdString()};
    return {false, "Respuesta inesperada del servidor."};
}

RespuestaIA ClienteTranscriptor::limpiar(const std::string& texto) const {
    return _iaSimple("/limpiar", texto);
}

RespuestaIA ClienteTranscriptor::resumir(const std::string& texto) const {
    return _iaSimple("/resumir", texto);
}

RespuestaIA ClienteTranscriptor::analizar(const std::string& texto) const {
    return _iaSimple("/analizar", texto);
}

RespuestaIA ClienteTranscriptor::traducir(const std::string& texto,
                                           const std::string& idiomaDestino) const
{
    QJsonObject body;
    body["texto"]          = QString::fromStdString(texto);
    body["idioma_destino"] = QString::fromStdString(idiomaDestino);
    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QByteArray resp = postJson("/traducir", json);
    if (resp.isEmpty())
        return {false, "Sin respuesta del servidor."};

    QJsonObject obj = QJsonDocument::fromJson(resp).object();
    if (obj.contains("resultado"))
        return {true, obj["resultado"].toString().toStdString()};
    if (obj.contains("error"))
        return {false, obj["error"].toString().toStdString()};
    return {false, "Respuesta inesperada del servidor."};
}

RespuestaIA ClienteTranscriptor::preguntar(
    const std::string& texto,
    const std::string& pregunta,
    const std::vector<std::pair<std::string,std::string>>& historial) const
{
    QJsonObject body;
    body["texto"]    = QString::fromStdString(texto);
    body["pregunta"] = QString::fromStdString(pregunta);

    QJsonArray hist;
    for (const auto& par : historial) {
        QJsonArray dupla;
        dupla.append(QString::fromStdString(par.first));
        dupla.append(QString::fromStdString(par.second));
        hist.append(dupla);
    }
    body["historial"] = hist;

    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QByteArray resp = postJson("/preguntar", json);
    if (resp.isEmpty())
        return {false, "Sin respuesta del servidor."};

    QJsonObject obj = QJsonDocument::fromJson(resp).object();
    if (obj.contains("resultado"))
        return {true, obj["resultado"].toString().toStdString()};
    if (obj.contains("error"))
        return {false, obj["error"].toString().toStdString()};
    return {false, "Respuesta inesperada del servidor."};
}

} // namespace DIArize::Red
