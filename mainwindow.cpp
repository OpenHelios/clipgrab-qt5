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



#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    ui.setupUi(this);
    currentVideo = NULL;
    searchReply = NULL;
}


MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("isMaximized", isMaximized());
    if (!isMaximized()) { // the position and size is only valid if not maximized
        settings.setValue("x", x());
        settings.setValue("y", y());
        settings.setValue("width", width());
        settings.setValue("height", height());
    }
}

void MainWindow::init()
{
    //*
    //* Adding version info to the footer
    //*
    this->ui.label->setText(ui.label->text().replace("%version", "ClipGrab " + QCoreApplication::applicationVersion()));

    //*
    //* Tray Icon
    //*
    systemTrayIcon.setIcon(QIcon(":/img/icon.png"));
    systemTrayIcon.show();
    connect(&systemTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systemTrayIconActivated(QSystemTrayIcon::ActivationReason)));
    connect(&systemTrayIcon, SIGNAL(messageClicked()), this, SLOT(systemTrayMessageClicked()));

    //*
    //* Clipboard Handling
    //*
    connect(cg, SIGNAL(compatiblePortalFound(bool, video*)), this, SLOT(compatiblePortalFound(bool, video*)));
    connect(cg, SIGNAL(compatibleUrlFoundInClipboard(QString)), this, SLOT(compatibleUrlFoundInClipBoard(QString)));

    //*
    //* Download Tab
    //*
    connect(ui.downloadStart, SIGNAL(clicked()), this, SLOT(startDownload()));
    connect(ui.downloadLineEdit, SIGNAL(textChanged(QString)), cg, SLOT(determinePortal(QString)));
    connect(this, SIGNAL(itemToRemove(int)), cg, SLOT(removeDownload(int)));
    //connect(ui.downloadTree, SIGNAL(doubleClicked(QModelIndex)), this, openFinishedVideo(QModelIndex));
    ui.downloadTree->header()->setResizeMode(1, QHeaderView::Stretch);
    ui.downloadTree->header()->setStretchLastSection(false);
    ui.downloadTree->header()->setResizeMode(3, QHeaderView::ResizeToContents);
    ui.downloadLineEdit->setFocus(Qt::OtherFocusReason);

    int lastFormat = cg->settings.value("LastFormat", 0).toInt();
    for (int i = 0; i < this->cg->formats.size(); ++i)
    {
        this->ui.downloadComboFormat->addItem(this->cg->formats.at(i)._name);
    }
    //"Fix" for Meego: this->ui.downloadComboFormat->addItem(this->cg->formats.at(0)._name);

    this->ui.downloadComboFormat->setCurrentIndex(lastFormat);

    ui.downloadPause->hide(); //Qt does currently not handle throttling downloads properly


    //*
    //* Search Tab
    //*
    this->ui.searchWebView->setContextMenuPolicy(Qt::NoContextMenu);

    this->searchNam = new QNetworkAccessManager();

    //*
    //* Settings Tab
    //*
    connect(this->ui.settingsRadioClipboardAlways, SIGNAL(toggled(bool)), this, SLOT(settingsClipboard_toggled(bool)));
    connect(this->ui.settingsRadioClipboardNever, SIGNAL(toggled(bool)), this, SLOT(settingsClipboard_toggled(bool)));
    connect(this->ui.settingsRadioClipboardAsk, SIGNAL(toggled(bool)), this, SLOT(settingsClipboard_toggled(bool)));
    connect(this->ui.settingsRadioNotificationsAlways, SIGNAL(toggled(bool)), this, SLOT(settingsNotifications_toggled(bool)));
    connect(this->ui.settingsRadioNotificationsFinish, SIGNAL(toggled(bool)), this, SLOT(settingsNotifications_toggled(bool)));
    connect(this->ui.settingsRadioNotificationsNever, SIGNAL(toggled(bool)), this, SLOT(settingsNotifications_toggled(bool)));


    this->ui.settingsSavedPath->setText(cg->settings.value("savedPath", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation)).toString());
    this->ui.settingsSaveLastPath->setChecked(cg->settings.value("saveLastPath", true).toBool());
    ui.settingsNeverAskForPath->setChecked(cg->settings.value("NeverAskForPath", false).toBool());

    ui.settingsUseMetadata->setChecked(cg->settings.value("UseMetadata", false).toBool());
    connect(this->ui.settingsUseMetadata, SIGNAL(stateChanged(int)), this, SLOT(on_settingsUseMetadata_stateChanged(int)));


    ui.settingsUseProxy->setChecked(cg->settings.value("UseProxy", false).toBool());
    ui.settingsProxyAuthenticationRequired->setChecked(cg->settings.value("ProxyAuthenticationRequired", false).toBool());
    ui.settingsProxyHost->setText(cg->settings.value("ProxyHost", "").toString());
    ui.settingsProxyPassword->setText(cg->settings.value("ProxyPassword", "").toString());
    ui.settingsProxyPort->setValue(cg->settings.value("ProxyPort", "").toInt());
    ui.settingsProxyUsername->setText(cg->settings.value("ProxyUsername", "").toString());
    ui.settingsProxyType->setCurrentIndex(cg->settings.value("ProxyType", 0).toInt());

    connect(this->ui.settingsUseProxy, SIGNAL(toggled(bool)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyAuthenticationRequired, SIGNAL(toggled(bool)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyHost, SIGNAL(textChanged(QString)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyPassword, SIGNAL(textChanged(QString)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyPort, SIGNAL(valueChanged(int)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyUsername, SIGNAL(textChanged(QString)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsProxyChanged()));
    settingsProxyChanged();
    ui.settingsMinimizeToTray->setChecked(cg->settings.value("MinimizeToTray", false).toBool());

    if (cg->settings.value("Clipboard", "ask") == "always")
    {
        ui.settingsRadioClipboardAlways->setChecked(true);
    }
    else if (cg->settings.value("Clipboard", "ask") == "never")
    {
        ui.settingsRadioClipboardNever->setChecked(true);
    }
    else
    {
        ui.settingsRadioClipboardAsk->setChecked(true);
    }

    if (cg->settings.value("Notifications", "finish") == "finish")
    {
        ui.settingsRadioNotificationsFinish->setChecked(true);
    }
    else if (cg->settings.value("Notifications", "finish") == "always")
    {
        ui.settingsRadioNotificationsAlways->setChecked(true);
    }
    else
    {
        ui.settingsRadioNotificationsNever->setChecked(true);
    }

    ui.settingsRememberLogins->setChecked(cg->settings.value("rememberLogins", true).toBool());
    ui.settingsRememberVideoQuality->setChecked(cg->settings.value("rememberVideoQuality", true).toBool());
    ui.settingsUseWebM->setChecked(cg->settings.value("UseWebM", false).toBool());
    ui.settingsRemoveFinishedDownloads->setChecked(cg->settings.value("RemoveFinishedDownloads", false).toBool());
    ui.settingsIgnoreSSLErrors->setChecked(cg->settings.value(("IgnoreSSLErrors"), false).toBool());


    int langIndex = 0;
    for (int i=0; i < cg->languages.count(); i++)
    {
        if (cg->languages.at(i).code == cg->settings.value("Language", "auto").toString())
        {
            langIndex = i;
        }
    }
    for (int i=0; i < cg->languages.count(); i++)
    {
        ui.settingsLanguage->addItem(cg->languages.at(i).name, cg->languages.at(i).code);
    }
    ui.settingsLanguage->setCurrentIndex(langIndex);


    this->ui.tabWidget->removeTab(2); //fixme!

    startTimer(500);

    //*
    //* About Tab
    //*

    #ifdef Q_WS_MAC
        this->ui.downloadOpen->hide();
        this->cg->settings.setValue("Clipboard", "always");
        this->ui.generalSettingsTabWidget->removeTab(2);
        this->setStyleSheet("#label_4{padding-top:25px}#label{font-size:10px}#centralWidget{background:#fff};#mainTab{margin-top;-20px};#label_4{padding:10px}#downloadInfoBox, #settingsGeneralInfoBox, #settingsLanguageInfoBox, #aboutInfoBox, #searchInfoBox{color: background: #00B4DE;}");
        this->ui.label_4->setMinimumHeight(120);
    #endif

    //*
    //* Drag and Drop
    //*
    this->setAcceptDrops(true);
    this->ui.searchWebView->setAcceptDrops(false);

    //*
    //*Keyboard shortcuts
    //*
    QSignalMapper* tabShortcutMapper = new QSignalMapper(this);

    QShortcut* tabShortcutSearch = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_1), this);
    tabShortcutMapper->setMapping(tabShortcutSearch, 0);
    QShortcut* tabShortcutDownload = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_2), this);
    tabShortcutMapper->setMapping(tabShortcutDownload, 1);
    QShortcut* tabShortcutSettings = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_3), this);
    tabShortcutMapper->setMapping(tabShortcutSettings, 2);
    QShortcut* tabShortcutAbout = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_4), this);
    tabShortcutMapper->setMapping(tabShortcutAbout, 3);

    connect(tabShortcutSearch, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutDownload, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutSettings, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutAbout, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutMapper, SIGNAL(mapped(int)), this->ui.mainTab, SLOT(setCurrentIndex(int)));

    QApplication::quit();

    //*
    //*Miscellaneous
    //*
    on_searchLineEdit_textChanged("");
    this->ui.mainTab->setCurrentIndex(cg->settings.value("MainTab", 0).toInt());

    //Prevent updating remembered resolution when updating programatically
    this->updatingComboQuality = false;

    #ifdef Q_OS_MACX
    //if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    //{
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    //}
    #endif
}

