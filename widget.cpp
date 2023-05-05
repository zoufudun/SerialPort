#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMessageBox>
#include <QButtonGroup>
#include <QAbstractItemView>
#include <QStatusBar>
#include <QMainWindow>
#include <QWidget>
#include <QTextCodec>
#include <QPainter>
#include <QResizeEvent>
#include <QFileDialog>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QMimeData>
#include <QMimeDatabase>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    resize(650,700);
    setWindowTitle("SerialPort V1.0    https://github.com/zoufudun");

    RecvBytes = 0;
    TxBytes= 0;
    isSerialOpen = false;
    sendTextChangedFlag = false;

    isSendFile = false;
    FrameCount = 0;
    FrameLen = 0;
    lastFrameLen = 0;
    FrameNumber = 0;
    FrameGap = 0;
    ProgressBarValue = 0;

    timer = new QTimer(this);
    timerRecRead = new QTimer(this);

    timer->start(500);

    connect(timer,&QTimer::timeout,this,&Widget::TimerEvent);

    timerFileSend = new QTimer(this);
    connect(timerFileSend,SIGNAL(timeout()),this,SLOT(File_TimerSend()));

    /*定时发送定时器*/
    timerSend = new QTimer(this);
    /*定时器超时信号槽*/
    connect(timerSend, &QTimer::timeout, this, [&](){
        SerialSendData(sendByteArry);
    });

    ui->spinBoxTime->setMinimum(0);
    ui->spinBoxTime->setValue(1000);
    ui->spinBoxTime->setSingleStep(10);

    ui->comboBoxBaudRate->setCurrentIndex(5);
    ui->comboBoxDataBits->setCurrentIndex(3);
    ui->comboBoxParity->setCurrentIndex(2);
    ui->comboBoxStopBits->setCurrentIndex(0);
    ui->comboBoxFlowCtr->setCurrentIndex(0);

    serialPort = new QSerialPort(this);

    //connect(serialPort,SIGNAL(readyRead()),this,SLOT(RecvData()));
    connect(serialPort,&QSerialPort::readyRead,this,[&]()
            {
                timerRecRead->start(100);
                recBuffer.append(serialPort->readAll());
            });
    connect(timerRecRead,&QTimer::timeout,this,&Widget::RecvData);


    //选中发送的历史数据的信号槽：将选中的发送历史数据填入到发送文本框中。
    connect(ui->comboBoxSendData,&QComboBox::textActivated,this,[&](const QString &text){
        ui->textEditSend->clear();
        ui->textEditSend->setText(text);
    });

    //串口状态
    QString status = "欢迎使用串口调试助手";
    ui->labelSerialSta->setText(status);
    ui->labelSerialSta->setStyleSheet("color:blue");

    RecGroupButton=new QButtonGroup(this);
    RecGroupButton->addButton(ui->radioButtonRecASCII,0);
    RecGroupButton->addButton(ui->radioButtonRecHex,1);
    ui->radioButtonRecASCII->setChecked(true);

    TxGroupButton=new QButtonGroup(this);
    TxGroupButton->addButton(ui->radioButtonTxASCII,0);
    TxGroupButton->addButton(ui->radioButtonTxHex,1);
    ui->radioButtonTxASCII->setChecked(true);

    //ui->textEditSend->setText("欢迎使用SerialPortPHD");

    ui->checkBoxTxNewLine->setChecked(true);

    ui->label_status->setPixmap(QPixmap(":/Image/Image/OFF.png"));

    qint32 CustomBandrate = ui->comboBoxBaudRate->currentText().toUInt();
    serialPort->setBaudRate(CustomBandrate, QSerialPort::AllDirections);

    /*波特率支持自定义*/
    connect(ui->comboBoxBaudRate, QOverload<int>::of(&QComboBox::currentIndexChanged),[&](int index){
        if (index == 6)
        {
            ui->comboBoxBaudRate->setEditable(true);
            ui->comboBoxBaudRate->setCurrentText(NULL);
        } else
        {
            ui->comboBoxBaudRate->setEditable(false);
        }
    });

    /*发送文本框信号槽：发送文本框内容发生变化的信号槽*/
    connect(ui->textEditSend, &QTextEdit::textChanged, this, [=](){
        sendTextChangedFlag = true;
        //获取发送框字符
        SendTextEditStr = ui->textEditSend->document()->toPlainText();
        if (SendTextEditStr.isEmpty())
        {
            //timerSend->stop();
            //ui->spinBoxTime->setEnabled(true);
            sendByteArry.clear();
            return;
        }
        //勾选hex发送则判断是否有非法hex字符
        if (ui->radioButtonTxHex->isChecked())
        {
            char ch;
            bool flag = false;
            uint32_t i, len;
            //去掉无用符号
            SendTextEditStr = SendTextEditStr.replace(' ',"");
            SendTextEditStr = SendTextEditStr.replace(',',"");
            SendTextEditStr = SendTextEditStr.replace('\r',"");
            SendTextEditStr = SendTextEditStr.replace('\n',"");
            SendTextEditStr = SendTextEditStr.replace('\t',"");
            SendTextEditStr = SendTextEditStr.replace("0x","");
            SendTextEditStr = SendTextEditStr.replace("0X","");
            //判断数据合法性
            for(i = 0, len = SendTextEditStr.length(); i < len; i++)
            {
                ch = SendTextEditStr.at(i).toLatin1();
                if (ch >= '0' && ch <= '9')
                {
                    flag = false;
                }
                else if (ch >= 'a' && ch <= 'f')
                {
                    flag = false;
                }
                else if (ch >= 'A' && ch <= 'F')
                {
                    flag = false;
                }
                else
                {
                    flag = true;
                }
            }
            if(flag)
            {
                QMessageBox::warning(this,"警告","输入内容包含非法16进制字符");
            }
        }
        //QString转QByteArray
        //sendByteArry = SendTextEditStr.toUtf8();
        sendByteArry = SendTextEditStr.toLocal8Bit();
    });

    /*HEX发送chexkBox信号槽*/
    connect(ui->radioButtonTxHex,QOverload<bool>::of(&QRadioButton::toggled),this,[=](bool checked){

        //asccii与hex转换
        if (checked == true)
        {
            //转换成QByteArray -> 转换成16进制数，按空格分开 -> 转换为大写
            sendByteArry = sendByteArry.toHex(' ').toUpper();
            ui->textEditSend->document()->setPlainText(sendByteArry);
        }
        else
        {
            //从QByteArray转换为QString
            //SendTextEditStr = sendByteArry.fromHex(sendByteArry);
            sendByteArry = sendByteArry.fromHex(sendByteArry);
            SendTextEditStr = QString::fromLocal8Bit(sendByteArry);
            //SendTextEditStr = SendTextEditStr.toLatin1();
            ui->textEditSend->document()->setPlainText(SendTextEditStr);
        }

    });




