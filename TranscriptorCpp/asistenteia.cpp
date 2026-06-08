#include "asistenteia.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

const QString AsistenteIA::INSTRUCCION_LIMPIEZA =
    "Sos un editor de texto. Recibis una transcripcion automatica de un audio "
    "con etiquetas tipo 'Persona 1:' o 'Persona 2:'. Tu tarea es UNICAMENTE limpiar "
    "el texto: eliminar muletillas ('eh', 'em', 'este', 'o sea' repetidos), "
    "corregir falsos arranques, arreglar puntuacion y mayusculas, "
    "y unir frases entrecortadas. NO RESUMAS, NO REINTERPRETES, NO AGREGUES INFORMACION. "
    "El contenido y las ideas tienen que quedar identicos. Manteneme las etiquetas de "
    "persona y los timestamps si estan presentes. Devolve solo el texto limpio.";

const QString AsistenteIA::INSTRUCCION_RESUMEN =
    "Sos un asistente que resume conversaciones transcritas. Te paso una transcripcion "
    "con etiquetas 'Persona 1', 'Persona 2', etc. Genera un resumen en espanol, claro y conciso, "
    "que incluya: (1) tema principal, (2) puntos clave que menciono cada persona, "
    "(3) conclusiones o acuerdos si los hay. Maximo 200 palabras.";

const QString AsistenteIA::INSTRUCCION_ANALISIS =
    "Sos un analista. Recibis una transcripcion de audio con etiquetas 'Persona 1', etc. "
    "Devolve un analisis estructurado en espanol con estas secciones:\n"
    "- Temas tratados (lista)\n"
    "- Tono general de la conversacion\n"
    "- Postura/rol de cada persona\n"
    "- Acciones a tomar o pendientes (si se mencionan)\n"
    "- Observaciones interesantes\n"
    "Se directo y objetivo, sin inventar.";

const QString AsistenteIA::INSTRUCCION_QA =
    "Sos un asistente que responde preguntas sobre una transcripcion de audio. "
    "Usa SOLO la informacion del audio que se te pasa como contexto. Si la respuesta "
    "no esta en el audio, deci 'No se menciono en el audio'. Responde en espanol, breve.";

AsistenteIA::AsistenteIA(const QString &apiKey, const QString &modelo,
                         const QString &urlEndpoint, QObject *parent)
    : QObject(parent), m_apiKey(apiKey), m_modelo(modelo), m_urlEndpoint(urlEndpoint)
{
    m_net = new QNetworkAccessManager(this);
}

QJsonObject AsistenteIA::mensaje(const QString &rol, const QString &contenido)
{
    QJsonObject m;
    m["role"] = rol;
    m["content"] = contenido;
    return m;
}

void AsistenteIA::limpiar(const QString &transcripcion)
{
    enviar("limpieza", INSTRUCCION_LIMPIEZA, transcripcion, 0.1);
}

void AsistenteIA::resumir(const QString &transcripcion)
{
    enviar("resumen", INSTRUCCION_RESUMEN, transcripcion, 0.3);
}

void AsistenteIA::analizar(const QString &transcripcion)
{
    enviar("analisis", INSTRUCCION_ANALISIS, transcripcion, 0.3);
}

void AsistenteIA::preguntar(const QString &transcripcion, const QString &pregunta,
                            const QVector<QPair<QString, QString>> &historial)
{
    QJsonArray mensajes;
    mensajes.append(mensaje("system", INSTRUCCION_QA));
    mensajes.append(mensaje("user",
        "--- TRANSCRIPCION DEL AUDIO ---\n" + transcripcion + "\n--- FIN ---"));
    for (const auto &qa : historial) {
        mensajes.append(mensaje("user", qa.first));
        mensajes.append(mensaje("assistant", qa.second));
    }
    mensajes.append(mensaje("user", pregunta));
    enviarMensajes("qa", mensajes, 0.2);
}

void AsistenteIA::enviar(const QString &tipo, const QString &instruccionSistema,
                         const QString &contenidoUsuario, double temperatura)
{
    QJsonArray mensajes;
    mensajes.append(mensaje("system", instruccionSistema));
    mensajes.append(mensaje("user", contenidoUsuario));
    enviarMensajes(tipo, mensajes, temperatura);
}

void AsistenteIA::enviarMensajes(const QString &tipo, const QJsonArray &mensajes,
                                 double temperatura)
{
    if (!disponible()) {
        emit errorIA(tipo, "No hay OPENAI_API_KEY configurada.");
        return;
    }

    QJsonObject cuerpo;
    cuerpo["model"] = m_modelo;
    cuerpo["messages"] = mensajes;
    cuerpo["temperature"] = temperatura;

    QNetworkRequest req((QUrl(m_urlEndpoint)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    QNetworkReply *reply = m_net->post(req, QJsonDocument(cuerpo).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, tipo]() {
        reply->deleteLater();
        const QByteArray datos = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            // Intentamos sacar el mensaje de error que devuelve OpenAI
            QString detalle = QString::fromUtf8(datos);
            const QJsonObject obj = QJsonDocument::fromJson(datos).object();
            if (obj.contains("error"))
                detalle = obj.value("error").toObject().value("message").toString();
            emit errorIA(tipo, "Error de la IA: " + detalle);
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(datos).object();
        const QJsonArray choices = obj.value("choices").toArray();
        if (choices.isEmpty()) {
            emit errorIA(tipo, "Respuesta vacia de la IA.");
            return;
        }
        const QString contenido = choices.first().toObject()
                                      .value("message").toObject()
                                      .value("content").toString().trimmed();
        emit respuestaLista(tipo, contenido);
    });
}
