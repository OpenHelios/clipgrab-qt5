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



#include "video_youtube.h"

#include "QMutableListIterator"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	#include <QtWebKitWidgets>
#endif

video_youtube::video_youtube()
{
    this->_name = "Youtube";
    this->_supportsTitle = true;
    this->_supportsDescription = true;
    this->_supportsThumbnail = true;
    this->_supportsSearch = true;
    this->_icon = 0;
    this->_urlRegExp << QRegExp("http[s]?://\\w*\\.youtube\\.com/watch.*v\\=.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://\\w*\\.youtube\\.com/view_play_list\\?p\\=.*&v\\=.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://youtu.be/.*", Qt::CaseInsensitive);
    this->_urlRegExp << QRegExp("http[s]?://w*\\.youtube\\.com/embed/.*", Qt::CaseInsensitive);

    QSettings settings;
    QString serializedCookies = settings.value("youtubeCookies", "").toString();
    if (!serializedCookies.isEmpty())
    {
        QList<QNetworkCookie> cookies = this->handler->deserializeCookies(serializedCookies);
        this->handler->networkAccessManager->cookieJar()->setCookiesFromUrl(cookies, QUrl("https://www.youtube.com/"));
    }
}

video* video_youtube::createNewInstance()
{
    return new video_youtube();
}