void MainWindow::startDownload()
{
    QString targetDirectory = cg->settings.value("savedPath", QDesktopServices::storageLocation(QDesktopServices::DesktopLocation)).toString();
    QString fileName = currentVideo->getSaveTitle();

    if (cg->settings.value("NeverAskForPath", false).toBool() == false)
    {
        #ifdef Q_WS_MAC64
            targetFileSelected(NSSavePanelWrapper::getSaveFileName(tr("Select Target"), targetDirectory, fileName));
        #elif defined(Q_WS_MAC)

            QFileDialog* dialog = new QFileDialog(this->ui.widget);
            dialog->setDirectory(targetDirectory);
            dialog->setAcceptMode(QFileDialog::AcceptSave);
            dialog->setOption(QFileDialog::DontUseNativeDialog, false);
            dialog->setModal(true);
            dialog->selectFile(fileName);
            connect(dialog, SIGNAL(fileSelected(QString)), this, SLOT(targetFileSelected(QString)));
            dialog->open();
        #else
            targetFileSelected(QFileDialog::getSaveFileName(this, tr("Select Target"), targetDirectory +"/" + fileName));
        #endif

    }
    else
    {
        targetFileSelected(targetDirectory + "/" + fileName);
    }
}

void MainWindow::targetFileSelected(QString target)
{
    if (!target.isEmpty())
    {
        QTreeWidgetItem* tmpItem = new QTreeWidgetItem(QStringList() << currentVideo->getName() << currentVideo->title() << ui.downloadComboFormat->currentText());
        tmpItem->setSizeHint(0, QSize(16, 24));
        currentVideo->setTreeItem(tmpItem);
        currentVideo->setQuality(ui.downloadComboQuality->currentIndex());
        currentVideo->setConverter(cg->formats.at(ui.downloadComboFormat->currentIndex())._converter->createNewInstance(),cg->formats.at(ui.downloadComboFormat->currentIndex())._mode);

        if (cg->settings.value("saveLastPath", true) == true)
        {
            QString targetDir = target;
            targetDir.remove(targetDir.split("/", QString::SkipEmptyParts).last()).replace(QRegExp("/+"), "/");
            ui.settingsSavedPath->setText(targetDir);
        }
        currentVideo->setTargetPath(target);

        if (cg->settings.value("UseMetadata", false).toBool() == true)
        {
            if (ui.downloadComboFormat->currentIndex() == 4 || ui.downloadComboFormat->currentIndex() == 5)
            {
                metadataDialog = new QDialog;
                mdui.setupUi(metadataDialog);
                mdui.title->setText(currentVideo->title());
                metadataDialog->setModal(true);
                metadataDialog->exec();

                currentVideo->setMetaTitle(mdui.title->text());
                currentVideo->setMetaArtist(mdui.artist->text());

                delete metadataDialog;
            }
        }

        cg->addDownload(currentVideo);
        ui.downloadTree->insertTopLevelItem(0, tmpItem);

        currentVideo->_progressBar = new QProgressBar();
        currentVideo->_progressBar->setValue(0);
        currentVideo->_progressBar->setMaximum(1);
        ui.downloadTree->setItemWidget(tmpItem, 3, currentVideo->_progressBar);

        ((QProgressBar*) ui.downloadTree->itemWidget(tmpItem, 3))->setMaximum(100);
        connect(currentVideo, SIGNAL(progressChanged(int,int)),ui.downloadTree, SLOT(update()));
        connect(currentVideo, SIGNAL(conversionFinished(video*)), this, SLOT(handleFinishedConversion(video*)));
        currentVideo = NULL;
        ui.downloadLineEdit->clear();
    }
}

