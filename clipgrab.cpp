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



#include "clipgrab.h"

#include <QMessageBox>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

ClipGrab::ClipGrab()
{
    //*
    //* Set current version
    //*
    version = QCoreApplication::applicationVersion();

    //*
    //* Initialise languages
    //*
    language newLang;
    newLang.name = tr("Automatic language recognition");
    newLang.code = "auto";
    languages.append(newLang);

    newLang.name = "Bahasa Indonesia";
    newLang.code = "id";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Català");
    newLang.code = "ca";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Česky");
    newLang.code = "cs";
    languages.append(newLang);

    newLang.name = "Deutsch";
    newLang.code = "de";
    languages.append(newLang);

    newLang.name = "English";
    newLang.code = "en";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Español");
    newLang.code = "es";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Français");
    newLang.code = "fr";
    languages.append(newLang);

    newLang.name = "Italiano";
    newLang.code = "it";
    languages.append(newLang);

    newLang.name = "Kiswahili";
    newLang.code = "sw";
    languages.append(newLang);

    newLang.name = "Magyar";
    newLang.code = "hu";
    languages.append(newLang);

    newLang.name = "Nederlands";
    newLang.code = "nl";
    languages.append(newLang);

    newLang.name = "Norsk (Bokmål)";
    newLang.code = "no";
    languages.append(newLang);

    newLang.name = "Polski";
    newLang.code = "pl";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Português");
    newLang.code = "pt";
    languages.append(newLang);

    newLang.name = "Romana";
    newLang.code = "ro";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Slovenščina");
    newLang.code = "si";
    languages.append(newLang);

    newLang.name = "Suomi";
    newLang.code = "fi";
    languages.append(newLang);

    newLang.name = "Svenska";
    newLang.code = "sv";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Türkçe");
    newLang.code = "tr";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Tiếng Việt");
    newLang.code = "vi";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Ελληνικά¨");
    newLang.code = "el";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("فارسی");
    newLang.code = "fa";
    newLang.isRTL = true;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("日本語");
    newLang.code = "ja";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("한국어");
    newLang.code = "ko";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("ਪੰਜਾਬੀ");
    newLang.code = "pa";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Русский");
    newLang.code = "ru";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("Српски");
    newLang.code = "sr";
    languages.append(newLang);

    newLang.name = QString::fromLocal8Bit("中文");
    newLang.code = "zh";
    languages.append(newLang);

    //*
    //* Initialise the supported portals
    //*
    portals.append(new video_youtube);
    portals.append(new video_vimeo);
    portals.append(new video_dailymotion);
    portals.append(new video_myspass);
    portals.append(new video_facebook);
    
    //*
    //* Initialise the heuristic
    //*
    this->heuristic = new video_heuristic();

    //*
    //* Initialise the supported converters
    //*
    converters.append(new converter_copy);
    converters.append(new converter_ffmpeg);

    converter* tmpConverter;
    QList<QString> tmpModes;
    format tmpFormat;
    for (int i = 0; i < converters.size(); ++i)
    {
        tmpConverter = converters.at(i);
        connect(tmpConverter, SIGNAL(error(QString)), this, SLOT(errorHandler(QString)));
        if (tmpConverter->isAvailable())
        {
            tmpModes = tmpConverter->getModes();
            for (int i = 0; i < tmpModes.size(); ++i)
            {
                tmpFormat._converter = tmpConverter;
                tmpFormat._mode = i;
                tmpFormat._name = tmpModes.at(i);
                formats.append(tmpFormat);
            }
        }
    }

    //*
    //* Load Equifax CA certificate to avoid SSL problems on systems where it’s not in the trust store
    //*
    bool certExists = false;
    foreach (QSslCertificate cert, QSslSocket::defaultCaCertificates()) {
        if (cert.serialNumber() == "903804111")
        {
            certExists = true;
        }
    }
    if (!certExists)
    {
        QSslSocket::addDefaultCaCertificates(":/crt/equifax_secure_ca.crt");
        qDebug() << "Adding Equifax CA certificate";
    }

    //*
    //* Remove previously downloaded update
    //*
    QString updateFileName = settings.value("updateFile", "").toString();
    settings.remove("updateFile");
    if (!updateFileName.isEmpty())
    {
        QFile updateFile;
        updateFile.setFileName(updateFileName);
        if (updateFile.exists())
        {
            updateFile.remove();
        }
    }

    //*
    //* Disable HTTPS certificate verification on legacy Mac systems
    //*
    #ifdef MAC_LEGACY
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    configuration.setProtocol(QSsl::TlsV1);
    QSslConfiguration::setDefaultConfiguration(configuration);
    #endif

    //*
    //* Miscellaneous
    //*
    activateProxySettings();
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
}

