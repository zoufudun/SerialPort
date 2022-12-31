#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
//#include <QtSerialPort/QtSerialPort>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QButtonGroup>
#include <QDateTime>
#include <QTime>
#include <QCheckBox>


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    QTimer *timer;
    QTimer *timerSend;
    QStringList lastPortStringList;
    QSerialPort *serialPort;
    QStringList serialDevice;
    QButtonGroup *RecGroupButton;
    QButtonGroup *TxGroupButton;
    QByteArray sendByteArry;
    QString SendTextEditStr;
    long RecvBytes;
    long TxBytes;
    bool isSerialOpen;

public:
    void setDelay(int ms,int pixelSize);  //设置滚动延迟,多少ms滚动多少像素点

    void setText2(QString text,QRgb textColor,float speed=0.70,int blankSuffix=20); //设置字体,调用该函数后,将会自动启动定时器来滚动字幕


private slots:
    void TimerEvent(void);
    void RecvData(void);            //接收数据
    void on_pushButtonOpen_clicked();
    void on_pushButtonClrRec_clicked();
    void on_pushButtonClrTx_clicked();
    void on_pushButtonClrCount_clicked();
    void on_pushButtonTx_clicked();
    void on_textEditRecv_textChanged();
    void on_textEditSend_textChanged();
    void on_radioButtonTxHex_clicked();
    void on_radioButtonTxASCII_clicked();
    void SerialSendData(QByteArray baData);
    void on_checkBoxRepeatTx_stateChanged(int arg1);
};
#endif // WIDGET_H
