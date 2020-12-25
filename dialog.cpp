#include "dialog.h"
#include "ui_dialog.h"
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QTime>
#include <QSettings>
#include <QMessageBox>
#include <QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    portA(new QSerialPort()), portB(new QSerialPort()),
    timer(new QTimer()),
    bufA(new QByteArray()), bufB(new QByteArray()), masterAddr(new QByteArray()),
    state(0)
{
    ui->setupUi(this);
    QStringList comname;
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList)
        comname << serialPortInfo.portName();
    ui->cbPortA->addItems(comname);
    ui->cbPortB->addItems(comname);

    ui->ledA->turnOff();
    ui->ledB->turnOff();

    connect(ui->openCloseA, SIGNAL(clicked()), SLOT(onOpenCloseAClicked()));
    connect(ui->openCloseB, SIGNAL(clicked()), SLOT(onOpenCloseBClicked()));
    connect(ui->sendButton, SIGNAL(clicked()), SLOT(onSendButtonClicked()));
    connect(ui->pbClear, SIGNAL(clicked()), SLOT(onClearClicked()));
    connect(portA, SIGNAL(readyRead()), SLOT(onReadyReadA()));
    connect(portB, SIGNAL(readyRead()), SLOT(onReadyReadB()));
    connect(ui->cbMode, SIGNAL(currentIndexChanged(int)), SLOT(onModeChange(int)));
    connect(ui->cbCont, SIGNAL(stateChanged(int)), SLOT(onContinuous(int)));
    connect(timer, SIGNAL(timeout()), SLOT(onTimerTimeout()));

    setWindowTitle(QLatin1String("Serial Port Utility"));
    QSettings set("Pluto", "comloger");
    mode = set.value("mode", 0).toInt();
    ui->cbMode->setCurrentIndex(mode);
    onModeChange(mode);
    ui->cbPortA->setCurrentText(set.value("portAname", "").toString());
    ui->cbPortB->setCurrentText(set.value("portBname", "").toString());
    ui->cbBaudA->setCurrentText(set.value("portAbaud", "115200").toString());
    ui->cbBaudB->setCurrentText(set.value("portBbaud", "115200").toString());
    ui->cbAuto->setCheckState((Qt::CheckState)set.value("auto", 0).toInt());
    ui->cbHexA->setCheckState((Qt::CheckState)set.value("HexA", 0).toInt());
    ui->cbHexB->setCheckState((Qt::CheckState)set.value("HexB", 0).toInt());
    *masterAddr = set.value("masterAddr", "").toByteArray();
    if (ui->cbAuto->checkState() == Qt::Checked) {
        if (mode != 0) onOpenCloseBClicked();
        onOpenCloseAClicked();
    }
}

Dialog::~Dialog()
{
    delete ui;
    delete portA;
    delete portB;
    delete bufA;
    delete bufB;
    delete masterAddr;
    delete timer;
}

void Dialog::OpenCloseCom(QSerialPort *port, QComboBox *com, QComboBox *baud, HLed *led)
{
    if (port->isOpen()) {
        port->close();
    } else {
        port->setPortName(com->currentText());
        port->open(QIODevice::ReadWrite);
        port->setBaudRate((QSerialPort::BaudRate)baud->currentText().toInt());
    }
    if (port->isOpen())
        led->turnOn();
    else
        led->turnOff();
}

void Dialog::onOpenCloseAClicked()
{
    OpenCloseCom(portA, ui->cbPortA, ui->cbBaudA, ui->ledA);
    // Master
    if (portA->isOpen()) {
        state = OPEN_MASTER;
        portAWrite("AT\r\n"); // Test for conf mode
        timer->start(200);
    }
}

void Dialog::portAWrite(const QByteArray &buf)
{
    portA->write(buf);
    LogMsg(buf, CH_A);
}

void Dialog::portBWrite(const QByteArray &buf)
{
    portB->write(buf);
    LogMsg(buf, 0);
}

void Dialog::onOpenCloseBClicked()
{
    OpenCloseCom(portB, ui->cbPortB, ui->cbBaudB, ui->ledB);
    // Slave
    if (portB->isOpen()) {
        state = OPEN_SLAVE;
        portBWrite("AT\r\n"); // Test for conf mode
        timer->start(400);
    }
}

void Dialog::onSendButtonClicked()
{
    if (portA->isOpen() && !ui->sendEdit->toPlainText().isEmpty()) {
        QByteArray buf = ui->sendEdit->toPlainText().toLatin1();
        if (ui->cbHexA->checkState() == Qt::Checked)
            portA->write(QByteArray::fromHex(buf));
        else
            portA->write(buf);
        LogMsg(buf, CH_A);
    }
}

void Dialog::onTimerTimeout(void)
{
    timer->stop();
    if (state == OPEN_MASTER) {
        state = BT_IDLE;  // It's OK, just not in conf mode.
        return;
    }
    if (state == BT_TEST) {
        portBWrite("BT TEST\r\n");
        return;
    }
    QMessageBox::warning(this, "Info", "Something wrong, timeout");
    state = 0;
}

void Dialog::onModeChange(int m)
{
    mode = m;
    ui->cbPortB->setEnabled(mode != 0);
    ui->cbBaudB->setEnabled(mode != 0);
    ui->openCloseB->setEnabled(mode != 0);
    ui->cbHexB->setEnabled(mode != 0);
    ui->sendButton->setEnabled(mode != 2);
    if (mode == 0) {
        ui->ledB->turnOff();
        if (portB->isOpen()) portB->close();
    }
}