void MainWindow::compatiblePortalFound(bool found, video* portal)
{
    disableDownloadUi(true);
    this->updatingComboQuality = true;
    ui.downloadComboQuality->clear();
    this->updatingComboQuality = false;
    if (found == true)
    {
        ui.downloadLineEdit->setReadOnly(true);
        ui.downloadInfoBox->setText(tr("Please wait while ClipGrab is loading information about the video ..."));

        if (currentVideo)
        {
            currentVideo->deleteLater();;
        }
        currentVideo = portal->createNewInstance();
        currentVideo->setUrl(ui.downloadLineEdit->text());
        connect(currentVideo, SIGNAL(error(QString,video*)), cg, SLOT(errorHandler(QString,video*)));
        connect(currentVideo, SIGNAL(analysingFinished()), this, SLOT(updateVideoInfo()));
        currentVideo->analyse();

    }
    else
    {
        if (ui.downloadLineEdit->text() == "")
        {
            ui.downloadInfoBox->setText(tr("Please enter the link to the video you want to download in the field below."));
        }
        else if (ui.downloadLineEdit->text().startsWith("http://") || ui.downloadLineEdit->text().startsWith("https://"))
        {            
            ui.downloadLineEdit->setReadOnly(true);
            ui.downloadInfoBox->setText(tr("The link you have entered seems to not be recognised by any of the supported portals.<br/>Now ClipGrab will check if it can download a video from that site anyway."));

            if (currentVideo)
            {
                currentVideo->deleteLater();;
            }
            currentVideo = cg->heuristic->createNewInstance();
            currentVideo->setUrl(ui.downloadLineEdit->text());
            connect(currentVideo, SIGNAL(error(QString,video*)), cg, SLOT(errorHandler(QString,video*)));
            connect(currentVideo, SIGNAL(analysingFinished()), this, SLOT(updateVideoInfo()));
            currentVideo->analyse();
        }
    }
}

