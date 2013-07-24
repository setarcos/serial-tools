#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
    class Dialog;
}

class QSerialPort;
class QComboBox;
class HLed;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

protected:
    void closeEvent(QCloseEvent *event);
private Q_SLOTS:
    void onOpenCloseAClicked();
    void onOpenCloseBClicked();
    void onSendButtonClicked();
    void onReadyReadA();
    void onReadyReadB();
    void onModeChange(int m);
    void LogMsg(const QByteArray& s, int ch);
private:
    void OpenCloseCom(QSerialPort *port, QComboBox *com, QComboBox *baud, HLed *led);
    int mode;
    Ui::Dialog *ui;
    QSerialPort *portA;
    QSerialPort *portB;
};

#endif // DIALOG_H