void ClipGrab::getUpdateInfo()
{
    //*
    //* Make update info request to clipgrab.org
    //*
    QString sys = "x11";

    #if defined Q_WS_WIN
        sys = "win";
    #endif
    #if defined Q_WS_MAC
        #ifdef Q_WS_MAC64
            sys = "mac";
        #else
            sys = "mac-legacy";
        #endif
    #endif

    QString firstStarted = settings.value("firstStarted", "").toString();

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QUrl updateInfoRequestUrl("https://clipgrab.org/update/" + sys + "/");
#else
    QUrlQuery updateInfoRequestUrl("https://clipgrab.org/update/" + sys + "/");
#endif
    updateInfoRequestUrl.addQueryItem("v", this->version);
    updateInfoRequestUrl.addQueryItem("l", QLocale::system().name().split("_")[0]);
    if (!firstStarted.isEmpty())
    {
        updateInfoRequestUrl.addQueryItem("t", firstStarted);
    }
    QNetworkRequest updateInfoRequest;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    updateInfoRequest.setUrl(updateInfoRequestUrl);
#else
    QUrl url;
    url.setQuery(updateInfoRequestUrl);
    updateInfoRequest.setUrl(url);
#endif
    QNetworkAccessManager* updateInfoNAM = new QNetworkAccessManager;
    updateInfoNAM->get(updateInfoRequest);
    connect(updateInfoNAM, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseUpdateInfo(QNetworkReply*)));
}

