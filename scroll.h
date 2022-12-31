#ifndef SCROLL_H
#define SCROLL_H

#include <QObject>
#include<QLabel>
#include<QWidget>
#include <QTimer>
class Scroll :  public QLabel
{
    Q_OBJECT
public:
    Scroll(QWidget *parent = 0);
public:
    void showScrollText(QString text);
private:
    QTimer   m_TimerRoll;
    QString showText;
private slots:
    void updateIndex();
};

#endif // SCROLL_H
