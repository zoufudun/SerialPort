#include "scroll.h"

#define IDLENTH 10
Scroll::Scroll(QWidget *parent):QLabel(parent)
{
    connect(&m_TimerRoll,  SIGNAL(timeout()),  this,  SLOT(updateIndex()));
}

void Scroll::showScrollText(QString text)
{
    if(m_TimerRoll.isActive())
        m_TimerRoll.stop();

    showText = text;
    m_TimerRoll.start(300);
}

void Scroll::updateIndex()
{

    static int nPos = 0;

//    if(showText != NULL)
//    {
        if (nPos > showText.length())
        {
            nPos = 0;
        }
        this->setText(showText.mid(nPos));
        QString str = this->text().append(" "+showText.left(nPos));
        this->setText(str);
        nPos++;
        if(showText.length() < IDLENTH)
        {
            this->setText(showText);
        }
//    }

}
