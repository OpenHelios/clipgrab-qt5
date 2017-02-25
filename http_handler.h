/*
    ClipGrab³
    Copyright (C) Philipp Schmieder
    http://clipgrab.de
    feedback [at] clipgrab [dot] de

    This file is part of ClipGrab.
    ClipGrab is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ClipGrab is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ClipGrab.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <QtNetwork>
#include <QTemporaryFile>
#include <QDebug>

struct download
{

    download()
    {
        size = 0;
        currentProgress = 0;
        progress = 0;
		chunked = false;
		finished = false;
		redirectLevel = 0;
		tempFile = NULL;
        reply = NULL;
        _previousStatus = -2;
    }

    QNetworkReply* reply;

    QTemporaryFile* tempFile;

    qint64 size;
    qint64 progress;

    QStringList segments;
    int segmentPosition;

    qint64 currentProgress;
    bool chunked;

    int redirectLevel;
    bool finished;

    qint64 getProgress()
    {
        return currentProgress + progress;
    }

    int currentStatus()
    {
        if (reply)
        {
            return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        return -2;
    }

    int _previousStatus;
    int previousStatus()
    {
        return _previousStatus;
    }
};

class http_handler : public QObject
{
    Q_OBJECT

    public:
    http_handler();


    QNetworkReply* addDownload(QString, bool chunked=false, QByteArray postData=NULL, QStringList segments=QStringList());
    QNetworkReply* addDownload(QString initUrl, QStringList segments);
    void continueDownload(download*);
    void cancelAllDownloads();
    void pauseAllDownloads();
    void continueAllDownloads();
    void clearDownloads();
    QList<download*> downloads;

    QNetworkAccessManager* networkAccessManager;

    QNetworkRequest createRequest(QUrl);
    download* getDownload(QNetworkReply*);

    QString serializeCookies(QList<QNetworkCookie>);
    QList<QNetworkCookie> deserializeCookies(QString);

    protected slots:
        void dataHandler();
        void handleFinishedDownload(download* );
        void handleNetworkReply(QNetworkReply*);
        void handleSSLError(QNetworkReply* reply, const QList<QSslError> & errors);


    signals:
        void error(QString);
        void downloadProgress(qint64, qint64);

        void downloadFinished(download*);
        void allDownloadsFinished();
        void progressChanged(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // HTTP_HANDLER_H