//    connect(ui->radioButtonTxASCII,QOverload<bool>::of(&QRadioButton::toggled),this,[=](bool checked){

//        //asccii与hex转换
//        if (checked == true)
//        {
//            //从QByteArray转换为QString
//            SendTextEditStr = sendByteArry.fromHex(sendByteArry);
//            ui->textEditSend->document()->setPlainText(SendTextEditStr);
//        }
//        else
//        {
//            //转换成QByteArray -> 转换成16进制数，按空格分开 -> 转换为大写
//            sendByteArry = sendByteArry.toHex(' ').toUpper();
//            ui->textEditSend->document()->setPlainText(sendByteArry);
//        }
//    });



    // 样式表
    this->setStyleSheet(//正常状态样式
                        "#pushButtonOpen,#pushButtonTx{"
                        "background-color:rgb(255,255,255);" // 背景色（也可以设置图片）
                        "border-style:outset;" // 边框样式（inset/outset）
                        "border-radius:5px;"
                        "border-color:rgba(71,75,76,100);" // 边框颜色
                        "font: 12px;" // 字体，字体大小
                        //"font-weight:bold;" // 字体粗细
                        //"color:rgba(255, 103, 76,255);" // 字体颜色
                        "padding:1px;" // 填衬
                        "}"
                        //鼠标悬停样式
                        "#pushButtonOpen:hover,#pushButtonTx:hover{"
                        //"background-color:rgba(255, 103, 76,255);"
                        "border-color:rgba(255,255,255,200);"
                        "font:bold 14px;"
                       //"color:rgba(255,255,255,255);"
                        "}"

                        // 设置背景色 半透明
                        "#textEditRecv,#textEditSend{"
                        "background-color:rgba(255,255,255,0.7);"
                        //边框粗细-颜色-圆角设置
                        "border-radius:10px;"
                        "}"
                        );

}

Widget::~Widget()
{
    delete ui;
}


