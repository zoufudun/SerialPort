#ifndef SERIALEVENT_H
#define SERIALEVENT_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <windows.h>
#include <dbt.h>

class SerialEvent : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
    {
        (void)result;
        if(eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
        {
            MSG* msg = reinterpret_cast<MSG*>(message);
            int msgType = msg->message;
            if(msgType == WM_DEVICECHANGE)
            {
                PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
                switch (msg->wParam)
                {
                    case DBT_DEVICEARRIVAL:
                        qDebug() << "DBT_DEVICEARRIVAL:" << lpdb->dbch_devicetype;
                        if(lpdb->dbch_devicetype == DBT_DEVTYP_PORT)
                        {
                            PDEV_BROADCAST_PORT lpdbv = (PDEV_BROADCAST_PORT)lpdb;
                            QString portName = QString::fromWCharArray(lpdbv->dbcp_name);
                            qDebug() << "device arrival:" << portName;

                            emit comDevArriaval(portName);
                        }
                        break;
                    case DBT_DEVICEREMOVECOMPLETE:
                        qDebug() << "DBT_DEVICEREMOVECOMPLETE:" << lpdb->dbch_devicetype;
                        if(lpdb->dbch_devicetype == DBT_DEVTYP_PORT)
                        {
                            PDEV_BROADCAST_PORT lpdbv = (PDEV_BROADCAST_PORT)lpdb;
                            QString portName = QString::fromWCharArray(lpdbv->dbcp_name);
                            qDebug() << "device remove complete:" << portName;

                            emit comDevRemoveComplete(portName);
                        }
                        break;
                    case DBT_DEVNODES_CHANGED:
                        break;
                    default:
                        break;
                }
            }
        }
        return false;
    }

signals:
    // 串口插入了
    void comDevArriaval(QString devName);
    //串口拔掉了
    void comDevRemoveComplete(QString devName);

};

#endif // SERIALEVENT_H
