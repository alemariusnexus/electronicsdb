#pragma once

#include "../../global.h"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "../../util/Task.h"


namespace electronicsdb
{


class SimpleURLDownloader : public QObject
{
    Q_OBJECT

public:
    SimpleURLDownloader();

    void downloadToFile(const QUrl& url, const QString& fpath);
    bool downloadToFileBlocking(const QUrl& url, const QString& fpath);

    bool isLastSuccessful() const { return lastSuccess; }

private slots:
    void downloadFinished(QNetworkReply* reply);
    void onFinished(bool success);

signals:
    void finished(bool success);

private:
    QNetworkAccessManager netMgr;
    QString fpath;
    Task* task;
    QNetworkReply* reply;
    bool lastSuccess;
};


}
