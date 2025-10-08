// PixmapWidget.h
// Class definition file

#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

// Qt includes
#include <QWidget>



class PixmapWidget
    : public QWidget
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    // Constructor
    PixmapWidget();

    // Destructor
    virtual ~PixmapWidget();



    // ==================================================================== GUI
public:
    // Set pixmap
    void SetPixmap(const QPixmap & mcrPixmap);
    QPixmap GetPixmap();
private:
    QPixmap m_Pixmap;

protected:
    // Picture coordinates
    int m_ScaledPixmap_x0;
    int m_ScaledPixmap_y0;
    int m_ScaledPixmap_x1;
    int m_ScaledPixmap_y1;

protected:
    void paintEvent(QPaintEvent * mpEvent);
};

#endif
