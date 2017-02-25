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



#ifndef VIDEO_FACEBOOK_H
#define VIDEO_FACEBOOK_H

#include "video.h"
#include "ui_login_dialog.h"
#include "QtWebKit"

class video_facebook : public video
{
    Q_OBJECT

public:
    video_facebook();
    video* createNewInstance();

    bool setUrl(QString);
    virtual void parseVideo(QString html);

private:
    QDialog* passwordDialog;
    Ui::LoginDialog* dui;
    bool authenticating;

public slots:
    void handleLogin();
    void acceptLoginDialog();
};

#endif // VIDEO_FACEBOOK_H