void MainWindow::updateVideoInfo()
{
    QList< QPair<QString, int> > supportedQualities = currentVideo->getSupportedQualities();

    if (currentVideo && currentVideo->title() != "" && !supportedQualities.isEmpty())
    {
        this->updatingComboQuality = true;
        ui.downloadInfoBox->setText("<strong>" + currentVideo->title() + "</strong>");
        ui.downloadLineEdit->setReadOnly(false);
        disableDownloadUi(false);
        ui.downloadComboQuality->clear();
        int rememberedResolution = cg->settings.value("rememberedVideoQuality", -1).toInt();
        int bestResolutionMatch = 0;
        int bestResolutionMatchPosition = 0;
        for (int i = 0; i < supportedQualities.length(); i++)
        {
            int resolution = supportedQualities.at(i).second;
            ui.downloadComboQuality->addItem(supportedQualities.at(i).first, resolution);
            if (resolution <= rememberedResolution && resolution > bestResolutionMatch)
            {
                bestResolutionMatch = resolution;
                bestResolutionMatchPosition = i;
            }
        }
        if (cg->settings.value("rememberVideoQuality", true).toBool())
        {
            ui.downloadComboQuality->setCurrentIndex(bestResolutionMatchPosition);
        }
        this->updatingComboQuality = false;
    }
    else
    {
        ui.downloadInfoBox->setText(tr("No downloadable video could be found.<br />Maybe you have entered the wrong link or there is a problem with your connection."));
        ui.downloadLineEdit->setReadOnly(false);
        disableDownloadUi(true);
    }
}
void MainWindow::on_settingsSavedPath_textChanged(QString string)
{
    this->cg->settings.setValue("savedPath", string);
}

void MainWindow::on_settingsBrowseTargetPath_clicked()
{
    QString newPath;
    newPath = QFileDialog::getExistingDirectory(this, tr("ClipGrab - Select target path"), ui.settingsSavedPath->text());
    if (!newPath.isEmpty())
    {
        ui.settingsSavedPath->setText(newPath);
    }
}

void MainWindow::on_downloadCancel_clicked()
{
    if (ui.downloadTree->currentIndex().row() != -1)
    {
        int item = ui.downloadTree->topLevelItemCount() - ui.downloadTree->currentIndex().row() -1;
        ui.downloadTree->takeTopLevelItem(ui.downloadTree->currentIndex().row());
        emit itemToRemove(item);
    }
}

void MainWindow::on_settingsSaveLastPath_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        cg->settings.setValue("saveLastPath", true);
    }
    else
    {
        cg->settings.setValue("saveLastPath", false);
    }
}

void MainWindow::on_downloadOpen_clicked()
{
    if (ui.downloadTree->currentIndex().row() != -1)
    {
        QString targetDir;
        targetDir = cg->getDownloadTargetPath(ui.downloadTree->topLevelItemCount()-1 - ui.downloadTree->currentIndex().row());
        targetDir.remove(targetDir.split("/").last());
        QDesktopServices::openUrl(targetDir);
    }
}


