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



#include "video_facebook.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	#include <QtWebKitWidgets>
#endif

video_facebook::video_facebook()
{
    this->_name = "Facebook";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("(http[s]?://(www\\.)?facebook\\.com.*/videos).*/(\\d+)", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://(www\\.)?facebook\\.com.*/(?:pg/)?.*/videos/.*", Qt::CaseInsensitive);
    _treeItem = NULL;
    this->authenticating = false;

    QSettings settings;
    QString serializedCookies = settings.value("facebookCookies", "").toString();
    if (!serializedCookies.isEmpty())
    {
        QList<QNetworkCookie> cookies = this->handler->deserializeCookies(serializedCookies);
        this->handler->networkAccessManager->cookieJar()->setCookiesFromUrl(cookies, QUrl("https://www.facebook.com/"));
    }
}

video* video_facebook::createNewInstance()
{
    return new video_facebook();
}

bool video_facebook::setUrl(QString url)
{
    _originalUrl = url;

    if (_urlRegExp.first().indexIn(url) > -1) {
        this->_url = QUrl(_urlRegExp.first().cap(1) + "/" + _urlRegExp.first().cap(3));
    } else if (_urlRegExp.last().indexIn(url) > -1) {
        this->_url = QUrl(originalUrl());
    }

    if (_url.isValid())
    {
        return true;
    }
    return false;
}


void video_facebook::parseVideo(QString data)
{
    QSettings settings;
    QRegExp expression;

    expression = QRegExp("\"?videoData\"?:(\\[\\{.*\\}\\])");
    expression.setMinimal(true);

    //If there is no video data, login might be required
    if (expression.indexIn(data) == -1) {
        if (this->authenticating)
        {
            //If we’re already in the process of authenticating
            //and no video data has been found
            return;
        }
        this->authenticating = true;
        dui = new Ui::LoginDialog();
        passwordDialog = new QDialog;
        dui->setupUi(passwordDialog);
        connect(dui->loginDialogWebView, SIGNAL(loadFinished(bool)), this, SLOT(handleLogin()));
        connect(this, SIGNAL(analysingFinished()), this, SLOT(acceptLoginDialog()));
        dui->loginDialogWebView->setUrl(QUrl::fromUserInput("https://m.facebook.com/login.php?next=" + QUrl::toPercentEncoding(this->_url.toString())));
        dui->rememberLogin->setChecked(settings.value("facebookRememberLogin", true).toBool());

        if (passwordDialog->exec() == QDialog::Accepted)
        {
            passwordDialog->deleteLater();
            return;
        }

        passwordDialog->deleteLater();
        emit error("Could not retrieve video info.", this);
        emit analysingFinished();
        return;
    }

    //Parse video information
    QString videoData = expression.cap(1);
    QList< QPair<QRegExp, QString> > supportedQualities;
    supportedQualities.append(qMakePair(QRegExp("\"?hd_src_no_ratelimit\"?:\"([^\"]+)"), tr("HD")));
    supportedQualities.append(qMakePair(QRegExp("\"?hd_src\"?:\"([^\"]+)"), tr("HD")));
    supportedQualities.append(qMakePair(QRegExp("\"?sd_src_no_ratelimit\"?:\"([^\"]+)"), tr("normal")));
    supportedQualities.append(qMakePair(QRegExp("\"?sd_src\"?:\"([^\"]+)"), tr("normal")));

    for (int i = 0; i < supportedQualities.length(); i++)
    {
        QString quality = supportedQualities.at(i).second;
        QRegExp expression = supportedQualities.at(i).first;

        bool isNew = true;
        for (int j = 0; j < this->_supportedQualities.length(); j++) {
            if (this->_supportedQualities.at(j).quality == quality)
            {
                isNew = false;
                break;
            }
        }
        if (!isNew)
        {
            continue;
        }

        if (expression.indexIn(videoData) == -1)
        {
            continue;
        }
        QString videoLink = expression.cap(1).replace("\\/", "/");
        videoQuality newQuality = videoQuality(quality, videoLink);
        newQuality.containerName = ".mp4";
        if  (newQuality.quality == tr("HD"))
        {
            newQuality.resolution = 720;
        }
        else {
            newQuality.resolution = 360;
        }
        this->_supportedQualities.append(newQuality);
    }

    //Obtain page title
    expression = QRegExp("<title.*>(.*)</title>");
    expression.setMinimal(true);
    if (expression.indexIn(data) == -1)
    {
        emit error("Could not retrieve video title.", this);
        emit analysingFinished();
        return;
    }
    this->_title = expression.cap(1);

    emit analysingFinished();
}

void video_facebook::handleLogin()
{
    QString html = dui->loginDialogWebView->page()->mainFrame()->toHtml();
    this->parseVideo(html);
}

void video_facebook::acceptLoginDialog()
{
    QSettings settings;
    if (dui->rememberLogin->isChecked())
    {
        QList<QNetworkCookie> cookies = dui->loginDialogWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(this->_url);
        settings.setValue("facebookCookies", this->handler->serializeCookies(cookies));
    }
    else
    {
        settings.remove("facebookCookies");
    }
    settings.setValue("facebookRememberLogin", dui->rememberLogin->isChecked());
    passwordDialog->accept();
}
