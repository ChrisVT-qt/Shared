// CoverArtWidget.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "CoverArtWidget.h"

// Qt includes
#include <QMouseEvent>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
CoverArtWidget::CoverArtWidget() :
    PixmapWidget()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
CoverArtWidget::~CoverArtWidget()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



// ====================================================================== Other



///////////////////////////////////////////////////////////////////////////////
// Mouse click
void CoverArtWidget::mousePressEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
                .arg(CALL_SHOW(mpEvent)));

    // Check if left mousebutton is pressed
    if (!(mpEvent -> buttons() & Qt::LeftButton))
    {
        // Nope. We don't care, then.
        CALL_OUT("");
        return;
    }

    // Accept event
    mpEvent -> accept();

    // Any click
    emit SingleClick();

    // Check if image was clicked (and not the frame around it)
    const int mouse_x = mpEvent -> pos().x();
    const int mouse_y = mpEvent -> pos().y();
    if (mouse_x >= m_ScaledPixmap_x0 &&
        mouse_x <= m_ScaledPixmap_x1 &&
        mouse_y >= m_ScaledPixmap_y0 &&
        mouse_y <= m_ScaledPixmap_y1)
    {
        emit ClickOnImage();
    } else
    {
        emit ClickOnFrame();
    }

    CALL_OUT("");
}
