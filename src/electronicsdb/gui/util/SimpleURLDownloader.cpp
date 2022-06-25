#include "SimpleURLDownloader.h"

#include <QTemporaryFile>
#include <QProgressDialog>
#include <QEventLoop>

#include "../../exception/IOException.h"
#include "../../System.h"


namespace electronicsdb
{


SimpleURLDownloader::SimpleURLDownloader()
        : task(nullptr), reply(nullptr), lastSuccess(false)
{
    connect(&netMgr, &QNetworkAccessManager::finished, this, &SimpleURLDownloader::downloadFinished);
    connect(this, &SimpleURLDownloader::finished, this, &SimpleURLDownloader::onFinished);
}

void SimpleURLDownloader::downloadToFile(const QUrl& url, const QString& fpath)
{
    this->fpath = fpath;

    QNetworkRequest req(url);
    reply = netMgr.get(req);
}

bool SimpleURLDownloader::downloadToFileBlocking(const QUrl& url, const QString& fpath)
{
    System* sys = System::getInstance();

    downloadToFile(url, fpath);

    task = new Task(QString("Downloading %1...").arg(url.fileName()), 0, 0);
    sys->startTask(task);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    return isLastSuccessful();
}

void SimpleURLDownloader::downloadFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        reply = nullptr;
        lastSuccess = false;
        emit finished(false);
        throw IOException("Error sending network request for download.", __FILE__, __LINE__);
    }

    QByteArray dlData = reply->readAll();
    reply = nullptr;

    QFile file(fpath);
    if (!file.open(QFile::WriteOnly)) {
        lastSuccess = false;
        emit finished(false);
        throw IOException("Error opening temporary file for download", __FILE__, __LINE__);
    }

    qint64 numWritten = file.write(dlData);
    if (numWritten < 0) {
        lastSuccess = false;
        emit finished(false);
        throw IOException("Error writing to temporary file for download", __FILE__, __LINE__);
    }
    if (numWritten != dlData.size()) {
        lastSuccess = false;
        emit finished(false);
        throw IOException("Unexpected number of bytes written to temporary file for download", __FILE__, __LINE__);
    }

    file.flush();
    file.close();

    lastSuccess = true;
    emit finished(true);
}

void SimpleURLDownloader::onFinished(bool success)
{
    System* sys = System::getInstance();

    if (task) {
        sys->endTask(task);
        delete task;
        task = nullptr;
    }
}


}