void Dialog::onClearClicked()
{
    ui->recvEdit->clear();
}

void Dialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QSettings set("Pluto", "comloger");
    set.setValue("mode", ui->cbMode->currentIndex());
    set.setValue("portAname", ui->cbPortA->currentText());
    set.setValue("portBname", ui->cbPortB->currentText());
    set.setValue("portAbaud", ui->cbBaudA->currentText());
    set.setValue("portBbaud", ui->cbBaudB->currentText());
    set.setValue("auto", ui->cbAuto->checkState());
    set.setValue("HexA", ui->cbHexA->checkState());
    set.setValue("HexB", ui->cbHexB->checkState());
    set.setValue("masterAddr", QString::fromLatin1(*masterAddr));
}

void Dialog::processA()
{
    *bufA = bufA->trimmed();
    if (state == ADDR_MASTER) {
        if (bufA->indexOf("ADDR") >= 0) *masterAddr = bufA->mid(6);
    }
    if (state == BT_TEST) {
        if (bufA->indexOf("BT TEST") == 0) {
            QMessageBox::information(this, "OK", "This one is OK");
            timer->stop();
            state = BT_IDLE;
        }
    }
    if (bufA->indexOf("OK") == 0) {
        switch (state) {
            case OPEN_MASTER:
                timer->stop();
                state = ADDR_MASTER;
                portAWrite("AT+ADDR?\r\n");
                break;
            case ADDR_MASTER:
                state = BAUD_MASTER;
                portAWrite("AT+UART=38400,0,0\r\n");
                break;
            case BAUD_MASTER:
                state = ROLE_MASTER;
                portAWrite("AT+ROLE=1\r\n");
                break;
            case ROLE_MASTER:
                state = PSWD_MASTER;
                portAWrite("AT+PSWD=1234\r\n");
                break;
            case PSWD_MASTER:
                state = BT_IDLE;
                portAWrite("AT+RESET\r\n");
                break;
        }
    }
    bufA->clear();
}

void Dialog::processB()
{
    *bufB = bufB->trimmed();
    if (bufB->indexOf("OK") == 0) {
        switch (state) {
            case OPEN_SLAVE:
                timer->stop();
                state = BAUD_SLAVE;
                portBWrite("AT+UART=38400,0,0\r\n");
                break;
            case BAUD_SLAVE:
                state = ROLE_SLAVE;
                portBWrite("AT+ROLE=0\r\n");
                break;
            case ROLE_SLAVE:
                state = BIND_SLAVE;
                portBWrite("AT+BIND=" + masterAddr->replace(':', ',') + "\r\n");
                break;
            case BIND_SLAVE:
                state = PSWD_SLAVE;
                portBWrite("AT+PSWD=1234\r\n");
                break;
            case PSWD_SLAVE:
                state = BT_TEST;
                portBWrite("AT+RESET\r\n");
                timer->start(2000);
                break;
        }
    }
    bufB->clear();
}

void Dialog::onReadyReadA()
{
    if (portA->bytesAvailable()) {
        QByteArray ba = portA->readAll();
        LogBin(ba, "comA.bin");
        bufA->append(ba);
        if (bufA->indexOf('\r') > 0) processA();
        if ((mode == 2) && (portB->isOpen())) portB->write(ba);
        if (ui->cbHexA->checkState() == Qt::Checked)
            LogMsg(ba.toHex().toUpper(), CH_A | CH_IN);
        else
            LogMsg(ba, CH_A | CH_IN);
    }
}

void Dialog::onReadyReadB()
{
    if (portB->bytesAvailable()) {
        QByteArray ba = portB->readAll();
        LogBin(ba, "comB.bin");
        bufB->append(ba);
        if (bufB->indexOf('\r') > 0) processB();
        if ((mode == 2) && (portA->isOpen())) portA->write(ba);
        if (ui->cbHexB->checkState() == Qt::Checked)
            LogMsg(ba.toHex().toUpper(), CH_IN);
        else
            LogMsg(ba, CH_IN);
    }
}

void Dialog::LogMsg(const QByteArray& s, int ch)
{
//    static int last_a_line, last_b_line;
//    static QTime last_a_time, last_b_time;
    QByteArray prefix;
    if (mode == 1) {
        prefix = (ch & CH_A ? "A" : "B");
        if (ch & CH_IN) prefix += "<=:";
        else prefix += "=>:";
    }
    else if (mode == 2)
        prefix = (ch & CH_A ? "A=>B:" : "B=>A:");
    else
        prefix = (ch & CH_IN ? "<=:" : "=>:");
    ui->recvEdit->moveCursor(QTextCursor::End);
    ui->recvEdit->insertPlainText(QString::fromLatin1(prefix + s + "\n"));
    QTime now(QTime::currentTime());
    QFile f("com.log");
    f.open(QIODevice::Append | QIODevice::Text);
    f.write(now.toString("hh:mm:ss.zzz").toLatin1() + " " + prefix + s + "\n");
    f.close();
}

void Dialog::LogBin(const QByteArray& s, const QString &fname)
{
    QFile f(fname);
    f.open(QIODevice::Append);
    f.write(s);
    f.close();
}

void Dialog::onContinuous(int s)
{
    if (s) timer->start(200);
    else timer->stop();
}
