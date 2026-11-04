// CoverArtWidget.h
// Class definition file

#ifndef COVERARTWIDGET_H
#define COVERARTWIDGET_H

// Project includes
#include "PixmapWidget.h"



class CoverArtWidget
    : public PixmapWidget
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    // Constructor
    CoverArtWidget();

    // Destructor
    virtual ~CoverArtWidget();



    // ================================================================== Other
protected:
    virtual void mousePressEvent(QMouseEvent * mpEvent);
signals:
    void SingleClick();
    void ClickOnImage();
    void ClickOnFrame();
};

#endif
