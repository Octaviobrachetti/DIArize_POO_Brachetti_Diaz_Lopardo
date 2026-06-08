#include "red/ClienteHTTP.h"
#include <QEventLoop>
#include <QHttpMultiPart>

namespace DIArize::Red {

ClienteHTTP::ClienteHTTP(const QString& urlBase) : _urlBase(urlBase) {}

ClienteHTTP::~ClienteHTTP() = default;

QNetworkRequest ClienteHTTP::construirRequest(const QUrl& url) const {
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    return req;
}

QByteArray ClienteHTTP::get(const QString& endpoint) const {
    QUrl url(_urlBase + endpoint);
    QEventLoop loop;
    QNetworkReply* reply = _manager.get(construirRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QByteArray data = (reply->error() == QNetworkReply::NoError)
                          ? reply->readAll()
                          : QByteArray{};
    reply->deleteLater();
    return data;
}

QByteArray ClienteHTTP::post(const QString& endpoint, QHttpMultiPart* multipart) const {
    QUrl url(_urlBase + endpoint);
    QEventLoop loop;
    QNetworkReply* reply = _manager.post(construirRequest(url), multipart);
    multipart->setParent(reply);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QByteArray data = (reply->error() == QNetworkReply::NoError)
                          ? reply->readAll()
                          : QByteArray{};
    reply->deleteLater();
    return data;
}

} // namespace DIArize::Red
