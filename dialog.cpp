#include "dialog.h"
#include "ui_dialog.h"
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QTime>
#include <QSettings>
#include <QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    portA(new QSerialPort()),
    portB(new QSerialPort()),
    timer(new QTimer())
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
    connect(timer, SIGNAL(timeout()), SLOT(onSendButtonClicked()));

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
}

void Dialog::onOpenCloseBClicked()
{
    OpenCloseCom(portB, ui->cbPortB, ui->cbBaudB, ui->ledB);
}

void Dialog::onSendButtonClicked()
{
    if (portA->isOpen() && !ui->sendEdit->toPlainText().isEmpty()) {
        QByteArray buf = ui->sendEdit->toPlainText().toLatin1();
   /*     for (int i = 0; i < buf.size(); ++i) {
            if (buf[i] == '$')
                port->write(QByteArray(1, '\001'));
            else if (buf[i] == '[')
                port->write(QByteArray(1, '\002')); // STX
            else if (buf[i] == ']')
                port->write(QByteArray(1, '\003')); // ETX
            else if (buf[i] == '}')
                port->write(QByteArray(1, 0x17)); // ETB
            else if (buf[i] == '^')
                port->write(QByteArray(1, 0x1B)); // ESC
            else
                port->write(QByteArray(1, buf[i]));
        } */
        if (ui->cbHexA->checkState() == Qt::Checked)
            portA->write(QByteArray::fromHex(buf));
        else
            portA->write(buf);
        LogMsg(buf, 1);
    }
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
}

void Dialog::onReadyReadA()
{
    if (portA->bytesAvailable()) {
        QByteArray ba = portA->readAll();
        LogBin(ba, "comA.bin");
        if ((mode == 2) && (portB->isOpen())) portB->write(ba);
        if (ui->cbHexA->checkState() == Qt::Checked)
            LogMsg(ba.toHex().toUpper(), 0);
        else
            LogMsg(ba, 0);
    }
}

void Dialog::onReadyReadB()
{
    if (portB->bytesAvailable()) {
        QByteArray ba = portB->readAll();
        LogBin(ba, "comB.bin");
        if ((mode == 2) && (portA->isOpen())) portA->write(ba);
        if (ui->cbHexB->checkState() == Qt::Checked)
            LogMsg(ba.toHex().toUpper(), 1);
        else
            LogMsg(ba, 1);
    }
}

void Dialog::LogMsg(const QByteArray& s, int ch)
{
    QByteArray prefix;
    if (mode > 0)
        prefix = (ch == 0 ? "A: " : "B: ");
    else {
        if (ch) prefix = "=>";
        else prefix = "<=";
    }
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
