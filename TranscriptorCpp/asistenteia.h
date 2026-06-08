#ifndef ASISTENTEIA_H
#define ASISTENTEIA_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>

class QNetworkAccessManager;

/**
 * @brief Cliente de OpenAI (Chat Completions) usando Qt Network.
 *
 * Equivale al AsistenteIA de Python pero 100% en C++: arma el pedido HTTPS,
 * lo manda asincronicamente y emite la respuesta. Cuatro operaciones sobre la
 * transcripcion: limpiar, resumir, analizar y preguntar (Q&A).
 *
 * La clave de OpenAI se pasa en el constructor (se lee del entorno o del .env).
 */
class AsistenteIA : public QObject
{
    Q_OBJECT

public:
    // Por defecto usa Google Gemini via su endpoint compatible con OpenAI (gratis).
    explicit AsistenteIA(const QString &apiKey,
                         const QString &modelo = "gemini-2.5-flash",
                         const QString &urlEndpoint =
                             "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions",
                         QObject *parent = nullptr);

    bool disponible() const { return !m_apiKey.isEmpty(); }

    void limpiar(const QString &transcripcion);
    void resumir(const QString &transcripcion);
    void analizar(const QString &transcripcion);
    void preguntar(const QString &transcripcion, const QString &pregunta,
                   const QVector<QPair<QString, QString>> &historial);

signals:
    /** tipo ∈ {"limpieza","resumen","analisis","qa"} */
    void respuestaLista(const QString &tipo, const QString &contenido);
    void errorIA(const QString &tipo, const QString &mensaje);

private:
    QString m_apiKey;
    QString m_modelo;
    QString m_urlEndpoint;
    QNetworkAccessManager *m_net;

    // Instrucciones de sistema (mismas que la version Python)
    static const QString INSTRUCCION_LIMPIEZA;
    static const QString INSTRUCCION_RESUMEN;
    static const QString INSTRUCCION_ANALISIS;
    static const QString INSTRUCCION_QA;

    /** Helper: arma un mensaje {role, content}. */
    static QJsonObject mensaje(const QString &rol, const QString &contenido);
    /** Envia un chat con system + user simple. */
    void enviar(const QString &tipo, const QString &instruccionSistema,
                const QString &contenidoUsuario, double temperatura);
    /** Envia una lista de mensajes ya armada (para el Q&A con historial). */
    void enviarMensajes(const QString &tipo, const QJsonArray &mensajes,
                        double temperatura);
};

#endif // ASISTENTEIA_H
