#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
    class Dialog;
}

class QSerialPort;
class QComboBox;
class HLed;
class QTimer;

const int CH_A = 0x10;
const int CH_IN = 0x01;
enum BT_STATE{
    BT_IDLE, BT_TEST,
    OPEN_MASTER, ADDR_MASTER, BAUD_MASTER, ROLE_MASTER, PSWD_MASTER,
    OPEN_SLAVE, BAUD_SLAVE, ROLE_SLAVE, PSWD_SLAVE, RESET_SLAVE, BIND_SLAVE,
};

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
    void onClearClicked();
    void onSendButtonClicked();
    void onTimerTimeout();
    void onReadyReadA();
    void onReadyReadB();
    void onModeChange(int m);
    void onContinuous(int s);
    void processA();
    void processB();
    void portAWrite(const QByteArray &buf);
    void portBWrite(const QByteArray &buf);
    void LogMsg(const QByteArray& s, int ch);
    void LogBin(const QByteArray& s, const QString &fname);
private:
    void OpenCloseCom(QSerialPort *port, QComboBox *com, QComboBox *baud, HLed *led);
    int mode;
    Ui::Dialog *ui;
    QSerialPort *portA;
    QSerialPort *portB;
    QTimer *timer;
    QByteArray *bufA;
    QByteArray *bufB;
    QByteArray *masterAddr;
    int state;
};

#endif // DIALOG_H
