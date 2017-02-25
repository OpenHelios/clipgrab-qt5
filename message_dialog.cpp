#include "message_dialog.h"
#include "ui_message_dialog.h"

messageDialog::messageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::messageDialog)
{
    ui->setupUi(this);
}

messageDialog::~messageDialog()
{
    delete ui;
}

void messageDialog::setUrl(QUrl url)
{
    ui->webView->setUrl(url);
}

void messageDialog::setLinkDelegationPolicy(QWebPage::LinkDelegationPolicy policy)
{
    ui->webView->page()->setLinkDelegationPolicy(policy);
    this->linkDelegationPolicy = policy;
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(handleLink(QUrl)));
}

void messageDialog::handleLink(QUrl url)
{
    if (this->linkDelegationPolicy == QWebPage::DelegateExternalLinks && url.host() == ui->webView->url().host())
    {
        this->ui->webView->load(url);
        return;
    }

    QDesktopServices::openUrl(url);
}