void MainWindow::compatibleUrlFoundInClipBoard(QString url)
{
    if (QApplication::activeModalWidget() == 0)
    {
        if (cg->settings.value("Clipboard", "ask") == "ask")
        {
            Notifications::showMessage(tr("ClipGrab: Video discovered in your clipboard"), tr("ClipGrab has discovered the address of a compatible video in your clipboard. Click on this message to download it now."), &systemTrayIcon);
        }
        else if (cg->settings.value("Clipboard", "ask") == "always")
        {
            this->ui.downloadLineEdit->setText(url);
            this->ui.mainTab->setCurrentIndex(1);
            this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            this->show();
            this->activateWindow();
            this->raise();
        }
    }

}

void MainWindow::systemTrayMessageClicked()
{
    if (QApplication::activeModalWidget() == 0)
    {
        this->ui.downloadLineEdit->setText(cg->clipboardUrl);
        this->ui.mainTab->setCurrentIndex(1);
        this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        this->show();
        this->activateWindow();
        this->raise();
    }
    

}

void MainWindow::systemTrayIconActivated(QSystemTrayIcon::ActivationReason)
{
    if (cg->settings.value("MinimizeToTray", false).toBool() && !this->isHidden())
    {
        this->hide();
    }
    else
    {
        this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        this->show();
        this->activateWindow();
        this->raise();
    }
}

void MainWindow::settingsClipboard_toggled(bool)
{
    if (ui.settingsRadioClipboardAlways->isChecked())
    {
        cg->settings.setValue("Clipboard", "always");
    }
    else if (ui.settingsRadioClipboardNever->isChecked())
    {
        cg->settings.setValue("Clipboard", "never");
    }
    else if (ui.settingsRadioClipboardAsk->isChecked())
    {
        cg->settings.setValue("Clipboard", "ask");
    }
}

void MainWindow::disableDownloadUi(bool disable)
{
    ui.downloadComboFormat->setDisabled(disable);
    ui.downloadComboQuality->setDisabled(disable);
    ui.downloadStart->setDisabled(disable);
    ui.label_2->setDisabled(disable);
    ui.label_3->setDisabled(disable);
}

void MainWindow::disableDownloadTreeButtons(bool disable)
{
    ui.downloadOpen->setDisabled(disable);
    ui.downloadCancel->setDisabled(disable);
    ui.downloadPause->setDisabled(disable);
}

void MainWindow::on_downloadTree_currentItemChanged(QTreeWidgetItem* /*current*/, QTreeWidgetItem* /*previous*/)
{
    if (ui.downloadTree->currentIndex().row() == -1)
    {
        disableDownloadTreeButtons();
    }
    else
    {
        disableDownloadTreeButtons(false);
    }
}

void MainWindow::on_settingsNeverAskForPath_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        cg->settings.setValue("NeverAskForPath", true);
    }
    else
    {
        cg->settings.setValue("NeverAskForPath", false);
    }
}

 void MainWindow::closeEvent(QCloseEvent *event)
 {
    if (cg->downloadsRunning() > 0)
    {
        QMessageBox* exitBox;
        exitBox = new QMessageBox(QMessageBox::Question, tr("ClipGrab - Exit confirmation"), tr("There is still at least one download in progress.<br />If you exit the program now, all downloads will be canceled and cannot be recovered later.<br />Do you really want to quit ClipGrab now?"), QMessageBox::Yes|QMessageBox::No);
        if (exitBox->exec() == QMessageBox::Yes)
        {
            cg->cancelAll();
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
       event->accept();;
    }
 }

 void MainWindow::settingsNotifications_toggled(bool)
 {
    if (ui.settingsRadioNotificationsAlways->isChecked())
    {
        cg->settings.setValue("Notifications", "always");
    }
    else if (ui.settingsRadioNotificationsFinish->isChecked())
    {
        cg->settings.setValue("Notifications", "finish");
    }
    else if (ui.settingsRadioNotificationsNever->isChecked())
    {
        cg->settings.setValue("Notifications", "never");
    }
 }

 void MainWindow::handleFinishedConversion(video* finishedVideo)
 {
     if (cg->settings.value("Notifications", "finish") == "always")
     {
         Notifications::showMessage(tr("Download finished"), tr("Downloading and converting “%title” is now finished.").replace("%title", finishedVideo->title()), &systemTrayIcon);
     }
     else if (cg->downloadsRunning() == 0 && cg->settings.value("Notifications", "finish") == "finish")
     {
         Notifications::showMessage(tr("All downloads finished"), tr("ClipGrab has finished downloading and converting all selected videos."), &systemTrayIcon);
     }

     if (cg->settings.value("RemoveFinishedDownloads", false) == true)
     {
         int finishedItemIndex = ui.downloadTree->indexOfTopLevelItem(finishedVideo->treeItem());
         int item = ui.downloadTree->topLevelItemCount() - finishedItemIndex -1;
         ui.downloadTree->takeTopLevelItem(finishedItemIndex);
         emit itemToRemove(item);
     }
 }

void MainWindow::on_settingsRemoveFinishedDownloads_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        cg->settings.setValue("RemoveFinishedDownloads", true);
    }
    else
    {
        cg->settings.setValue("RemoveFinishedDownloads", false);
    }
}