void Widget::TimerEvent(void)
{
    QStringList newPortStringList;  
    int maxlen = 0;/* 获取最长字符串 */
    newPortStringList.clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
         newPortStringList <<  info.portName() + ':' + info.description();
         serialDevice << info.portName();
    }
    if(newPortStringList.size() != lastPortStringList.size())
    {
        lastPortStringList = newPortStringList;
        ui->comboBoxProtNum->clear();
        ui->comboBoxProtNum->addItems(newPortStringList);
    }

    for (int index = 0; index < ui->comboBoxProtNum->count(); index++)
    {
        if (ui->comboBoxProtNum->itemText(index).length() > maxlen)
        {
            maxlen = ui->comboBoxProtNum->itemText(index).length();
        }
    }

    /*获取字体磅值转换为像素值*/
    int fontsize = ui->comboBoxProtNum->font().pointSize();//获取字体的磅值
    ui->comboBoxProtNum->view()->setFixedWidth(fontsize * maxlen * 1.75);//设置像素值


}




void Widget::on_pushButtonOpen_clicked()
{
    if(ui->pushButtonOpen->text() == QString("打开串口"))
    {
        QString dev = serialDevice.at(ui->comboBoxProtNum->currentIndex());
        serialPort->setPortName(dev);
        //serialPort->setPortName(ui->comboBoxProtNum->currentText());
        serialPort->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
        switch (ui->comboBoxDataBits->currentText().toInt())
        {
            case 5:
                serialPort->setDataBits(QSerialPort::Data5);
                break;
            case 6:
                serialPort->setDataBits(QSerialPort::Data6);
                break;
            case 7:
                serialPort->setDataBits(QSerialPort::Data7);
                break;
            case 8:
                serialPort->setDataBits(QSerialPort::Data8);
                break;
            default:
                break;
        }

        switch (ui->comboBoxParity->currentIndex())
        {
            case 0:
                serialPort->setParity(QSerialPort::EvenParity);
                break;
            case 1:
                serialPort->setParity(QSerialPort::OddParity);
                break;
            case 2:
                serialPort->setParity(QSerialPort::NoParity);
                break;
            default:
                break;
        }

        switch (ui->comboBoxStopBits->currentIndex())
        {
            case 0:
                serialPort->setStopBits(QSerialPort::OneStop);
                break;
            case 1:
                serialPort->setStopBits(QSerialPort::OneAndHalfStop);
                break;
            case 2:
                serialPort->setStopBits(QSerialPort::TwoStop);
                break;
            default:
                break;
        }

        switch (ui->comboBoxFlowCtr->currentIndex())
        {
            case 0:
                serialPort->setFlowControl(QSerialPort::NoFlowControl);
                break;
            case 1:
                serialPort->setFlowControl(QSerialPort::HardwareControl);
                break;
            case 2:
                serialPort->setFlowControl(QSerialPort::SoftwareControl);
                break;
            default:
                break;
        }

        if(!serialPort->open(QIODeviceBase::ReadWrite))
        {
           QMessageBox::information(this,"串口打开提示","串口打开失败",QMessageBox::Ok);
           ui->labelSerialSta->setText("串口打开失败！！！");
           QString sm = "串口[%1] 打开失败！！！";
           QString status = sm.arg(serialPort->portName());
           ui->labelSerialSta->setText(status);
           ui->labelSerialSta->setStyleSheet("color:yellow");
           return;
        }
        else
        {
           isSerialOpen = true;
           //ui->labelSerialSta->setText("串口打开成功！！！");
           ui->label_status->setPixmap(QPixmap(":/Image/Image/ON.png"));
           //ui->label_status->setProperty("isOn",true);
           //ui->label_status->style()->polish(ui->label_status);
           QString sm = "串口[%1] OPENED, %2, %3, %4, %5";
           QString status = sm.arg(serialPort->portName()).arg(serialPort->baudRate()).arg(serialPort->dataBits()).arg(ui->comboBoxParity->currentText()).arg(ui->comboBoxStopBits->currentText());
           ui->labelSerialSta->setText(status);
           ui->labelSerialSta->setStyleSheet("color:green");
        }


        //当打开串口后，检测到自动发送已经使能后，开启自动发送
        if(ui->checkBoxRepeatTx->isChecked())
        {
            /*获取设定时间*/
            int time = ui->spinBoxTime->text().toInt();
            if (time > 0)
            {
                timerSend->start(time);
                ui->spinBoxTime->setEnabled(false);
            }
            else
            {
                QMessageBox::warning(this, "警告", "时间必须大于0");
                ui->spinBoxTime->setEnabled(true);
                ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
            }
        }
        else
        {
            /*停止发送*/
            timerSend->stop();
            ui->spinBoxTime->setEnabled(true);
            //ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
        }


        ui->comboBoxProtNum->setEnabled(false);
        ui->comboBoxBaudRate->setEnabled(false);
        ui->comboBoxDataBits->setEnabled(false);
        ui->comboBoxParity->setEnabled(false);
        ui->comboBoxStopBits->setEnabled(false);
        ui->comboBoxFlowCtr->setEnabled(false);

        ui->pushButtonOpen->setText("关闭串口");

    }
    else
    {
        isSerialOpen = false;
        serialPort->close();

        //当关闭串口后，检测到自动发送已经使能后，停止自动发送
        /*停止定时发送*/
        timerSend->stop();
        ui->spinBoxTime->setEnabled(true);

        //serialPort->deleteLater();//删除串口对象
        ui->comboBoxProtNum->setEnabled(true);
        ui->comboBoxBaudRate->setEnabled(true);
        ui->comboBoxDataBits->setEnabled(true);
        ui->comboBoxParity->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->comboBoxFlowCtr->setEnabled(true);
        ui->pushButtonOpen->setText("打开串口");
        //ui->labelSerialSta->setText("串口已关闭！！！");
        ui->label_status->setPixmap(QPixmap(":/Image/Image/OFF.png"));
        //ui->label_status->setProperty("isOn",false);
        //ui->label_status->style()->polish(ui->label_status);
        QString sm = "串口[%1] CLOSED";
        QString status = sm.arg(serialPort->portName());
        ui->labelSerialSta->setText(status);
        ui->labelSerialSta->setStyleSheet("color:red");
    }
}

