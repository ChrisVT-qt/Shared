// VideoWidget.h
// Class definition file

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

// Qt includes
#include <QVideoWidget>

class VideoWidget :
    public QVideoWidget
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    VideoWidget(QWidget * mpParentWidget);
    virtual ~VideoWidget();



    // ==================================================== Additional Controls
protected:
    virtual void mousePressEvent(QMouseEvent * mpEvent);
    virtual void mouseMoveEvent(QMouseEvent * mpEvent);
    QPoint m_DragStartPosition;

signals:
    void MouseButtonPressed();
    void StartDrag();
};

#endif