void MainWindow::on_downloadPause_clicked()
{
    if (ui.downloadTree->currentIndex().row() != -1)
    {
        int item = ui.downloadTree->topLevelItemCount() - ui.downloadTree->currentIndex().row() -1;
        cg->pauseDownload(item);
    }
}

void MainWindow::settingsProxyChanged()
{
    cg->settings.setValue("UseProxy", ui.settingsUseProxy->isChecked());
    cg->settings.setValue("ProxyHost", ui.settingsProxyHost->text());
    cg->settings.setValue("ProxyPort", ui.settingsProxyPort->value());
    cg->settings.setValue("ProxyAuthenticationRequired", ui.settingsProxyAuthenticationRequired->isChecked());
    cg->settings.setValue("ProxyPassword", ui.settingsProxyPassword->text());
    cg->settings.setValue("ProxyUsername", ui.settingsProxyUsername->text());
    cg->settings.setValue("ProxyType", ui.settingsProxyType->currentIndex());
    ui.settingsProxyGroup->setEnabled(ui.settingsUseProxy->isChecked());
    ui.settingsProxyAuthenticationRequired->setEnabled(ui.settingsUseProxy->isChecked());
    if (ui.settingsUseProxy->isChecked())
    {
        if (ui.settingsProxyAuthenticationRequired->isChecked())
        {
            ui.settingsProxyAuthenticationGroup->setEnabled(ui.settingsProxyAuthenticationRequired->isChecked());
        }
    }
    else
    {
        ui.settingsProxyAuthenticationGroup->setEnabled(false);
        ui.settingsProxyAuthenticationRequired->setEnabled(false);
     }
    cg->activateProxySettings();
}

void MainWindow::timerEvent(QTimerEvent *)
{
    QPair<qint64, qint64> downloadProgress = cg->downloadProgress();
    if (downloadProgress.first != 0 && downloadProgress.second != 0)
    {
        #ifdef Q_WS_X11
            systemTrayIcon.setToolTip("<strong style=\"font-size:14px\">" + tr("ClipGrab") + "</strong><br /><span style=\"font-size:13px\">" + QString::number(downloadProgress.first*100/downloadProgress.second) + " %</span><br />" + QString::number((double)downloadProgress.first/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB") + "/" + QString::number((double)downloadProgress.second/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB"));
        #else
        systemTrayIcon.setToolTip(tr("ClipGrab") + " - " + QString::number(downloadProgress.first*100/downloadProgress.second) + " % - " + QString::number((double)downloadProgress.first/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB") + "/" + QString::number((double)downloadProgress.second/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" KiB"));
        #endif
        setWindowTitle("ClipGrab - " + QString::number(downloadProgress.first*100/downloadProgress.second) + " %");
    }
    else
    {
        #ifdef Q_WS_X11
            systemTrayIcon.setToolTip("<strong style=\"font-size:14px\">" + tr("ClipGrab") + "</strong><br /><span style=\"font-size:13px\">" + tr("Currently no downloads in progress."));
        #endif
        systemTrayIcon.setToolTip(tr("ClipGrab") + tr("Currently no downloads in progress."));
        setWindowTitle(tr("ClipGrab - Download and Convert Online Videos"));
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (isMinimized() && cg->settings.value("MinimizeToTray", false).toBool())
        {
            QTimer::singleShot(500, this, SLOT(hide()));
            event->ignore();
        }
    }
}

void MainWindow::on_settingsMinimizeToTray_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        cg->settings.setValue("MinimizeToTray", true);
    }
    else
    {
        cg->settings.setValue("MinimizeToTray", false);
    }
}


void MainWindow::on_downloadLineEdit_returnPressed()
{
    if (currentVideo)
    {
        if (!currentVideo->title().isEmpty())
        {
            this->startDownload();
        }
    }
}

void MainWindow::on_label_linkActivated(QString link)
{
    QDesktopServices::openUrl(link);
}

void MainWindow::on_settingsUseMetadata_stateChanged(int state)
{
    if (state == Qt::Checked)
    {
        cg->settings.setValue("UseMetadata", true);
    }
    else
    {
        cg->settings.setValue("UseMetadata", false);
    }
}

