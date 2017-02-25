#ifndef MESSAGE_DIALOG_H
#define MESSAGE_DIALOG_H

#include <QDialog>
#include <QWebPage>
#include <QDesktopServices>

namespace Ui {
class messageDialog;
}

class messageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit messageDialog(QWidget *parent = 0);
    ~messageDialog();
    void setUrl(QUrl url);
    void setLinkDelegationPolicy(QWebPage::LinkDelegationPolicy);

private:
    Ui::messageDialog *ui;
    QWebPage::LinkDelegationPolicy linkDelegationPolicy;

public slots:
    void handleLink(QUrl);
};

#endif // MESSAGE_DIALOG_H
