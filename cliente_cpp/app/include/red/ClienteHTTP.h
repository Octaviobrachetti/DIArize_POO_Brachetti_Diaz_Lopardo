#pragma once
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>

namespace DIArize::Red {

// Clase base para clientes HTTP.
// Encapsula QNetworkAccessManager y ofrece GET/POST sincronos via QEventLoop.
// Demuestra: encapsulamiento, funcion virtual, clase base no abstracta.
class ClienteHTTP {
public:
    explicit ClienteHTTP(const QString& urlBase);
    virtual ~ClienteHTTP();

    // Sin copia: QNetworkAccessManager no es copiable
    ClienteHTTP(const ClienteHTTP&)            = delete;
    ClienteHTTP& operator=(const ClienteHTTP&) = delete;

    QByteArray get (const QString& endpoint) const;
    QByteArray post(const QString& endpoint, QHttpMultiPart* multipart) const;

protected:
    // Virtual: subclases pueden personalizar headers antes de cada envio
    virtual QNetworkRequest construirRequest(const QUrl& url) const;

    QString _urlBase;

    // mutable: get/post son logicamente "const" para el cliente, pero
    // QNetworkAccessManager requiere llamadas no-const internamente.
    mutable QNetworkAccessManager _manager;
};

} // namespace DIArize::Red
