// VideoWidget.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "VideoWidget.h"

// Qt includes
#include <QApplication>
#include <QMouseEvent>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
VideoWidget::VideoWidget(QWidget * mpParentWidget)
    : QVideoWidget(mpParentWidget)
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
VideoWidget::~VideoWidget()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



// ======================================================== Additional Controls



///////////////////////////////////////////////////////////////////////////////
// Mouse clicks
void VideoWidget::mousePressEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    mpEvent -> accept();

    if (mpEvent -> buttons() !=  Qt::LeftButton)
    {
        CALL_OUT("");
        return;
    }

    emit MouseButtonPressed();

    // Store drag start position
    m_DragStartPosition = mpEvent -> pos();

    CALL_OUT("");

}



///////////////////////////////////////////////////////////////////////////////
// Dragging
void VideoWidget::mouseMoveEvent(QMouseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    mpEvent -> accept();

    if (mpEvent -> buttons() !=  Qt::LeftButton)
    {
        CALL_OUT("");
        return;
    }

    const int distance =
        (mpEvent -> pos() - m_DragStartPosition).manhattanLength();
    if (distance < QApplication::startDragDistance())
    {
        // Not dragging
        CALL_OUT("");
        return;
    }

    // Start to drag
    emit StartDrag();

    CALL_OUT("");
}

