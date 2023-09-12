#include "serial.h"
#include <QDebug>

Serial::Serial()
{

}

QStringList Serial::serialScan(void)
{
    QStringList serialPortList;
    serialDescription.clear(); // 每次扫描先清除串口描述信息
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        this->setPort(info);
        serialPortList.append(this->portName());
        serialDescription.append(info.description());
        //qDebug() << this->portName();
        //qDebug() << info.description();
    }
    return serialPortList;
}

bool Serial::serialOpen(QString serialName, int baudRate)
{
    this->setPortName(serialName);
    if(this->open(QIODeviceBase::ReadWrite)) {
        this->setBaudRate(baudRate);
        this->setDataBits(QSerialPort::Data8);
        this->setParity(QSerialPort::NoParity);
        this->setStopBits(QSerialPort::OneStop);
        this->setFlowControl(QSerialPort::NoFlowControl);
        // 下位机发送数据会响应这个槽函数
        connect(this, &QSerialPort::readyRead, this, &Serial::ReadData);
        // 下位机发送数据会触发这个信号
        connect(this, &QSerialPort::readyRead, this, &Serial::readSignal);
        return true;
    }
    return false;
}
// 读数据
void Serial::ReadData()
{
    dataBuf = this->readAll();
}
// 写数据
void Serial::sendData(QByteArray sendData)
{
    this->write(sendData);
}
// 关闭串口
void Serial::serialClose()
{
    this->clear();
    this->close();
}
