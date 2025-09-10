// PixmapWidget.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "PixmapWidget.h"

// Qt includes
#include <QPainter>
#include <QPaintEvent>



// ============================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
PixmapWidget::PixmapWidget()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
PixmapWidget::~PixmapWidget()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



// ======================================================================== GUI



///////////////////////////////////////////////////////////////////////////////
// Set pixmap
void PixmapWidget::SetPixmap(const QPixmap & mcrPixmap)
{
    CALL_IN(QString("mcrPixmap=%1")
        .arg(CALL_SHOW(mcrPixmap)));

    m_Pixmap = mcrPixmap;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Paint event
void PixmapWidget::paintEvent(QPaintEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Accept event
    mpEvent -> accept();

    // Check for no pixmap
    if (m_Pixmap.isNull())
    {
        m_ScaledPixmap_x0 = -1;
        m_ScaledPixmap_y0 = -1;
        m_ScaledPixmap_x1 = -1;
        m_ScaledPixmap_y1 = -1;
        CALL_OUT("");
        return;
    }

    // Scale to current size
    QPixmap scaled = m_Pixmap.scaled(width(), height(), Qt::KeepAspectRatio);
    if (scaled.width() > m_Pixmap.width())
    {
        // Don't enlarge
        scaled = m_Pixmap;
    }

    // Get box we need to draw
    const int pic_width = scaled.width();
    const int pic_height = scaled.height();

    m_ScaledPixmap_x0 = int(width() - pic_width)/2;
    m_ScaledPixmap_y0 = int(height() - pic_height)/2;
    m_ScaledPixmap_x1 = m_ScaledPixmap_x0 + scaled.width();
    m_ScaledPixmap_y1 = m_ScaledPixmap_y0 + scaled.height();

    // Paint it
    QPainter mypainter(this);
    mypainter.drawPixmap(m_ScaledPixmap_x0, m_ScaledPixmap_y0,
        scaled.width(), scaled.height(), scaled);

    CALL_OUT("");
}