void MainWindow::on_searchLineEdit_textChanged(QString keywords)
{
    if (!keywords.isEmpty())
    {
    if (searchReply)
    {
        searchReply->abort();
        searchReply->deleteLater();
    }
        this->searchReply = searchNam->get(QNetworkRequest(QUrl("https://www.youtube.com/results?search_query==" + keywords.replace(QRegExp("[&\\?%\\s]"), "+"))));
        connect(this->searchReply, SIGNAL(finished()), this, SLOT(processSearchReply()));
    }
    else
    {
        this->searchReply = searchNam->get(QNetworkRequest(QUrl("https://www.youtube.com")));
        connect(this->searchReply, SIGNAL(finished()), this, SLOT(processSearchReply()));
    }
}

void MainWindow::processSearchReply()
{
    QWebView* view = new QWebView();
    view->setHtml(searchReply->readAll());
    QWebFrame *dom = view->page()->mainFrame();

    int limit = 12;
    if (searchReply->url().toString() == "https://www.youtube.com") {
        limit = 4;
    }


    QWebElementCollection entries = dom->findAllElements(".yt-lockup");

    QString searchHtml;
    searchHtml.append("<style>body {margin:0;padding:0;width:100%;left:0px;top:0px;font-family:'Segoe UI', Ubuntu, sans-serif} img{position:relative;top:-22px;left:-15px} .entry{display:block;position:relative;width:50%;float:left;height:100px;} a{color:#1a1a1a;text-decoration:none;} span.title{display:block;font-weight:bold;font-size:14px;position:absolute;left:140px;top:16px;} span.duration{display:block;font-size:11px;position:absolute;left:140px;top:70px;color:#aaa}  span.thumbnail{width:120px;height:68px;background:#00b2de;display:block;overflow:hidden;position:absolute;left:8px;top:16px;} a:hover{background: #00b2de;color:#fff}</style>");
    searchHtml.append("<body>");

    int i=0;
    foreach (QWebElement entry, entries)
    {
        QString duration = entry.findFirst(".video-time").toPlainText();
        QString thumbnail = entry.findFirst(".yt-thumb img").attribute("data-thumb");
        if (thumbnail.isEmpty())
        {
            thumbnail = entry.findFirst(".yt-thumb img").attribute("src");
        }
        if (thumbnail.startsWith("//"))
        {
            thumbnail.prepend("https:");
        }

        QString link = entry.findFirst("a.spf-link").attribute("href", "");
        if (!link.startsWith("/watch")) {
            continue;
        }

        searchHtml.append("<a href=\"https://www.youtube.com" + link + "\" class=\"entry\">");
        searchHtml.append("<span class=\"title\">" + entry.findFirst("h3 a.spf-link").toPlainText() + "</span>");
        searchHtml.append("<span class=\"duration\">" + duration + "</span>");
        searchHtml.append("<span class=\"thumbnail\"><img src=\"" + thumbnail + "\"  /></span");
        searchHtml.append("</a>");

        i++;
        if (i == limit) {
            break;
        }
    }

    searchHtml.append("</body>");
    this->ui.searchWebView->setHtml(searchHtml);
    ui.searchWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

}

void MainWindow::on_searchWebView_linkClicked(QUrl url)
{
    this->ui.downloadLineEdit->setText(url.toString());;
    this->ui.mainTab->setCurrentIndex(1);
}

void MainWindow::on_downloadComboFormat_currentIndexChanged(int index)
{
    cg->settings.setValue("LastFormat", index);
}

void MainWindow::on_mainTab_currentChanged(int index)
{
    cg->settings.setValue("MainTab", index);
}

void MainWindow::on_downloadTree_doubleClicked(QModelIndex /*index*/)
{
    if (ui.downloadTree->currentIndex().row() != -1)
    {
        if (cg->isDownloadFinished(ui.downloadTree->topLevelItemCount()-1))
        {
            QString targetFile;
            cg->downloadProgress();
            targetFile = cg->getDownloadTargetPath(ui.downloadTree->topLevelItemCount()-1 - ui.downloadTree->currentIndex().row());
            QUrl targetFileUrl = QUrl::fromLocalFile(targetFile);
            qDebug() << targetFileUrl << targetFileUrl.isValid();
            qDebug() << QDesktopServices::openUrl(targetFileUrl);
        }
    }
}

void MainWindow::on_settingsLanguage_currentIndexChanged(int index)
{
    cg->settings.setValue("Language", cg->languages.at(index).code);

}

void MainWindow::on_buttonDonate_clicked()
{
    QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=AS6TDMR667GJL"));
}

void MainWindow::on_settingsUseWebM_toggled(bool checked)
{
    cg->settings.setValue("UseWebM", checked);
}

void MainWindow::on_settingsIgnoreSSLErrors_toggled(bool checked)
{
    cg->settings.setValue("IgnoreSSLErrors", checked);
}