/*
void Widget::RecvData(void)
{
    qDebug() << "Recv Data";
    if(ui->checkBoxStop->checkState() != Qt::Checked)
    {
        //qDebug() << "Recv Data";
        QByteArray recBuf;
        recBuf = serialPort->readAll();
        QString myStrTemp = QString::fromLocal8Bit(recBuf);
        RecvBytes += myStrTemp.length();
        ui->labelRecvBytes->setText(QString::number(RecvBytes));
        if(!recBuf.isEmpty())
        {
            switch (RecGroupButton->checkedId())
            {
                case 0:
                    {
                        ui->checkBoxHexSpace->setChecked(false);
                        //如果换行被勾选将会在每次接收显示前换行,另外判断是不是为空，确保第一行不会先给显示个回车符
                        if((ui->checkBoxWrap->isChecked())&&(!ui->textEditRecv->toPlainText().isEmpty()))
                        {
                            ui->textEditRecv->insertPlainText("\n");
                        }

                        ui->textEditRecv->insertPlainText(myStrTemp);
                    }
                    break;
                case 1:
                {
                     QDataStream out(&recBuf,QIODevice::ReadWrite);   //将str的数据 读到out里面去
                     //QString buf;

                     if(ui->checkBoxWrap->isChecked())
                     {
                         ui->textEditRecv->insertPlainText("\n");
                     }
                     while(!out.atEnd())
                     {
                         qint8 outChar = 0;
                         out >> outChar;   //每次一个字节的填充到 outchar
                         QString str =QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper();// + QString(" ");   //2 字符宽度
                         if(ui->checkBoxHexSpace->isChecked())
                         {
                             qDebug() << "Recv Data hex space";
                             ui->textEditRecv->insertPlainText(str+' ');
                         }
                         else
                         {
                             qDebug() << "Recv Data hex";
                             ui->textEditRecv->insertPlainText(str);
                         }
                         //buf += str;
                     }
                }
                    break;
                default:
                    break;
            }
        }
    }
}
*/

