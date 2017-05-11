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



#include "video_vimeo.h"

video_vimeo::video_vimeo()
{
    this->_name = "Vimeo";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http[s]?://(www\\.)?vimeo\\.com/(channels/[a-z0-9]+/)?([0-9]+)", Qt::CaseInsensitive);
    _treeItem = NULL;
}

video* video_vimeo::createNewInstance()
{
    return new video_vimeo();
}


bool video_vimeo::setUrl(QString url)
{
    _originalUrl = url;

    _urlRegExp.first().indexIn(url);
    if (!_urlRegExp.first().cap(3).isEmpty())
    {
        this->_url = QUrl(url);
        if (_url.isValid())
        {
            return true;
        }
    }
    return false;
}


void video_vimeo::parseVideo(QString data)
{
    if (this->downloading.isEmpty() || this->downloading == "html")
    {
        QRegExp expression;
        QString expressionString = "\"(https://player.vimeo.com/video/\\d+/config[^\"]*)\"";
        expressionString.replace("/", "\\\\/");
        expression = QRegExp(expressionString);
        if (expression.indexIn(data) > -1) {
            QString jsonUrl = expression.cap(1).replace("&amp;", "&").replace("\\/", "/");
            qDebug() << "Using config URL" << jsonUrl;
            this->downloading = "json";
            handler->addDownload(jsonUrl);
        }
        else
        {
            expression = QRegExp("<form[^>]*id=\"pw_form\"");
            if (expression.indexIn(data) > -1)
            {
                dui = new Ui::LoginDialog();
                passwordDialog = new QDialog;
                dui->setupUi(passwordDialog);
                dui->loginDialogWebView->setUrl((this->_url));
                dui->rememberLogin->hide();
                connect(dui->loginDialogWebView, SIGNAL(urlChanged(QUrl)), this, SLOT(handleLogin(QUrl)));
                connect(dui->loginDialogWebView, SIGNAL(loadFinished(bool)), this, SLOT(verifyForm(bool)));

                if (passwordDialog->exec() == QDialog::Accepted)
                {
                    handler->addDownload(this->_url.toString());
                }
                else
                {
                    emit error("This video requires you to be signed in.", this);
                    emit analysingFinished();
                }

                passwordDialog->deleteLater();
                return;
            }
            emit error("Could not retrieve video info.", this);
            emit analysingFinished();
        }

    }
    else if (this->downloading == "json")
    {
        QRegExp expression;
        expression = QRegExp("\"title\":\"(.*)\",");
        expression.setMinimal(true);
        if (expression.indexIn(data) !=-1)
        {
            _title = QString(expression.cap(1)).replace("\\\"", "\"");

            QWebPage* page = new QWebPage();
            QList<QVariant> qualityList;
            qualityList = page->mainFrame()->evaluateJavaScript("var data = " + data + "; data.request.files.progressive").toList();

            for (int i = 0; i < qualityList.size(); i++) {
                QMap<QString, QVariant> videoInfo = qualityList.at(i).toMap();
                videoQuality newQuality;

                newQuality.quality = videoInfo.value("quality").toString();
                newQuality.videoUrl = videoInfo.value("url").toString();
                newQuality.containerName = ".mp4";
                expression = QRegExp("\\d+");
                if (expression.indexIn(newQuality.quality) > -1)
                {
                    newQuality.resolution = expression.cap(0).toInt();
                }
                _supportedQualities.append(newQuality);
            }

            //Sort quality list because Vimeo’s output isn’t ordered by default
            qSort(_supportedQualities);
            QList<videoQuality> invertedQualityList;
            for (int i = _supportedQualities.size() - 1; i >= 0; i--) {
                invertedQualityList << _supportedQualities.at(i);
            }
            _supportedQualities = invertedQualityList;

            if (_supportedQualities.isEmpty())
            {
                emit error("Could not retrieve video link.", this);

            }
        }
        else
        {
            emit error("Could not retrieve video title.", this);
        }

        emit analysingFinished();
    }
}

void video_vimeo::verifyForm(bool finished)
{
    if (!finished) return;

    QWebElement formElement = dui->loginDialogWebView->page()->mainFrame()->documentElement().findFirst("#pw_form");
    if (!formElement.isNull() && formElement.findFirst("[name=token]").isNull())
    {
        QString html = dui->loginDialogWebView->page()->mainFrame()->toHtml();
        QRegExp token = QRegExp("\"xsrft\":\"([^\"]+)");
        if (token.indexIn(html) > -1)
        {
            qDebug() << "Inserting token into form" << token.cap(1);
            formElement.appendInside("<input value=\"" + token.cap(1) + "\" name=\"token\" type=\"hidden\"></input>");
        }
    }
}

void video_vimeo::handleLogin(QUrl url)
{
    QList<QNetworkCookie> cookies = dui->loginDialogWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(url);
    for (uint i = 0; i < cookies.length(); i++)
    {
        if (cookies.at(i).name().contains("_password"))
        {
            this->handler->networkAccessManager->cookieJar()->setCookiesFromUrl(dui->loginDialogWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(url), url);
            passwordDialog->accept();
        }
    }
}
