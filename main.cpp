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



#include <QtGui/QApplication>
#include <QTranslator>
#include "mainwindow.h"
#include "clipgrab.h"

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Philipp Schmieder");
    QCoreApplication::setOrganizationDomain("clipgrab.de");
    QCoreApplication::setApplicationName("ClipGrab");
    QCoreApplication::setApplicationVersion(QString(STRINGIZE_VALUE_OF(CLIPGRAB_VERSION)).replace("\"", ""));
    QSplashScreen splash(QPixmap(":/img/splash.png"), Qt::FramelessWindowHint);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    splash.setMask(QPixmap(":/img/splash.png").mask());
    splash.show();

    ClipGrab cg;

    QTranslator translator;
    QSettings settings;
    QString locale = settings.value("Language", "auto").toString();
    if (locale == "auto")
    {
        locale = QLocale::system().name();
    }
    translator.load(QString(":/lng/clipgrab_") + locale);
    app.installTranslator(&translator);
    for (int i=0; i < cg.languages.length(); i++)
    {
        if (cg.languages[i].code == locale) {
            if (cg.languages[i].isRTL)
            {
                QApplication::setLayoutDirection(Qt::RightToLeft);
            }
            break;
        }
    }

    MainWindow w;
    w.cg = &cg;
    w.init();
    w.show();
    splash.finish(w.centralWidget());
    cg.getUpdateInfo();
    return app.exec();
}