void MainWindow::on_downloadTree_customContextMenuRequested(const QPoint &pos)
{

    QTreeWidgetItem* item = ui.downloadTree->itemAt(pos);
    int selectedVideo = ui.downloadTree->topLevelItemCount()-1 - ui.downloadTree->indexAt(pos).row();

    if (selectedVideo == -1 || ui.downloadTree->indexAt(pos).row() == -1)
    {
        return;
    }

    QMenu contextMenu;
    QAction* openDownload = contextMenu.addAction(tr("&Open downloaded file"));
    QAction* openFolder = contextMenu.addAction(tr("Open &target folder"));
    contextMenu.addSeparator();
    QAction* pauseDownload = contextMenu.addAction(tr("&Pause download"));
    QAction* restartDownload = contextMenu.addAction(tr("&Restart download"));
    QAction* cancelDownload = contextMenu.addAction(tr("&Cancel download"));
    contextMenu.addSeparator();
    QAction* copyLink = contextMenu.addAction(tr("Copy &video link"));
    QAction* openLink = contextMenu.addAction(tr("Open video link in &browser"));


    if (cg->isDownloadPaused(selectedVideo))
    {
        pauseDownload->setText(tr("Resume download"));
    }

    if (cg->isDownloadFinished(selectedVideo))
    {
        contextMenu.removeAction(pauseDownload);
        contextMenu.removeAction(restartDownload);
        contextMenu.removeAction(cancelDownload);
        #ifdef Q_WS_MAC
            openFolder->setText(tr("Show in &Finder"));
        #endif
    }
    else {
        contextMenu.removeAction(openDownload);
    }


    QAction* selectedAction = contextMenu.exec(ui.downloadTree->mapToGlobal(pos));
    if (selectedAction == restartDownload)
    {
        cg->restartDownload(selectedVideo);
    }
    else if (selectedAction == cancelDownload)
    {
        cg->cancelDownload(selectedVideo);
    }
    else if (selectedAction == pauseDownload)
    {
        cg->pauseDownload(selectedVideo);
    }
    else if (selectedAction == openFolder)
    {
        if (cg->isDownloadFinished(selectedVideo))
        {
            #ifdef Q_WS_MAC
                QProcess* finderProcess = new QProcess();
                QStringList arguments;
                arguments << "-e" << "tell application \"Finder\"";
                arguments << "-e" << "reveal POSIX file \"" + cg->getDownloadSaveFileName(selectedVideo) + "\"";
                arguments << "-e" << "activate";
                arguments << "-e" << "end tell";
                finderProcess->start("osascript", arguments);
                return;
            #elif defined(Q_WS_WIN)
                QProcess* explorerProcess = new QProcess();
                explorerProcess->start("explorer.exe /select,"  + QDir::toNativeSeparators(cg->getDownloadSaveFileName(selectedVideo)));
                return;
            #endif
        }

        QString path = QFileInfo(cg->getDownloadTargetPath(selectedVideo)).absoluteDir().path();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
    else if (selectedAction == openDownload)
    {
        QString path = cg->getDownloadSaveFileName(selectedVideo);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
    else if (selectedAction == openLink)
    {
        QString link = cg->getDownloadOriginalUrl(selectedVideo);
        QDesktopServices::openUrl(QUrl(link));
    }
    else if (selectedAction == copyLink)
    {
        QApplication::clipboard()->setText(cg->getDownloadOriginalUrl(selectedVideo));
    }

    contextMenu.deleteLater();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << event->mimeData()->formats().join(", ");
    if (event->mimeData()->hasFormat("text/uri-list"))
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    ui.downloadLineEdit->setText(event->mimeData()->text());
    ui.mainTab->setCurrentIndex(1);
}

void MainWindow::on_settingsRememberLogins_toggled(bool checked)
{
    cg->settings.setValue("rememberLogins", checked);
    cg->settings.setValue("youtubeRememberLogin", checked);
    cg->settings.setValue("facebookRememberLogin", checked);
    if (!checked)
    {
        cg->settings.remove("youtubeCookies");
        cg->settings.remove("facebookCookies");
    }
}

void MainWindow::on_settingsRememberVideoQuality_toggled(bool checked)
{
    cg->settings.setValue("rememberVideoQuality", checked);
}

void MainWindow::on_downloadComboQuality_currentIndexChanged(int index)
{
    if (this->updatingComboQuality || !cg->settings.value("rememberVideoQuality", true).toBool() || !this->currentVideo)
    {
        return;
    }

    cg->settings.setValue("rememberedVideoQuality", ui.downloadComboQuality->itemData(index, Qt::UserRole).toInt());
}
