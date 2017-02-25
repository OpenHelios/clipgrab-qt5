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



#ifndef CLIPGRAB_H
#define CLIPGRAB_H

#include <QtGui>
#include <QtNetwork>
#include <QtXml>
#include <QtDebug>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QtWidgets/QSystemTrayIcon>
#endif

#include "video.h"
#include "video_youtube.h"
#include "video_vimeo.h"
#include "video_dailymotion.h"
#include "video_myspass.h"
#include "video_facebook.h"
#include "video_heuristic.h"

#include "converter.h"
#include "converter_copy.h"
#include "converter_ffmpeg.h"

#include "ui_update_message.h"
#include "message_dialog.h"

struct format
{
    converter* _converter;
    QString _name;
    int _mode;
};

struct language
{
    language(): isRTL(false) {}

    QString name;
    QString code;
    bool isRTL;
};

struct updateInfo
{
    QString version;
    QString url;
    QString sha1;
    QDomNodeList domNodes;

    friend bool operator>(const updateInfo &a, const updateInfo &b)
    {
        return a.version.compare(b.version) < 0;
    }

    friend bool operator<(const updateInfo &a, const updateInfo &b)
    {
        return b.version.compare(a.version) > 0;
    }

    friend bool operator==(const updateInfo &a, const updateInfo &b)
    {
        return a.version == b.version;
    }
};


class ClipGrab : public QObject
{
    Q_OBJECT

    public:
    ClipGrab();

    QList<video*> portals;
    QList<format> formats;
    QList<video*> downloads;
    QList<converter*> converters;
    QSettings settings;
    QSystemTrayIcon trayIcon;
    QString getDownloadSaveFileName(int item);
    QString getDownloadTargetPath(int item);
    QString getDownloadOriginalUrl(int item);
    QClipboard* clipboard;
    QString clipboardUrl;
    bool isDownloadPaused(int);
    QPair<qint64, qint64> downloadProgress();
    QString version;
    bool isDownloadFinished(int item);
    video* heuristic;

    QList<language> languages;

    int downloadsRunning();

    void getUpdateInfo();

    protected:
        video* getVideoFromId(int id);
        QDialog* updateMessageDialog;
        Ui::UpdateMessage* updateMessageUi;
        QList<updateInfo> availableUpdates;
        QTemporaryFile* updateFile;
        QNetworkReply* updateReply;

    public slots:
        void determinePortal(QString url);
        void errorHandler(QString);
        void errorHandler(QString, video*);
        void addDownload(video* clip);
        void parseUpdateInfo(QNetworkReply* reply);
        void cancelDownload(int item);
        void removeDownload(int item);
        void restartDownload(int item);
        void clipboardChanged();
        void pauseDownload(int);
        void activateProxySettings();
        void cancelAll();
        void startUpdateDownload();
        void skipUpdate();
        void updateDownloadFinished();
        void updateDownloadProgress(qint64, qint64);
        void updateReadyRead();

    signals:
        void compatiblePortalFound(bool, video* portal);
        void compatibleUrlFoundInClipboard(QString url);
};

#endif // CLIPGRAB_H
