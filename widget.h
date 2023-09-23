#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QObject>
#include <QTimer>
//#include <QtSerialPort/QtSerialPort>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QButtonGroup>
#include <QDateTime>
#include <QTime>
#include <QCheckBox>
#include "SerialEvent.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    SerialEvent *mySerialEvent;

private:
    Ui::Widget *ui;
    QTimer *timer;
    QTimer *timerSend;
    QTimer *timerFileSend;
    QTimer *timerRecRead;
    QStringList lastPortStringList;
    QSerialPort *serialPort;
    QStringList serialDevice;
    QButtonGroup *RecGroupButton;
    QButtonGroup *TxGroupButton;
    QByteArray sendByteArry;
    QByteArray recBuffer;
    QString SendTextEditStr;
    QString fileText;
    bool isSendFile;
    int FrameCount;
    int FrameLen;
    int lastFrameLen;
    int FrameNumber;
    int FrameGap;
    int ProgressBarValue;
    int ProgressBarStep;
    long RecvBytes;
    long TxBytes;
    bool isSerialOpen;
    bool sendTextChangedFlag;



public:
    void setDelay(int ms,int pixelSize);  //设置滚动延迟,多少ms滚动多少像素点

    void setText2(QString text,QRgb textColor,float speed=0.70,int blankSuffix=20); //设置字体,调用该函数后,将会自动启动定时器来滚动字幕
    void SendFile();



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
    void on_pushButtonOpenFile_clicked();
    void on_pushButtonSaveFile_clicked();
    void on_pushButtonSendFile_clicked();
    void File_TimerSend(void);

public slots:
//    void showEvent(QShowEvent* e) override;
//    void closeEvent(QCloseEvent* e) override;
      bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;
      QStringList SerialScan(void);
};
#endif // WIDGET_H