void ClipGrab::parseUpdateInfo(QNetworkReply* reply)
{
    if (!reply->bytesAvailable())
    {
        qDebug() << "No updates available";
        return;
    }

    QDomDocument updateInfoXml;
    updateInfoXml.setContent(reply->readAll());

    //Process timestamp
    QDomNodeList timestamps = updateInfoXml.elementsByTagName("timestamp");
    if (!timestamps.isEmpty() && settings.value("firstStarted", "").toString().indexOf(".") == -1)
    {
        settings.setValue("firstStarted", timestamps.at(0).toElement().attribute("time"));
    }

    //Processing available updates
    QDomNodeList updates = updateInfoXml.elementsByTagName("update");
    if (!updates.isEmpty() && this->settings.value("DisableUpdateNotifications", false) == false)
    {
        QString currentVersion = QCoreApplication::applicationVersion();

        //Get update information
        for (uint i = 0; i < updates.length(); i++)
        {
            QDomElement update = updates.at(i).toElement();

            //Only consider newer versions
            if (currentVersion.compare(update.attribute("version", "0")) >= 0)
            {
                continue;
            }

            updateInfo newUpdateInfo;
            newUpdateInfo.version = update.attribute("version");
            newUpdateInfo.url = update.attribute("uri");
            newUpdateInfo.sha1 = update.attribute("sha1");
            newUpdateInfo.domNodes = update.childNodes();
            this->availableUpdates.append(newUpdateInfo);
        }

        if (!this->availableUpdates.isEmpty())
        {
            qSort(this->availableUpdates.begin(), this->availableUpdates.end());

            //Create changelog document
            QDomDocument updateNotesDocument("html");
            updateNotesDocument.setContent(QString("<style>body{font-family:'Segoe UI', Ubuntu, sans-serif};h3{font-size: 100%;}</style>"));

            for (int i = (this->availableUpdates.length() - 1); i >= 0; i--)
            {
                QDomElement updateNotes = updateNotesDocument.createElement("div");
                updateNotes.appendChild(updateNotesDocument.createElement("h3"));
                updateNotes.childNodes().at(0).appendChild(updateNotesDocument.createTextNode(this->availableUpdates.at(i).version));

                for (uint j = 0; j < this->availableUpdates.at(i).domNodes.length(); j++)
                {
                    updateNotes.appendChild(this->availableUpdates.at(i).domNodes.at(j).cloneNode());
                }

                updateNotesDocument.appendChild(updateNotes);
            }

            if (settings.value("skip-" + this->availableUpdates.last().version , false).toBool() == false)
            {
                this->updateMessageUi = new Ui::UpdateMessage();
                this->updateMessageDialog = new QDialog(QApplication::activeWindow());
                this->updateMessageUi->setupUi(this->updateMessageDialog);

                this->updateMessageUi->labelInfoText->setText(this->updateMessageUi->labelInfoText->text().arg(this->availableUpdates.last().version, currentVersion));
                this->updateMessageUi->progressBar->hide();
                this->updateMessageUi->labelDownloadProgress->hide();
                this->updateMessageUi->webView->setContent(updateNotesDocument.toString().toUtf8());
                this->updateMessageUi->webView->setAcceptDrops(false);

                this->updateReply = NULL;
                this->updateFile = NULL;


                connect(this->updateMessageUi->buttonConfirm, SIGNAL(clicked()), this, SLOT(startUpdateDownload()));
                connect(this->updateMessageUi->buttonSkip, SIGNAL(clicked()), this, SLOT(skipUpdate()));

                if (this->updateMessageDialog->exec() == QDialog::Rejected)
                {
                    //Cancel any ongoing update operations
                    if (this->updateReply != NULL)
                    {
                        this->updateReply->disconnect();
                        this->updateReply->abort();
                        this->updateReply->deleteLater();
                    }
                    if (this->updateFile != NULL)
                    {
                        this->updateFile->close();
                        this->updateFile->deleteLater();
                    }
                }
            }
        }
    }

    //Processing commands
    QDomNodeList commands = updateInfoXml.elementsByTagName("command");
    if (!commands.isEmpty())
    {
        for (uint i = 0; i < commands.length(); i++)
        {
            QDomElement command = commands.at(i).toElement();

            if (command.attribute("repeat", "false") != "true")
            {
                QString id = "cmd-";
                if (command.hasAttribute("id"))
                {
                    id.append(command.attribute("id"));
                }
                else
                {
                    QDomDocument doc("cmd");
                    doc.appendChild(command.cloneNode());
                    QCryptographicHash hash(QCryptographicHash::Sha1);
                    hash.addData(doc.toString().toUtf8());
                    id.append(hash.result().toHex());
                }

                if (settings.value(id, false) == true)
                {
                    continue;
                }
                settings.setValue(id, true);
            }

            if (command.attribute("type") == "message")
            {
                messageDialog* dialog = new messageDialog(QApplication::activeWindow());
                dialog->setUrl(QUrl(command.attribute("uri")));

                if (command.hasAttribute("title")) {
                    dialog->setWindowTitle(command.attribute("title"));
                }
                if (command.hasAttribute("link-policy")) {
                    if (command.attribute("link-policy") == "open")
                    {
                        dialog->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
                    }
                    else if (command.attribute("link-policy") == "open-external")
                    {
                        dialog->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
                    }
                }
                dialog->exec();
            }
            else if ((command.attribute("type") == "open"))
            {
                QDesktopServices::openUrl(QUrl(command.attribute("uri")));
            }
            else if (command.attribute("type") == "die")
            {
                QApplication::quit();
            }
        }
    }
}