void Widget::RecvData(void)
{
    qDebug() << "Recv Data";
    timerRecRead->stop();
    if(ui->checkBoxStop->checkState() != Qt::Checked)
    {
        //qDebug() << "Recv Data";
//        QByteArray recBuf;
//        recBuf = serialPort->readAll();
        QByteArray recBuf = recBuffer;
        recBuffer.clear();
        QString myStrTemp ;//= QString::fromLocal8Bit(recBuf);
        if(!recBuf.isEmpty())
        {
            switch (RecGroupButton->checkedId())
            {
                case 0:
                    {
                        /*ascii显示*/
                        myStrTemp = QString::fromLocal8Bit(recBuf);
                        ui->textEditRecv->setTextColor(QColor(Qt::red));
                        ui->checkBoxHexSpace->setChecked(false);
                        //如果换行被勾选将会在每次接收显示前换行,另外判断是不是为空，确保第一行不会先给显示个回车符
                        if((ui->checkBoxWrap->isChecked())&&(!ui->textEditRecv->toPlainText().isEmpty()))
                        {
                            ui->textEditRecv->insertPlainText("\n");
                        }

                    }
                    break;
                case 1:
                    {
                        /*hex显示*/
                        if(ui->checkBoxHexSpace->isChecked())
                        {
                            myStrTemp = recBuf.toHex(' ').trimmed().toUpper();
                            ui->textEditRecv->setTextColor(QColor(Qt::green));
                        }
                        else
                        {
                            myStrTemp = recBuf.toHex(0).trimmed().toUpper();
                            ui->textEditRecv->setTextColor(QColor(Qt::blue));
                        }
                        if(ui->checkBoxWrap->isChecked())
                        {
                            ui->textEditRecv->insertPlainText("\n");
                        }
                    }
                    break;
                default:
                    break;
            }

            /*是否显示时间戳*/
            if (ui->checkBoxShowTime->isChecked())
            {
                //myStrTemp = QString("[%1] --> \r\nRX : %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")).arg(myStrTemp);
                //ui->textEditRecv->append(myStrTemp);
                ui->textEditRecv->append( QString("[%1] --> \r\nRX : ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
                ui->textEditRecv->setTextColor(QColor(Qt::black));
                ui->textEditRecv->insertPlainText(myStrTemp);
            }
            else
            {
                ui->textEditRecv->setTextColor(QColor(Qt::darkRed));
                ui->textEditRecv->insertPlainText(myStrTemp);
            }

            RecvBytes += myStrTemp.length();
            ui->labelRecvBytes->setText(QString::number(RecvBytes));
        }
    }
}

void Widget::on_pushButtonClrRec_clicked()
{
    ui->textEditRecv->clear();
    ui->labelRecvBytes->clear();
    RecvBytes = 0;
    ui->labelRecvBytes->setText(QString::number(RecvBytes));
}


void Widget::on_pushButtonClrTx_clicked()
{
    ui->textEditSend->clear();
    ui->labelSendBytes->clear();
    TxBytes = 0;
    ui->labelSendBytes->setText(QString::number(TxBytes));
}


void Widget::on_pushButtonClrCount_clicked()
{
    ui->labelRecvBytes->clear();
    RecvBytes = 0;
    ui->labelRecvBytes->setText(QString::number(RecvBytes));

    ui->labelSendBytes->clear();
    TxBytes = 0;
    ui->labelSendBytes->setText(QString::number(TxBytes));
}





//void Widget::on_pushButtonTx_clicked()
//{
//    QString str = ui->textEditSend->toPlainText();

//    switch (TxGroupButton->checkedId())
//    {
//    case 0:
//        {
//            QByteArray byteArray;
//            if(ui->checkBoxTxNewLine->isChecked())
//            {
//               //byteArray =( ui->textEditSend->toPlainText() + '\r').toLocal8Bit();
//               byteArray = (str + '\r').toLocal8Bit();
//            }
//            else
//            {
//                //byteArray = ui->textEditSend->toPlainText().toLocal8Bit();
//                byteArray = (str).toLocal8Bit();
//            }
//            serialPort->write(byteArray);
//            TxBytes += byteArray.length();
//            ui->labelSendBytes->setText(QString::number(TxBytes));

//        }

//        break;
//    case 1:
//        {
//            //QString str = ui->textEditSend->toPlainText();

//            QByteArray text;
//            if(ui->checkBoxTxNewLine->isChecked())
//            {
//               qDebug()<<"lyun";
//               text = QByteArray::fromHex(( str + '\r').toLocal8Bit());////获取文本框的字符串，转换成字流
//            }
//            else
//            {
//               text = QByteArray::fromHex(str.toLocal8Bit());////获取文本框的字符串，转换成字流;
//            }

//            //QByteArray text=QByteArray::fromHex(str.toLatin1()); //获取文本框的字符串，转换成字流
//            //qDebug()<<text.data();            // returns 字符串
//            serialPort->write(text);
//        }

//        break;
//    default:
//        break;
//    }
//}


void Widget::on_pushButtonTx_clicked()
{
    if (isSerialOpen != false)
    {
        /*将发送框数据发送*/
        SerialSendData(sendByteArry);
        if(sendTextChangedFlag == true)
        {
            ui->comboBoxSendData->addItem(SendTextEditStr);
            sendTextChangedFlag = false;
        }
    }
    else
    {
        QMessageBox::information(this, "提示", "串口未打开");
    }
}

void Widget::SerialSendData(QByteArray baData)
{
    if (baData.isEmpty() != true)
    {

            switch (TxGroupButton->checkedId())
            {
            case 1:// hex发送
            {

                /*获取hex格式的数据*/
                baData = baData.fromHex(baData);
                /*是否加回车换行*/
                if (ui->checkBoxTxNewLine->isChecked())
                {
                    baData.append("\r\n");
                }
                /*发送hex数据*/
                serialPort->write(baData);
                /*是否显示时间戳*/
                if (ui->checkBoxShowTxTime->isChecked())
                {
                    QString strdata = baData.toHex(' ').trimmed().toUpper();
                    ui->textEditRecv->setTextColor(QColor("blue"));
                    ui->textEditRecv->append(QString("[%1]-->\r\nTX:").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
                    ui->textEditRecv->setTextColor(QColor(Qt::darkGreen));
                    ui->textEditRecv->insertPlainText(strdata);
                }
            }
                break;
            case 0://ascii发送
            {
                /*是否加回车换行*/
                if (ui->checkBoxTxNewLine->isChecked())
                {
                    baData.append("\r\n");
                }
                /*发送ascii数据*/
                serialPort->write(baData);
                /*是否显示时间戳*/
                if (ui->checkBoxShowTxTime->isChecked())
                {
                    QString strdata = QString::fromLocal8Bit(baData);
                    //QString strdata = QString(baData);
                    ui->textEditRecv->setTextColor(QColor("red"));
                    ui->textEditRecv->append(QString("[%1]-->\r\nTX: ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
                    ui->textEditRecv->setTextColor(QColor(Qt::green));
                    ui->textEditRecv->insertPlainText(strdata);
                }
            }
                break;
            default:break;
            }
            //移动光标到末尾
            ui->textEditRecv->moveCursor(QTextCursor::End);
            //更新发送计数
            TxBytes += baData.length();
            ui->labelSendBytes->setText(QString::number(TxBytes));
    }
    else
    {
        if (ui->checkBoxTxNewLine->isChecked())
        {
            baData.append("\r\n");
        }
        serialPort->write(baData);

        /*是否显示时间戳*/
        if (ui->checkBoxShowTxTime->isChecked())
        {
            QString strdata = QString::fromLocal8Bit(baData);
            //QString strdata = QString(baData);
            ui->textEditRecv->setTextColor(QColor("red"));
            ui->textEditRecv->append(QString("[%1]-->\r\nTX: ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
            ui->textEditRecv->setTextColor(QColor(Qt::green));
            ui->textEditRecv->insertPlainText(strdata);
        }
        //QMessageBox::warning(this, "警告", "数据为空");
    }
}

void Widget::on_textEditRecv_textChanged()
{
    // 将光标焦点移动至文末为了显示最新的内容
//    QTextCursor cursor = ui->textEditRecv->textCursor();
//    cursor.movePosition(QTextCursor::End);
//    ui->textEditRecv->setTextCursor(cursor);
    ui->textEditRecv->moveCursor(QTextCursor::End);
}

void Widget::on_textEditSend_textChanged()
{
//        QString SendTextEditStr = ui->textEditSend->document()->toPlainText();
//        if (SendTextEditStr.isEmpty())
//        {
//            return;
//        }
//        //勾选hex发送则判断是否有非法hex字符
//        if (ui->radioButtonTxHex->isChecked())
//        {
//            char ch;
//            bool flag = false;
//            uint32_t i, len;
//            //去掉无用符号
//            SendTextEditStr = SendTextEditStr.replace(' ',"");
//            SendTextEditStr = SendTextEditStr.replace(',',"");
//            SendTextEditStr = SendTextEditStr.replace('\r',"");
//            SendTextEditStr = SendTextEditStr.replace('\n',"");
//            SendTextEditStr = SendTextEditStr.replace('\t',"");
//            SendTextEditStr = SendTextEditStr.replace("0x","");
//            SendTextEditStr = SendTextEditStr.replace("0X","");
//            //判断数据合法性
//            for(i = 0, len = SendTextEditStr.length(); i < len; i++)
//            {
//                ch = SendTextEditStr.at(i).toLatin1();
//                if (ch >= '0' && ch <= '9')
//                {
//                    flag = false;
//                }
//                else if (ch >= 'a' && ch <= 'f')
//                {
//                    flag = false;
//                }
//                else if (ch >= 'A' && ch <= 'F')
//                {
//                    flag = false;
//                }
//                else
//                {
//                    flag = true;
//                }
//            }
//            if(flag)
//            {
//                QMessageBox::warning(this,"警告","输入内容包含非法16进制字符");
//            }
//        }
//        //QString转QByteArray
//        //sendByteArry = SendTextEditStr.toUtf8();
//        sendByteArry = SendTextEditStr.toLocal8Bit();
}



void Widget::on_radioButtonTxHex_clicked()
{
//    if (SendTextEditStr.isEmpty())
//    {
//        return;
//    }
//    //asccii与hex转换


//    //转换成QByteArray -> 转换成16进制数，按空格分开 -> 转换为大写
//    sendByteArry = sendByteArry.toHex(' ').toUpper();
//    ui->textEditSend->document()->setPlainText(sendByteArry);
}


void Widget::on_radioButtonTxASCII_clicked()
{
//    if (SendTextEditStr.isEmpty())
//    {
//        return;
//    }
//    //asccii与hex转换

//    //从QByteArray转换为QString
//    SendTextEditStr = sendByteArry.fromHex(sendByteArry);
//    ui->textEditSend->document()->setPlainText(SendTextEditStr);
}


void Widget::on_checkBoxRepeatTx_stateChanged(int arg1)
{
    int time;
    /*判断串口是否打开*/
    if (false == isSerialOpen)
    {
        if (ui->checkBoxRepeatTx->isChecked())
        {
            QMessageBox::information(this, "提示", "串口未打开");
            ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
        }
        return;
    }
//    /*判断是否有数据*/
//    if (ui->textEditSend->document()->isEmpty() == true)
//    {
//        if (ui->checkBoxRepeatTx->isChecked())
//        {
//            QMessageBox::warning(this, "警告", "数据为空");
//            ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
//        }
//        return;
//    }

    /*判断勾选状态*/
    if (arg1 == Qt::Checked)
    {
        /*获取设定时间*/
        time = ui->spinBoxTime->text().toInt();
        if (time > 0)
        {
            timerSend->start(time);
            ui->spinBoxTime->setEnabled(false);
        }
        else
        {
            QMessageBox::warning(this, "警告", "时间必须大于0");
            ui->spinBoxTime->setEnabled(true);
            ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
        }
    }
    else
    {
        /*停止发送*/
        timerSend->stop();
        ui->spinBoxTime->setEnabled(true);
        //ui->checkBoxRepeatTx->setCheckState(Qt::Unchecked);
    }
}



void Widget::on_pushButtonOpenFile_clicked()
{
    QString curPath = QDir::currentPath();
    QString filter = "文本文件(*.txt);;二进制文件(*.bin *.dat);;所有文件(*.*)";
    QString filePath = QFileDialog::getOpenFileName(this,"打开文件",curPath,filter);
    QFileInfo fileinfo(filePath);
    ui->lineEditFilePath->clear();
    ui->lineEditFilePath->setText(filePath);

    QFile file(filePath);
    if (!file.exists())
    {
        QMessageBox::warning(this,"警告","文件不存在");
        return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"警告","文件打开失败");
        return;
    }

    /*判断文件类型*/
    int type = 0;
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath);
    if (mime.name().startsWith("text/"))
    {
        type = 1;	//文本文件
    }
    else if (mime.name().startsWith("application/"))
    {
        type = 2;	//二进制文件
    }

    /*读取文件*/
    switch(type)
    {
        case 1:
        {
            //QIODevice读取普通文本
            QByteArray data = file.readAll();
            fileText =  data;
            file.close();
            if (data.isEmpty())
            {
                QMessageBox::information(this, "提示", "文件内容空");
                return;
            }
        }
        break;
        case 2:
        {
            int filelen = fileinfo.size();
            QVector<char> cBuf(filelen);//储存读出数据
            //使用QDataStream读取二进制文件
            QDataStream datain(&file);
            datain.readRawData(&cBuf[0],filelen);
            file.close();
            //char数组转QString
            fileText = QString::fromLocal8Bit(&cBuf[0],filelen);
        }
        break;
    }

    //显示文件大小信息
    QString info = QString("%1").arg(fileText.length());
    ui->lineEditFileSize->clear();
    ui->lineEditFileSize->setText(info);
    //显示文件内容
    if (ui->radioButtonTxHex->isChecked())
    {
        ui->textEditSend->setPlainText(fileText.toLocal8Bit().toHex(' ').toUpper());
    }
    else
    {
        ui->textEditSend->insertPlainText(fileText);
    }
    //设置显示焦点在最顶部
    ui->textEditRecv->moveCursor(QTextCursor::Start,QTextCursor::MoveAnchor);
    /*标记有文件发送*/
    isSendFile = true;
    FrameCount = 0;
    ProgressBarValue = 0;

//    QByteArray fileByteArray = file.readAll();
//    ui->textEditSend->clear();
//    ui->textEditSend->append(fileByteArray);
//    file.close();

}


void Widget::on_pushButtonSaveFile_clicked()
{
    QString dataRx = ui->textEditRecv->toPlainText();
    if(dataRx.isEmpty())
    {
        QMessageBox::information(this,"提示","数据内容为空");
        return;
    }

    QString curPath = QDir::currentPath();
    QString filterFile = "文本文件(*.txt);;二进制文件(*.bin *.dat);;所有文件(*.*)";

    QString saveFileName = QFileDialog::getSaveFileName(this,"保存文件",curPath,filterFile);
    if(saveFileName.isEmpty())
    {
        return;
    }

    QFile file(saveFileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        return;
    }

    QTextStream textStream(&file);
    textStream << dataRx;
    file.close();

}


void Widget::on_pushButtonSendFile_clicked()
{
    if (isSerialOpen != false)
    {
        if (isSendFile)	//发送文件数据
        {
            if (ui->pushButtonSendFile->text() == "发送文件")
            {
                ui->pushButtonSendFile->setText("停止发送");
                SendFile();
            }
            else
            {
                ui->pushButtonSendFile->setText("发送文件");
                timerFileSend->stop();
            }
        }
        else	//发送发送框数据
        {
            SerialSendData(sendByteArry);
        }
    }
    else
    {
        QMessageBox::information(this, "提示", "串口未打开");
    }
}

void Widget::SendFile()
{
    /*按设置参数发送*/
      FrameLen = ui->lineEditFrameLen->text().toInt();  // 帧大小
      FrameGap = ui->lineEditFrameGap->text().toInt();      // 帧间隔
      int textlen = Widget::fileText.size();                // 文件大小
      if (FrameGap <= 0 || textlen <= FrameLen)
      {
          //时间间隔为0 或 帧大小≥文件大小 则直接一次发送
          serialPort->write(fileText.toLocal8Bit());
          //ui->pushButtonSendFile->setText("发送文件");
          QMessageBox::information(this, "提示", "文件发送完成");

      }
      else
      {
          //按设定时间和长度发送
          FrameNumber = textlen / FrameLen; // 包数量
          lastFrameLen = textlen % FrameLen; // 最后一包数据长度
          //进度条步进值
          if (FrameNumber >= 100)
          {
              ProgressBarStep = FrameNumber / 100;
          }
          else
          {
              ProgressBarStep = 100 / FrameNumber;
          }
          //设置定时器
          timerFileSend->start(FrameGap);
      }
}

/*
    函   数：File_TimerSend
    描   述：发送文件定时器槽函数
    输   入：无
    输   出：无
*/
void Widget::File_TimerSend(void)
{
    if (FrameCount < FrameNumber)
    {
        serialPort->write(fileText.mid(FrameCount * FrameLen, FrameLen).toLocal8Bit());
        FrameCount++;
        //更新进度条
        ui->progressBar->setValue(ProgressBarValue += ProgressBarStep);
    }
    else
    {
        if (lastFrameLen > 0)
        {
            serialPort->write(fileText.mid(FrameCount * FrameLen, lastFrameLen).toLocal8Bit());
            ui->progressBar->setValue(100);
        }
        /*发送完毕*/
        timerFileSend->stop();
        FrameCount = 0;
        ProgressBarValue = 0;
        FrameNumber = 0;
        lastFrameLen = 0;
        QMessageBox::information(this, "提示", "发送完成");
        ui->progressBar->setValue(0);
        ui->pushButtonSendFile->setText("发送文件");
    }
}
