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



#include "video.h"

video::video()
{
    handler = new http_handler;
    connect(handler, SIGNAL(allDownloadsFinished()), this, SLOT(handleDownloads()));
    connect(handler, SIGNAL(error(QString)), this, SLOT(networkError(QString)));
    connect(this, SIGNAL(downloadFinished()), this, SLOT(startConvert()));
    _treeItem = NULL;
    _downloadPaused = false;
    _isRestarted = false;
}

bool video::supportsSearch()
{
    if (_supportsSearch == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

QString video::getName()
{
    return _name;
}

QIcon* video::getIcon()
{
    if (_icon)

    {
        return _icon;
    }
    else
    {
        return new QIcon(QPixmap(":/img/icon.png"));
    }
}

bool video::compatibleWithUrl(QString url)
{
    for (int i = 0; i < this->_urlRegExp.size(); ++i)
    {
        if (this->_urlRegExp.at(i).indexIn(url)!=-1)
        {
            return true;
        }
    }

    return false;

}

bool video::setUrl(QString url)
{
    _originalUrl = url;

    if (_url.isEmpty())
    {
        this->_url = QUrl(url);
        if (_url.isValid())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void video::analyse()
{
    _finished = false;

    if (this->_url.isValid())
    {
        _step = 1;
        handler->addDownload(_url.toString());
		connect(this, SIGNAL(analysingFinished()), this, SLOT(slotAnalysingFinished()));
    }
    else
    {
        emit error("Url is not valid");
    }
}



void video::handleDownloads()
{
    switch (this->_step)
    {
        case 1:
        {
            handler->downloads.at(0)->tempFile->close();
            handler->downloads.at(0)->tempFile->open();
            QByteArray data = handler->downloads.at(0)->tempFile->readAll();
            handler->downloads.at(0)->tempFile->close();
            QString html = QString::fromUtf8(data, data.size());
            handler->clearDownloads();
            parseVideo(html);
            break;
        }

        case 3:
        {
            if (handler->downloads.size() == 1)
            {
                this->downloadFile = handler->downloads.at(0)->tempFile;
                this->downloadFile->flush();
                emit downloadFinished();
            }
            else
            {
                this->_step = 4;

                QList<QFile*> files;
                for (int i=0; i < handler->downloads.size(); i++)
                {
                    handler->downloads.at(i)->tempFile->flush();
                    files << handler->downloads.at(i)->tempFile;
                }
                this->_progressBar->setMinimum(0);
                this->_progressBar->setMaximum(0);
                setToolTip("<strong>" + tr("Converting ...") + "</strong>");
                this->downloadFile = new QTemporaryFile(QDir::tempPath() + "/clipgrab-concat--XXXXXX");
                this->downloadFile->open(QIODevice::ReadOnly);
                this->downloadFile->close();

                converter_ffmpeg* ffmpeg = new converter_ffmpeg;
                connect(ffmpeg, SIGNAL(conversionFinished()), this, SLOT(handleDownloads()));
                ffmpeg->concatenate(files, this->downloadFile, this->_supportedQualities.at(_quality).containerName);
            }
            break;
        }

        case 4:
        {
            emit downloadFinished();
			break;
        }
    }
}

void video::parseVideo(QString)
{
    emit error("Video format not implemented");
}

void video::changeProgress(qint64 bytesReceived, qint64 bytesTotal)
{

    //Only update if more than 128 new KiB have been received
    if (bytesReceived < cachedProgress.first + 131072)
    {
        return;
    }

    if (this->_treeItem)
    {
        if (bytesTotal > 0)
        {
            this->_progressBar->setFormat("%p%");
            this->_progressBar->setMaximum(bytesTotal);
            this->_progressBar->setValue(bytesReceived);
            setToolTip("<strong>" + tr("Downloading ...") + "</strong><br />" + QString::number((double)(bytesReceived)/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB") + "/" + QString::number((double)bytesTotal/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB"));
        }
    }
    else
    {
        qDebug() << bytesReceived << "of" << bytesTotal;
    }
	
	cachedProgress.first = bytesReceived;
	cachedProgress.second = bytesTotal;
	
    emit progressChanged(bytesReceived, bytesTotal);
}

void video::setQuality(int quality)
{
    this->_quality = quality;
}

QString video::quality()
{
    return this->_supportedQualities.at(_quality).quality;
}

video* video::createNewInstance()
{
    return new video();
}

//Get pair of quality label QString and resolution int
QList< QPair<QString, int> > video::getSupportedQualities()
{
    QList< QPair<QString, int> > result;
    for (int i = 0; i < _supportedQualities.size(); ++i)
    {
        result << qMakePair(_supportedQualities.at(i).quality, _supportedQualities.at(i).resolution);
    }
    return result;
}

QString video::title()
{
    return _title;
}

void video::download()
{
    _step = 3;
    handler->clearDownloads();

    connect(handler, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(changeProgress(qint64, qint64)));

    if (!this->_supportedQualities.at(_quality).videoUrl.isEmpty())
    {
        if (this->_supportedQualities.at(_quality).videoSegments.isEmpty())
        {
            qDebug() << "Downloading video file: " << this->_supportedQualities.at(_quality).videoUrl;
            handler->addDownload(this->_supportedQualities.at(_quality).videoUrl, this->_supportedQualities.at(_quality).chunkedDownload);
        }
        else
        {
            qDebug() << "Downloading segmented video file: " << this->_supportedQualities.at(_quality).videoUrl;
            handler->addDownload(this->_supportedQualities.at(_quality).videoUrl, this->_supportedQualities.at(_quality).videoSegments);
        }
    }
    if (!this->_supportedQualities.at(_quality).audioUrl.isEmpty())
    {
        if (this->_supportedQualities.at(_quality).audioSegments.isEmpty())
        {
            qDebug() << "Downloading audio file: " << this->_supportedQualities.at(_quality).audioUrl;
            handler->addDownload(this->_supportedQualities.at(_quality).audioUrl, this->_supportedQualities.at(_quality).chunkedDownload);
        }
        else
        {
            qDebug() << "Downloading segmented audio file: " << this->_supportedQualities.at(_quality).audioUrl;
            handler->addDownload(this->_supportedQualities.at(_quality).audioUrl, this->_supportedQualities.at(_quality).audioSegments);
        }
    }
}

void video::setTreeItem(QTreeWidgetItem* item)
{
    this->_treeItem = item;
}

void video::setToolTip(QString tooltip)
{
    if (this->_treeItem)
    {
        for (int i = 0; i <= 3; i++)
        {
            this->_treeItem->setToolTip(i, tooltip);
        }
    }
}

void video::setFormat(int format)
{
    this->_format = format;
}

void video::startConvert()
{
    this->_progressBar->setMinimum(0);
    this->_progressBar->setMaximum(0);
    setToolTip("<strong>" + tr("Converting ...") + "</strong>");

    connect(this->_converter, SIGNAL(conversionFinished()), this, SLOT(conversionFinished()));
    this->_converter->startConversion(this->downloadFile, _targetPath, _supportedQualities.at(_quality).containerName, _metaTitle, _metaArtist,_converterMode);

}

void video::setConverter(converter* converter, int mode)
{
    _converter = converter;
    _converterMode = mode;


    //If audio track is available separately, only download audio
    if (mode == 3 || mode == 4 || mode == 5)
    {
        if (!_supportedQualities.at(_quality).audioUrl.isEmpty())
        {
            _supportedQualities[_quality].videoUrl.clear();
        }
    }
}

void video::setTargetPath(QString target)
{
    _targetPath = target;
}

QString video::getSaveTitle()
{
    QString title = this->_title;
    if (title.size() > 0)
    {
        QStringList charsToBeRemoved;
        charsToBeRemoved << "\\" << "/" << ":" << "*" << "?" << "<" << ">" << "|" << "'" << "\"" << "&amp;quot;";
        title = title.simplified();

        if (title[0] == QChar('.'))
        {
            title = title.remove(0, 1);
        }
        if (title.size() > 0)
        {
            for (int i = 0; i < charsToBeRemoved.size(); ++i)
            {
                title = title.replace(charsToBeRemoved.at(i), "");
            }
        }
        else
        {
            return "download-empty-title";
        }
    }
    else
    {
        return "download-empty-title";
    }
    qDebug() << title;
    return title;
}

void video::conversionFinished()
{
    this->_progressBar->setMaximum(1);
    this->_progressBar->setValue(1);
    this->_progressBar->setFormat(tr("Finished"));
    this->_finished = true;
    if (!this->_converter->target.isEmpty())
    {
        this->_targetPath = this->_converter->target;
    }
    setToolTip("<strong>" + tr("Finished!") + "</strong>");

    handler->clearDownloads();
	if (this->downloadFile)
	{
		this->downloadFile->deleteLater();

	}
    emit conversionFinished(this);

}

QString video::getSaveFileName()
{
    return _targetPath;
}

QString video::getTargetPath()
{
    return _targetPath;
}

QTreeWidgetItem* video::treeItem()
{
    return this->_treeItem;
}


bool video::isDownloadPaused()
{
    return this->_downloadPaused;
}

void video::togglePause()
{
    if (this->_downloadPaused)
    {
        this->handler->continueAllDownloads();
    }
    else
    {
        this->handler->pauseAllDownloads();
    }

    this->_downloadPaused = !this->_downloadPaused;
}

QPair<qint64, qint64> video::downloadProgress()
{
	return cachedProgress;
}

void video::setMetaTitle(QString title)
{
    if (title.size() > 0)
    {
        QStringList charsToBeRemoved;
        charsToBeRemoved << "\\" << "/" << ":" << "*" << "?" << "<" << ">" << "|" << "'" << "\"" << "&amp;quot;";
        title = title.simplified();

        if (title[0] == QChar('.'))
        {
            title = title.remove(0, 1);
        }
        if (title.size() > 0)
        {
            for (int i = 0; i < charsToBeRemoved.size(); ++i)
            {
                title = title.replace(charsToBeRemoved.at(i), "");
            }
        }
        _metaTitle = title;
    }
}

void video::setMetaArtist(QString artist)
{
    if (artist.size() > 0)
    {
        QStringList charsToBeRemoved;
        charsToBeRemoved << "\\" << "!" << "\"";
        artist = artist.simplified();

        if (artist[0] == QChar('.'))
        {
            artist = artist.remove(0, 1);
        }
        if (artist.size() > 0)
        {
            for (int i = 0; i < charsToBeRemoved.size(); ++i)
            {
                artist = artist.replace(charsToBeRemoved.at(i), "");
            }
        }
        _metaArtist = artist;
    }
}

QString video::metaTitle()
{
    return _metaTitle;
}

QString video::metaArtist()
{
    return _metaArtist;
}

void video::slotAnalysingFinished()
{

    qDebug() << "Discovered: " << _title;
    for (int i = 0; i > _supportedQualities.size(); i++)
    {
        qDebug() << _supportedQualities.at(i).quality << _supportedQualities.at(i).containerName << _supportedQualities.at(i).videoUrl;
    }
	handler->clearDownloads();

    //If the video download has been restarted, commence download right away
    if (this->_isRestarted)
    {
        this->download();
    }
}


void video::networkError(QString message)
{
    emit error(message, this);
}

void video::cancel()
{
    handler->cancelAllDownloads();

    if (this->_progressBar)
    {
        this->_progressBar->setMaximum(1);
        this->_progressBar->setValue(0);
        this->_progressBar->setFormat(tr("Cancelled"));
    }
    setToolTip("");
}

void video::restart()
{
    this->_isRestarted = true;
    this->_finished = false;
    this->_downloadPaused = false;
    this->_step = 0;
    this->cachedProgress.first = 0;
    this->cachedProgress.second = 0;

    handler->disconnect(this);
    this->handler->cancelAllDownloads();

    connect(handler, SIGNAL(allDownloadsFinished()), this, SLOT(handleDownloads()));
    connect(handler, SIGNAL(error(QString)), this, SLOT(networkError(QString)));

    this->disconnect(SIGNAL(analysingFinished()));
    this->analyse();
}

bool video::isFinished()
{
    return _finished;
}

QString video::originalUrl()
{
    return _originalUrl;
}
