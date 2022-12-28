#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMessageBox>
#include <QButtonGroup>
#include <QAbstractItemView>
#include <QStatusBar>
#include <QMainWindow>
#include <QWidget>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    resize(650,550);
    setWindowTitle("SerialPort     https://github.com/zoufudun");

    timer = new QTimer(this);

    timer->start(500);

    connect(timer,&QTimer::timeout,this,&Widget::TimerEvent);

    ui->comboBoxBaudRate->setCurrentIndex(5);
    ui->comboBoxDataBits->setCurrentIndex(3);
    ui->comboBoxParity->setCurrentIndex(2);
    ui->comboBoxStopBits->setCurrentIndex(0);
    ui->comboBoxFlowCtr->setCurrentIndex(0);

    serialPort = new QSerialPort(this);

    connect(serialPort,SIGNAL(readyRead()),this,SLOT(RecvData()));

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

    RecvBytes = 0;
    TxBytes= 0;
    isSerialOpen = false;

    ui->label_status->setPixmap(QPixmap(":/Image/Image/OFF.png"));

    qint32 CustomBandrate = ui->comboBoxBaudRate->currentText().toUInt();
    serialPort->setBaudRate(CustomBandrate, QSerialPort::AllDirections);


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
            SendTextEditStr = sendByteArry.fromHex(sendByteArry);
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
    ui->comboBoxProtNum->view()->setFixedWidth(fontsize * maxlen * 0.75);//设置像素值


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
    if(ui->checkBoxStop->checkState() != Qt::Checked)
    {
        //qDebug() << "Recv Data";
        QByteArray recBuf;
        recBuf = serialPort->readAll();
        QString myStrTemp ;//= QString::fromLocal8Bit(recBuf);
        QString mtStrPrefix;
        if(!recBuf.isEmpty())
        {
            switch (RecGroupButton->checkedId())
            {
                case 0:
                    {
                        /*ascii显示*/
                        myStrTemp = QString::fromLocal8Bit(recBuf);
                        ui->textEditRecv->setTextColor(QColor(Qt::magenta));
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
                myStrTemp = QString("[%1] --> \r\nRX : %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")).arg(myStrTemp);
                ui->textEditRecv->append(myStrTemp);
            }
            else
            {
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
}


void Widget::on_pushButtonClrTx_clicked()
{
    ui->textEditSend->clear();
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




/*
void Widget::on_pushButtonTx_clicked()
{
    QString str = ui->textEditSend->toPlainText();

    switch (TxGroupButton->checkedId())
    {
    case 0:
        {
            QByteArray byteArray;
            if(ui->checkBoxTxNewLine->isChecked())
            {
               //byteArray =( ui->textEditSend->toPlainText() + '\r').toLocal8Bit();
               byteArray = (str + '\r').toLocal8Bit();
            }
            else
            {
                //byteArray = ui->textEditSend->toPlainText().toLocal8Bit();
                byteArray = (str).toLocal8Bit();
            }
            serialPort->write(byteArray);
            TxBytes += byteArray.length();
            ui->labelSendBytes->setText(QString::number(TxBytes));

        }

        break;
    case 1:
        {
            //QString str = ui->textEditSend->toPlainText();

            QByteArray text;
            if(ui->checkBoxTxNewLine->isChecked())
            {
               qDebug()<<"lyun";
               text = QByteArray::fromHex(( str + '\r').toLocal8Bit());////获取文本框的字符串，转换成字流
            }
            else
            {
               text = QByteArray::fromHex(str.toLocal8Bit());////获取文本框的字符串，转换成字流;
            }

            //QByteArray text=QByteArray::fromHex(str.toLatin1()); //获取文本框的字符串，转换成字流
            //qDebug()<<text.data();            // returns 字符串
            serialPort->write(text);
        }

        break;
    default:
        break;
    }
}
*/

void Widget::on_pushButtonTx_clicked()
{
    if (isSerialOpen != false)
    {
        /*将发送框数据发送*/
        SerialSendData(sendByteArry);
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
            /*是否加回车换行*/
            if (ui->checkBoxTxNewLine->isChecked())
            {
                baData.append("\r\n");
            }
            switch (TxGroupButton->checkedId())
            {
            case 1:// hex发送
            //if (ui->radioButtonTxHex->isChecked())  // hex发送
            {
                /*获取hex格式的数据*/
                baData = baData.fromHex(baData);
                /*发送hex数据*/
                serialPort->write(baData);
                /*是否显示时间戳*/
                if (ui->checkBoxShowTime->isChecked())
                {
                    QString strdata = baData.toHex(' ').trimmed().toUpper();
                    ui->textEditRecv->setTextColor(QColor("blue"));
                    ui->textEditRecv->append(QString("[%1]-->\r\nTX:").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
                    ui->textEditRecv->setTextColor(QColor("black"));
                    ui->textEditRecv->insertPlainText(strdata);
                }
            }
                break;
            case 0:     //ascii发送
            {
                /*发送ascii数据*/
                serialPort->write(baData);
                /*是否显示时间戳*/
                if (ui->checkBoxShowTime->isChecked())
                {
                    QString strdata = QString(baData);
                    ui->textEditRecv->setTextColor(QColor("red"));
                    ui->textEditRecv->append(QString("[%1]-->\r\nTX: ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz")));
                    ui->textEditRecv->setTextColor(QColor("black"));
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
            QMessageBox::warning(this, "警告", "数据为空");
        }
}
void Widget::on_textEditRecv_textChanged()
{
    // 将光标焦点移动至文末为了显示最新的内容
    QTextCursor cursor = ui->textEditRecv->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEditRecv->setTextCursor(cursor);
}

void Widget::on_textEditSend_textChanged()
{

        QString SendTextEditStr = ui->textEditSend->document()->toPlainText();
        if (SendTextEditStr.isEmpty())
        {
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
}



void Widget::on_radioButtonTxHex_clicked()
{
//    qDebug() << "狂插吕赟淫穴";
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
//    qDebug() << "狂插肖霞淫穴";
//    if (SendTextEditStr.isEmpty())
//    {
//        return;
//    }
//    //asccii与hex转换

//    //从QByteArray转换为QString
//    SendTextEditStr = sendByteArry.fromHex(sendByteArry);
//    ui->textEditSend->document()->setPlainText(SendTextEditStr);
}

