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



#ifndef VIDEO_H
#define VIDEO_H

#include <QtGui>
#include <QtNetwork>
#include "converter.h"
#include "converter_ffmpeg.h"
#include "http_handler.h"

struct videoQuality
{
    QString quality;
    QString videoUrl;
    QString audioUrl;
    QStringList videoSegments;
    QStringList audioSegments;
    QString containerName;
    bool chunkedDownload;
    int resolution;

    videoQuality()
    {
        quality = "standard";
        chunkedDownload = false;
        resolution = 0;
    }

    videoQuality(QString Quality, QString VideoUrl)
    {
        quality = Quality;
        videoUrl = VideoUrl;
        chunkedDownload = false;
        resolution = 0;
    }

    videoQuality(QString Quality, QString VideoUrl, bool ChunkedDownload)
    {
        quality = Quality;
        videoUrl = VideoUrl;
        chunkedDownload = ChunkedDownload;
        resolution = 0;
    }

    bool operator<(videoQuality other) const
    {
        int thisQuality;
        int otherQuality;

        QRegExp expression("[0-9]+");

        if (expression.indexIn(this->quality) != -1)  {
            thisQuality = expression.cap().toInt();

            if (expression.indexIn(other.quality) != -1) {
                otherQuality = expression.cap().toInt();

                return thisQuality < otherQuality;
            }
        }

        return false;
    }
};

class video : public QObject
{
    Q_OBJECT
public:
    video();

    virtual video* createNewInstance();

    //*
    //*Portal Information
    //*
    QIcon* getIcon();
    QString getName();
    bool supportsSearch();
    bool compatibleWithUrl(QString);

    //*
    //*Portal Access
    //*
    QString getSearch(QString);

    //*
    //*Video Access
    //*
    bool isFinished();
    QString originalUrl();
    virtual bool setUrl(QString);
    virtual void analyse();
    virtual void download();
    virtual void restart();
    void setQuality(int);
    QString quality();
    QString title();
    void setTreeItem(QTreeWidgetItem* item);
    virtual QList< QPair<QString, int> > getSupportedQualities();
    void setFormat(int format);
    QProgressBar* _progressBar; //fixme!
    void setTargetPath(QString target);
    QString getSaveTitle();
    QString getSaveFileName();
    QString getTargetPath();

    void setMetaTitle(QString);
    void setMetaArtist(QString);
    QString metaTitle();
    QString metaArtist();

    void setConverter(converter* converter, int mode);

    QTreeWidgetItem* treeItem();

    void togglePause();
    bool isDownloadPaused();

    QPair<qint64, qint64> downloadProgress();
    void cancel();


protected:
    //*
    //*Portal Information
    //*
    QString _name;
    QList<QRegExp> _urlRegExp;
    QIcon* _icon;
    bool _supportsTitle;
    bool _supportsDescription;
    bool _supportsThumbnail;
    bool _supportsSearch;
    QList<videoQuality> _supportedQualities;
    converter* _converter;
    int _converterMode;

    //*
    //*Video Information
    //*
    bool _finished;

    QString _originalUrl;
    QUrl _url;
    QUrl _urlThumbnail;
    QString _targetPath;
    bool _chunkedDownload;

    QString _title;

    int _format;
    int _quality;
    QByteArray _videoData;

    QString _metaTitle;
    QString _metaArtist;




    //*
    //*Processing
    //*
    http_handler* handler;
    QFile* downloadFile;
	QPair<qint64, qint64> cachedProgress;
    bool _downloadPaused;
    bool _isRestarted;

    int _step;
    virtual void parseVideo(QString);

    QTreeWidgetItem* _treeItem;
    void setToolTip(QString);

    protected slots:
        virtual void handleDownloads();
        void changeProgress(qint64, qint64);
        virtual void startConvert();
        void conversionFinished();
        virtual void slotAnalysingFinished();
        virtual void networkError(QString error);

    signals:
        void error(QString);
        void error(QString, video*);
        void progressChanged(int, int);
        void stateChanged(QString);
        void downloadFinished();
        void analysingFinished();
        void conversionFinished(video*);
};

#endif // ABSTRACT_video_H