void ClipGrab::startUpdateDownload()
{
    QString updateUrl = this->availableUpdates.last().url;
    QString updateFilePattern = updateUrl.split("/").last();
    updateFilePattern.insert(updateFilePattern.lastIndexOf(QRegExp("\\.dmg|\\.exe|\\.tar")), "-XXXXXX");
    this->updateFile = new QTemporaryFile(QDir::tempPath() + "/" + updateFilePattern);
    this->updateFile->open();
    qDebug() << "Downloading update to " << this->updateFile->fileName();

    this->updateMessageUi->buttonConfirm->setDisabled(true);
    this->updateMessageUi->buttonLater->setDisabled(true);
    this->updateMessageUi->buttonSkip->setDisabled(true);
    this->updateMessageUi->progressBar->setMaximum(1);
    this->updateMessageUi->progressBar->setValue(0);
    this->updateMessageUi->progressBar->show();
    this->updateMessageUi->labelDownloadProgress->show();

    QNetworkAccessManager* updateNAM = new QNetworkAccessManager();
    this->updateReply = updateNAM->get(QNetworkRequest(QUrl(updateUrl)));
    connect(updateReply, SIGNAL(readyRead()), this, SLOT(updateReadyRead()));
    connect(this->updateReply, SIGNAL(finished()), this, SLOT(updateDownloadFinished()));
    connect(this->updateReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64, qint64)));
}

void ClipGrab::updateReadyRead()
{
    this->updateFile->write(this->updateReply->readAll());
}

void ClipGrab::updateDownloadProgress(qint64 progress, qint64 total)
{
    int updateProgress = progress/1024/1024;
    int updateSize = total/1024/1024;

    this->updateMessageUi->labelDownloadProgress->setText(tr("Downloading update … %1/%2 MBytes").arg(QString::number(updateProgress), QString::number(updateSize)));
    this->updateMessageUi->progressBar->setMaximum(updateSize);
    this->updateMessageUi->progressBar->setValue(updateProgress);
}

void ClipGrab::updateDownloadFinished()
{
    if (this->updateReply->error())
    {
        this->errorHandler(tr("There was an error while downloading the update.: %1").arg(this->updateReply->errorString()));
        this->updateMessageDialog->reject();
        return;
    }

    this->updateFile->flush();

    QCryptographicHash fileHash(QCryptographicHash::Sha1);
    updateFile->seek(0);
    fileHash.addData(this->updateFile->readAll());
    QString hashResult = fileHash.result().toHex();
    QString updateSha1 = this->availableUpdates.last().sha1;
    if (hashResult != updateSha1)
    {
        this->errorHandler(tr("The fingerprint of the downloaded update file could not be verified: %1 should have been %2").arg(hashResult, updateSha1));
        this->updateMessageDialog->reject();
        return;
    }

    //Close and rename to avoid problems with file locks
    updateFile->setAutoRemove(false);
    updateFile->close();
    updateFile->rename(updateFile->fileName().insert(updateFile->fileName().lastIndexOf(QRegExp("\\.dmg|\\.exe|\\.tar")), "-update"));

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(updateFile->fileName())))
    {
        errorHandler(tr("Could not open update file."));
        this->updateFile->remove();
        this->updateMessageDialog->reject();
        return;
    }

    settings.setValue("updateFile", this->updateFile->fileName());
    QApplication::quit();
}

void ClipGrab::skipUpdate()
{
    settings.setValue("skip-" + this->availableUpdates.last().version, true);
}

void ClipGrab::determinePortal(QString url)
{
    if (url.startsWith("http://") || url.startsWith("https://"))
    {
        video* portal = 0;
        bool found = false;
        for (int i = 0; i < portals.size(); ++i)
        {
            if (portals.at(i)->compatibleWithUrl(url))
            {
                found = true;
                portal = portals.at(i);
            }
        }
        emit compatiblePortalFound(found, portal);
    }
    else
    {
        emit compatiblePortalFound(false, 0);
    }
}

void ClipGrab::errorHandler(QString error)
{
    QMessageBox box;
    box.setText(error);
    box.exec();
}

