#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QStringList>
#include <QString>
#include <QByteArray>

class Serial : public QSerialPort
{
    Q_OBJECT
public:
    explicit Serial();
    // 数据缓存区
    QByteArray dataBuf;
    // 串口描述内容
    QStringList serialDescription;
    // 串口扫描函数
    QStringList serialScan(void);
    bool serialOpen(QString serialName, int baudRate);
    void ReadData();
    void sendData(QByteArray sendData);
    void serialClose();
signals:
    void readSignal(void);
public slots:
};

#endif // SERIAL_H