bool video_youtube::setUrl(QString url)
{
    _originalUrl = url;

    if (_url.isEmpty())
    {
        url.replace("#!", "?");
        url.replace("youtube.com/embed/", "youtube.com/watch?v=");
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

QString video_youtube::getFmtLink(QStringList qualityLinks, QString fmt)
{
    QRegExp urlExpression;
    urlExpression = QRegExp("(http[s]?[^,]+)");

    QRegExp itagExpression;
    itagExpression = QRegExp("[,]?itag=([^,]+)");

    for (int i=0; i < qualityLinks.size(); i++)
    {
       bool urlExpressionMatch = (urlExpression.indexIn(qualityLinks.at(i)) > -1);
       bool itagExpressionMatch = (itagExpression.indexIn(qualityLinks.at(i)) > -1 );

       if (urlExpressionMatch && itagExpressionMatch && itagExpression.cap(1) == fmt)
       {
           return qualityLinks.at(i);
       }
    }
    return "";
}

QString video_youtube::getUrlFromFmtLink(QString link)
{
    QRegExp urlExpression;
    urlExpression = QRegExp("(http[s]?[^,]+)");

    if (urlExpression.indexIn(link) > -1)
    {
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        QString url = QUrl::fromEncoded(QUrl::fromEncoded(urlExpression.cap(1).toAscii()).toString().toAscii()).toString();
#else
        QString url = QUrl::fromPercentEncoding(QUrl::fromPercentEncoding(urlExpression.cap(1).toUtf8()).toUtf8());
#endif

        QRegExp sigExpression;
        sigExpression = QRegExp("(?:^|[^a-zA-Z])[,]?s(ig)?=([^,]+)");

        if (sigExpression.indexIn(link) > -1)
        {
            QString signature;
            if (sigExpression.cap(1) == "ig")
            {
                signature = sigExpression.cap(2);
            }
            else
            {
                signature = parseSignature(sigExpression.cap(2));
            }

            url = url.append("&signature=").append(signature);

        }

        QSettings settings;
        if (settings.value("UseRateBypass", true).toBool() && !url.contains("ratebypass"))
        {
            url = url.append("&ratebypass=yes");
        }

        return url;

    }

    return "";
}

QString video_youtube::getQualityFromFmtLink(QString link)
{
    QRegExp qualityExpression;
    qualityExpression = QRegExp("quality_label=([^,]+)");

    if (qualityExpression.indexIn(link) > -1)
    {
        QString quality = qualityExpression.cap(1);
        if (quality == "1080p")
        {
            return tr("HD (1080p)");
        }
        else if (quality == "1440p")
        {
            return tr("HD (1440p)");
        }
        else if (quality == "2160p")
        {
            return tr("4K (2160p)");
        }
        else if (quality == "2880p")
        {
            return tr("5K (2880p)");
        }
        else if (quality == "4320p")
        {
            return tr("8K (4320p)");
        }

        return qualityExpression.cap(1);
    }

    return "";
}

void video_youtube::extractJSMethod(QString name, QString js) {
    for (int i = 0; i < this->jsMethods.length(); i++) {
        if (this->jsMethods.at(i).name == name) {
            return;
        }
    }

    QString escapedName = name;
    escapedName.replace("$", "\\$");
    QRegExp expression("((?:function\\s*" + escapedName + "|(var\\s*|,\\s*|\\n)" + escapedName + "\\s*=(?:\\s*function)?)\\s*(\\([\\w,\\s]*\\))?\\s*)(\\{.*\\})");
    expression.setCaseSensitivity(Qt::CaseSensitive);
    if (expression.indexIn((js)) != -1) {
        jsMethod method;
        method.name = name;
        QString descriptor = expression.cap(1);
        if (!expression.cap(2).isEmpty()) {
            descriptor = descriptor.right(descriptor.length() - expression.cap(2).length());
        }
        QString code = expression.cap(4);
        QString parameters = expression.cap(3).replace(QRegExp("[\\(\\)\\s]"), "");

        for (qint64 i = 1; i < code.length(); i++) {
            QString partial = code.left(i);
            if (partial.count("{") == partial.count("}")) {
                method.code = descriptor +  partial;
                break;
            }
        }
        if (!method.code.isEmpty()) {
            this->jsMethods.append(method);

            expression = QRegExp("([\\w\\$]+)(?:[\\w\\.\\$])*\\s*(\\([\\w\\s,\"\\$]*)\\)");
            int pos = expression.indexIn(method.code);
            QStringList expressions;
            expressions << "function" << "return" << "if" << "elseif";
            expressions.append(parameters.split(","));

            while (pos != -1) {
                if (expressions.indexOf(expression.cap(1)) == -1) {
                    this->extractJSMethod(expression.cap(1), js);
                }
                pos = expression.indexIn(method.code, pos + expression.cap(0).length());
            }
        }
    }
}

void video_youtube::parseJS(QString js)
{
    QString methodName;

    QRegExp expression("signature=|set\\(\"signature\",([$\\w]+)\\(");
    expression.setMinimal(true);
    if (expression.indexIn(js) != -1)
    {
        methodName = expression.cap(1);
        this->signatureMethodName = methodName;
        extractJSMethod(methodName, js);
    }
}

void video_youtube::parseDashMpd(QString xml)
{
    QDomDocument mpd;
    mpd.setContent(xml);
    QDomNodeList adaptionSets = mpd.elementsByTagName("AdaptationSet");

    for (int i = 0; i < adaptionSets.length(); i++) {
        QDomElement set = adaptionSets.at(i).toElement();
        if (set.attribute(("mimeType")) == "video/mp4" || set.attribute(("mimeType")) == "audio/mp4") {
            QDomNodeList representations = set.elementsByTagName(("Representation"));
            for (int j = 0; j < representations.length(); j++) {
                QDomElement representation = representations.at(j).toElement();

                QString baseUrl;
                QStringList segments;
                if (representation.elementsByTagName("BaseURL").length() == 1) {
                    baseUrl = representation.elementsByTagName("BaseURL").at(0).toElement().text();
                    QDomNode initializationNode = representation.elementsByTagName("Initialization").at(0);
                    QDomNodeList segmentNodes = representation.elementsByTagName("SegmentURL");
                    if (initializationNode.isElement() && !segmentNodes.isEmpty())  {
                        segments << baseUrl + initializationNode.toElement().attribute("sourceURL");
                        for (int k = 0; k < segmentNodes.length(); k++) {
                            segments << baseUrl + segmentNodes.at(k).toElement().attribute("media");
                        }
                    }
                }
                if (!segments.empty()) {
                    this->dashQualityLinks << segments;
                }
            }
        }
    }
    adaptionSets.at(8).toDocument().elementsByTagName("Representation");
}

void video_youtube::parseVideo(QString html)
{
    QSettings settings;
    QRegExp expression;

    //Obtain basic info from web page
    if (this->downloading.isEmpty() || this->downloading == "html")
    {
        this->html = html;

        //Is the access to the video restricted to authenticated users?
        if (html.contains("age-gate-content"))
        {
            expression = QRegExp("<div id=\"watch7-player-age-gate-content\">.*<button.*href=\"([^\"]+)\"");
            expression.setMinimal(true);
            if (expression.indexIn(html) != -1)
            {
                dui = new Ui::LoginDialog();
                passwordDialog = new QDialog;
                dui->setupUi(passwordDialog);
                connect(dui->loginDialogWebView, SIGNAL(urlChanged(QUrl)), this, SLOT(handleLoginUrlChanged(QUrl)));
                connect(dui->loginDialogWebView, SIGNAL(loadFinished(bool)), this, SLOT(handleLoginLoadFinished()));
                dui->loginDialogWebView->setUrl(QUrl::fromUserInput(expression.cap(1).replace("&amp;", "&")));
                dui->rememberLogin->setChecked(settings.value("youtubeRememberLogin", true).toBool());
                dui->loginDialogWebView->setFocus();

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
        }

        //Does this video support MPEG-DASH?
        expression = QRegExp("\"dashmpd\": ?\"([^\"]*)");
        expression.setMinimal(false);
        bool hasMPDSignature = false;
        if (this->dashmpd.isEmpty() && expression.indexIn(html) != -1)
        {
            QString url = expression.cap(1).replace("\\", "");
            this->requiredDownloads["dashmpd"] = url;
            hasMPDSignature = url.split("/").contains("s");
        }

        //Is signature parsing required?
        expression = QRegExp("\"url_encoded_fmt_stream_map\": ?\"[^\"]*([^a-z])(s=)");
        expression.setMinimal(true);
        if (this->js.isEmpty() && (hasMPDSignature || expression.indexIn(html) != -1))
        {
            this->html = html;
            QString html5PlayerUrl;

            expression = QRegExp("html5player.+\\.js");
            expression.setMinimal(true);
            if (expression.indexIn(html) !=-1)
            {
                html5PlayerUrl = "http://s.ytimg.com/yts/jsbin/" + expression.cap(0).replace("\\/", "/");
            }
            if (html5PlayerUrl.isEmpty())
            {
                expression = QRegExp("//[^\"]*html5player[^\"]*\\.js");
                expression.setMinimal(true);
                if (expression.indexIn(html) !=-1)
                {
                    html5PlayerUrl = "https:" + expression.cap(0);
                }
            }
            if (html5PlayerUrl.isEmpty())
            {
                expression = QRegExp("(//?)[^\"]*player[^\"]*\\/base\\.js");
                expression.setMinimal(true);
                if (expression.indexIn(html) !=-1)
                {
                    if (expression.cap(1) == "//")
                    {
                        html5PlayerUrl = "https:" + expression.cap(0);
                    } else {
                        html5PlayerUrl = "https://www.youtube.com" + expression.cap(0);
                    }
                }
            }

            if (!html5PlayerUrl.isEmpty())
            {
                qDebug() << "Using player URL: " << html5PlayerUrl;
                this->requiredDownloads["js"] = html5PlayerUrl;
            }
            else
            {
                emit error("Could not retrieve player URL", this);
            }
        }
    }
    //Parse player JS
    else if (this->downloading == "js")
    {
        parseJS(html);
        this->js = "";

        for (int i = 0; i < this->jsMethods.length(); i++) {
            this->js.append(jsMethods.at(i).code).append(";");
        }
    }
    //Parse MPEG-DASH MPD
    else if (this->downloading == "dashmpd")
    {
        parseDashMpd(html);
        this->dashmpd = html;
    }

    //Check if additional urls need to be downloaded
    if (!this->requiredDownloads.empty()) {
        QString key;
        QString url;
        //Always download player first
        if (requiredDownloads.contains("js")) {
            key = "js";
            url = requiredDownloads["js"];
        }
        else {
            key = this->requiredDownloads.begin().key();
            url = this->requiredDownloads.begin().value();
        }

        //Signature parsing for MPEG-DASH MPD
        if (key == "dashmpd") {
            QStringList urlFragments = url.split("/");
            int signaturePosition = urlFragments.indexOf("s");
            if (signaturePosition > -1) {
                urlFragments.replace(signaturePosition, "signature");
                urlFragments.replace(signaturePosition + 1, this->parseSignature(urlFragments.at(signaturePosition + 1)));
                url = urlFragments.join("/");
            }
        }

        this->downloading = key;
        this->requiredDownloads.remove(key);
        handler->addDownload(url);
        return;
    }

    //Continue parsing the html
    if (!this->html.isEmpty())
    {
        html = this->html;
    }

    expression = QRegExp("<meta name=\"title\" content=\"(.*)\"");
    expression.setMinimal(true);
    if (expression.indexIn(html) !=-1)
    {
        _title = QString(expression.cap(1)).replace("&amp;quot;", "\"").replace("&amp;amp;", "&").replace("&#39;", "'").replace("&quot;", "\"").replace("&amp;", "&");
        QStringList qualityLinks;


        expression = QRegExp("\"adaptive_fmts\": ?\"(.*)\"");
        expression.setMinimal(true);
        if (expression.indexIn(html)!=-1 && expression.cap(1) != "")
        {
            qualityLinks << expression.cap(1).split(",");
        }

        expression = QRegExp("\"url_encoded_fmt_stream_map\": ?\"(.*)\"");
        expression.setMinimal(true);

        if (expression.indexIn(html)!=-1 && expression.cap(1) != "")
        {
            qualityLinks << expression.cap(1).split(",");
        }

        if (!qualityLinks.isEmpty() || ! this->dashQualityLinks.isEmpty())
        {

            qualityLinks.replaceInStrings("\\u0026", ",");

            if (settings.value("UseWebM", false).toBool() == false)
            {
                fmtQualities << fmtQuality("144p", 144, "160", "139");

                fmtQualities << fmtQuality("240p", 240, "5");
                fmtQualities << fmtQuality("240p", 240, "133", "139");

                fmtQualities << fmtQuality("360p", 360, "34");
                fmtQualities << fmtQuality("360p", 360, "18");
                fmtQualities << fmtQuality("360p", 360, "134", "140");

                fmtQualities << fmtQuality("480p", 480, "35");
                fmtQualities << fmtQuality("480p", 480, "135", "140");

                fmtQualities << fmtQuality("HD (720p)", 720, "22");
                fmtQualities << fmtQuality("HD (720p)", 720, "136", "141");

                fmtQualities << fmtQuality("HD (1080p)", 1080, "37");
                fmtQualities << fmtQuality("HD (1080p)", 1080, "137", "141");

                fmtQualities << fmtQuality("HD (1440p)", 1440, "264", "141");

                fmtQualities << fmtQuality("4K (2160p)", 2160, "38");
                fmtQualities << fmtQuality("4K (2160p)", 2160, "266", "141");

                fmtQualities << fmtQuality("HD (720p60)", 720, "298", "141");
                fmtQualities << fmtQuality("HD (1080p60)", 1080, "299", "141");

                fmtQualities << fmtQuality(tr("Original"), 3000, "138", "141");
            }
            else
            {
                fmtQualities << fmtQuality("240p", 240, "242", "172");

                fmtQualities << fmtQuality("360p", 360, "43");
                fmtQualities << fmtQuality("360p", 360, "243", "172");
                fmtQualities << fmtQuality("360p", 360, "167", "172");

                fmtQualities << fmtQuality("480p", 480, "44");
                fmtQualities << fmtQuality("480p", 480, "244", "172");
                fmtQualities << fmtQuality("480p", 480, "168", "172");

                fmtQualities << fmtQuality("HD (720p)", 720, "45");
                fmtQualities << fmtQuality("HD (720p)", 720, "247", "172");
                fmtQualities << fmtQuality("HD (720p)", 720, "169", "172");

                fmtQualities << fmtQuality("HD (1080p)", 1080, "46");
                fmtQualities << fmtQuality("HD (1080p)", 1080, "248", "172");
                fmtQualities << fmtQuality("HD (1080p)", 1080, "170", "172");

                fmtQualities << fmtQuality("HD (1440p)", 1440, "271", "172");

                fmtQualities << fmtQuality("4K (2160p)", 2160, "313", "172");

                fmtQualities << fmtQuality("HD (720p60)", 720, "302", "172");
                fmtQualities << fmtQuality("HD (1080p60)", 1080,"303", "172");
                fmtQualities << fmtQuality("HD (1440p60)", 1440, "308", "172");
                fmtQualities << fmtQuality("4K (2160p60)", 2160, "315", "172");

                fmtQualities << fmtQuality(tr("Original"), 3000, "272", "172");
            }

			QMutableListIterator<fmtQuality> i(fmtQualities);
			i.toBack();
			while (i.hasPrevious())
            {
				fmtQuality q = i.previous();

                //Only consider DASH qualities if supported by ffmpeg/avconv
                if (settings.value("DashSupported", false).toBool() == false)
                {
                    continue;
                }

                //Extract links
                bool videoIsFmt = true, audioIsFmt = true;
                QString videoLink = getFmtLink(qualityLinks, q.video);
                QString audioLink = !q.audio.isEmpty() ? getFmtLink(qualityLinks, q.audio) : "";
                QStringList videoLinkSegments, audioLinkSegments;

                //If preferred audio couldn’t be found
                if (!q.audio.isEmpty() && !videoLink.isEmpty() && audioLink.isEmpty())
                {
                    if (q.audio == "141")
                    {
                        audioLink = getFmtLink(qualityLinks, "140");
                    }
                    else if (q.audio == "172")
                    {
                        audioLink = getFmtLink(qualityLinks, "171");
                    }
                }

                //Try MPEG-Dash if Fmt link wasn’t found
                if (videoLink.isEmpty() || (!q.audio.isEmpty() && audioLink.isEmpty()))
                {
                    QRegExp itagExpression = QRegExp("[/]?itag/([^/]+)");

                    for (int j = 0; j < this->dashQualityLinks.length(); j++) {
                        if (!(itagExpression.indexIn(dashQualityLinks.at(j).at(0)) > -1))
                        {
                            continue;
                        }

                        if (videoLink.isEmpty() && itagExpression.cap(1) == q.video)
                        {
                            videoLink = dashQualityLinks.at(j).at(0);
                            videoLinkSegments = QStringList(dashQualityLinks.at(j));
                            videoLinkSegments.removeFirst();
                            videoIsFmt = false;
                        }
                        else if (audioLink.isEmpty() && itagExpression.cap(1) == q.audio)
                        {
                            audioLink = dashQualityLinks.at(j).at(0);
                            audioLinkSegments = QStringList(dashQualityLinks.at(j));
                            audioLinkSegments.removeFirst();
                            audioIsFmt = false;
                        }
                    }

                    //Look for alternative lower-quality audio
                    if (!videoLink.isEmpty() && !q.audio.isEmpty() && audioLink.isEmpty()) {
                        for (int j = 0; j < this->dashQualityLinks.length(); j++) {
                            if (!(itagExpression.indexIn(dashQualityLinks.at(j).at(0)) > -1))
                            {
                                continue;
                            }

                            if (q.audio == "141" && itagExpression.cap(1) == "140")
                            {
                                audioLink = dashQualityLinks.at(j).at(0);
                                audioLinkSegments = QStringList(dashQualityLinks.at(j));
                                audioLinkSegments.removeFirst();
                                audioIsFmt = false;
                            }
                        }

                    }
                }

                //If current quality set was found
                if ((!videoLink.isEmpty()) && (audioLink.isEmpty() == q.audio.isEmpty()))
                {
                    videoQuality newQuality;
                    newQuality.resolution = q.resolution;

                    QRegExp formatExpression = QRegExp("webm|flv");
                    newQuality.containerName = ".mp4";
                    if (formatExpression.indexIn(videoLink) > -1)
                    {
                        newQuality.containerName = "." + formatExpression.cap(0);
                    }

                    //Set quality name
                    newQuality.quality = q.quality;
                    if (q.quality == tr("Original")) {
                        newQuality.quality = getQualityFromFmtLink(videoLink);
                        if (newQuality.quality.isEmpty()){
                            newQuality.quality = tr("Original");
                        }
                    }

                    if (videoIsFmt)
                    {
                        newQuality.videoUrl = getUrlFromFmtLink(videoLink);
                    }
                    else
                    {
                        newQuality.videoUrl = videoLink;
                        newQuality.videoSegments = videoLinkSegments;
                    }

                    if (!audioLink.isEmpty() && audioIsFmt)
                    {
                        newQuality.audioUrl = getUrlFromFmtLink(audioLink);
                    }
                    else if (!audioLink.isEmpty()) {
                        newQuality.audioUrl = audioLink;
                        newQuality.audioSegments = audioLinkSegments;
                    }


                    newQuality.chunkedDownload = false;
                    if (!newQuality.videoUrl.contains("ratebypass=yes") && settings.value("UseChunkedDownload", true).toBool())
                    {
                        newQuality.chunkedDownload = true;
                    }

                    _supportedQualities.append(newQuality);

                    QMutableListIterator<fmtQuality> ii(fmtQualities);
                    while (ii.hasNext())
                    {
                        if (ii.next().quality == newQuality.quality)
                        {
                            ii.value().video.clear();
                            ii.value().audio.clear();
                        }
                    }
                }
            }
        }
        else if (expression.indexIn(html)!=-1 && expression.cap(1) == "")
        {
            expression = QRegExp("\"t\": \"(.*)\"");
            expression.setMinimal(true);
            QRegExp expression2 = QRegExp("\"video_id\": \"(.*)\"");
            expression2.setMinimal(true);
            if (expression.indexIn(html) !=-1 && expression2.indexIn(html) !=-1)
            {
                videoQuality newQuality;
                newQuality.quality = tr("normal");
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
                newQuality.videoUrl = QUrl::fromEncoded(QString("http://www.youtube.com/get_video?video_id=" + expression2.cap(1) + "&t=" + expression.cap(1)).toAscii()).toString(QUrl::None);
#else
                newQuality.videoUrl = QUrl::fromEncoded(QString("http://www.youtube.com/get_video?video_id=" + expression2.cap(1) + "&t=" + expression.cap(1)).toLatin1()).toString(QUrl::None);
#endif
                _supportedQualities.append(newQuality);
            }
            else
            {
                emit error("Could not retrieve video link.", this);

            }
        }
        else
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

QString video_youtube::parseSignature(QString s)
{
    qDebug() << "received signature: " << s;

    QWebView* view = new QWebView();
    view->setHtml("<script>" + this->js + "</script>");
    s = view->page()->mainFrame()->evaluateJavaScript(this->signatureMethodName + "(\"" + s + "\")").toString();
    qDebug() << "parsed signature: " << s;
    view->deleteLater();

    return s;
}



void video_youtube::handleLoginUrlChanged(const QUrl url)
{
    if (!this->compatibleWithUrl(url.toString()))
    {
        return;
    }

    QList<QNetworkCookie> cookies = dui->loginDialogWebView->page()->networkAccessManager()->cookieJar()->cookiesForUrl(url);

    QSettings settings;
    if (dui->rememberLogin->isChecked())
    {
        settings.setValue("youtubeCookies", this->handler->serializeCookies(cookies));
    }
    else
    {
        settings.remove("youtubeCookies");
    }
    settings.setValue("youtubeRememberLogin", dui->rememberLogin->isChecked());

    this->handler->networkAccessManager->cookieJar()->setCookiesFromUrl(cookies, url);
    passwordDialog->accept();
}

void video_youtube::handleLoginLoadFinished()
{
    dui->loginDialogWebView->history()->clear();
}