void ClipGrab::errorHandler(QString error, video* /*video*/)
{
    bool solved = false;
    if (!solved)
    {
        QMessageBox box;
        box.setText(error);
        box.exec();
    }

}

void ClipGrab::addDownload(video* clip)
{
    downloads.append(clip);
    clip->download();
}

void ClipGrab::clipboardChanged()
{
    if (settings.value("Clipboard", "ask").toString() != "never")
    {
        QString url = QApplication::clipboard()->text();
        if (!url.isEmpty())
        {
            bool found = false;
            for (int i = 0; i < portals.size(); ++i)
            {
                if (portals.at(i)->compatibleWithUrl(url))
                {
                    found = true;
                    break;
                }
            }
            if (found == true)
            {
                clipboardUrl = url;
                emit compatibleUrlFoundInClipboard(url);
            }
        }
    }
}

int ClipGrab::downloadsRunning()
{
    int result = 0;
    for (int i = 0; i < downloads.size(); ++i)
    {
        if (downloads.at(i)->isFinished() == false)
        {
            result++;
        }
    }
    return result;
}

QPair<qint64, qint64> ClipGrab::downloadProgress()
{
    QPair<qint64, qint64> returnValue;
    returnValue.first = 0;
    returnValue.second = 0;
    for (int i = 0; i < downloads.size(); ++i)
    {
        if (downloads.at(i)->isFinished() == false)
        {
            QPair<qint64, qint64> currentVideoProgress = downloads.at(i)->downloadProgress();
            returnValue.first = returnValue.first + currentVideoProgress.first;
            returnValue.second = returnValue.second + currentVideoProgress.second;
        }
    }
    return returnValue;
}

void ClipGrab::activateProxySettings()
{
    if (settings.value("UseProxy", false).toBool() == true)
    {
        QNetworkProxy proxy;
        if (settings.value("ProxyType", false).toInt() == 0)
        {
            proxy.setType(QNetworkProxy::HttpProxy);
        }
        else
        {
            proxy.setType(QNetworkProxy::Socks5Proxy);
        }
        proxy.setHostName(settings.value("ProxyHost", "").toString());
        proxy.setPort(settings.value("ProxyPort", "").toInt());
        if (settings.value("ProxyAuthenticationRequired", false).toBool() == true)
        {
            proxy.setUser(settings.value("ProxyUsername", "").toString());
            proxy.setPassword(settings.value("ProxyPassword").toString());
        }
        QNetworkProxy::setApplicationProxy(proxy);
    }
    else
    {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }

}

video* ClipGrab::getVideoFromId(int id)
{
    if (!downloads.count()-1 < id)
    {
        return downloads.at(id);
    }
    else
    {
        return NULL;
    }
}

void ClipGrab::cancelDownload(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        v->cancel();
    }
}

void ClipGrab::removeDownload(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        v->cancel();
        v->deleteLater();
        downloads.removeAt(item);
    }

}

void ClipGrab::restartDownload(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        v->restart();
    }
}

bool ClipGrab::isDownloadFinished(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        return v->isFinished();
    }
    return false;
}

QString ClipGrab::getDownloadOriginalUrl(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        return v->originalUrl();
    }
    return "";
}

QString ClipGrab::getDownloadSaveFileName(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        return v->getSaveFileName();
    }
    return "";
}

QString ClipGrab::getDownloadTargetPath(int item)
{
    video* v = getVideoFromId(item);
    if (v)
    {
        return v->getTargetPath();
    }
    return "";
}

void ClipGrab::pauseDownload(int item)
{
    if (!downloads.count()-1 < item)
    {
        downloads.at(item)->togglePause();
    }
}

bool ClipGrab::isDownloadPaused(int item)
{
    if (!downloads.count()-1 < item)
    {
        return downloads.at(item)->isDownloadPaused();
    }
    return false;
}

void ClipGrab::cancelAll()
{
    for (int i = 0; i < downloads.size(); i++)
    {
        downloads.at(i)->cancel();
    }
}
