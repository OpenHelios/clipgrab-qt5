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



#ifndef video_YOUTUBE_H
#define video_YOUTUBE_H

#include <qglobal.h>
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#include <QtWebKit>
#else
#include <QtWebKit/QtWebKit>
#endif
#include <QtGui>
#include <QFile>
#include <QDomDocument>
#include "ui_login_dialog.h"
#include "video.h"

struct fmtQuality
{
    QString quality;
    int resolution;
    QString video;
    QString audio;
    QStringList videoSegments;
    QStringList audioSegments;

    fmtQuality(QString quality, int resolution, QString video, QString audio="")
    {
        this->quality = quality;
        this->resolution = resolution;
        this->video = video;
        if (!audio.isEmpty())
        {
            this->audio = audio;
        }
    }
};

struct jsMethod
{
    QString name;
    QString code;
};


class video_youtube : public video
{
    Q_OBJECT

    private:
        QString _maxfmt;

        QString parseSignature(QString s);
        QString getFmtLink(QStringList, QString fmt);
        QString getUrlFromFmtLink(QString link);
        QString getQualityFromFmtLink(QString link);
        void parseJS(QString js);
        void extractJSMethod(QString name, QString js);
        void parseDashMpd(QString);
        QList<jsMethod> jsMethods;
        QString signatureMethodName;
        QString loginFormError;
        QString html;
        QString js;
        QString dashmpd;
        QList<QStringList> dashQualityLinks;
        QMap<QString, QString> requiredDownloads;
        QString downloading;
        QDialog* passwordDialog;
        Ui::LoginDialog* dui;
        QList<fmtQuality> fmtQualities;

    public:
        virtual bool setUrl(QString);
        video_youtube();
        video* createNewInstance();
        virtual void parseVideo(QString);

    public slots:
        void handleLoginUrlChanged(const QUrl);
        void handleLoginLoadFinished();
};

#endif // video_YOUTUBE_H
