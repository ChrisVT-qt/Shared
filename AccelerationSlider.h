// AccelerationSlider.h
// Class definition file

// Just include once
#ifndef ACCELERATIONSLIDER_H
#define ACCELERATIONSLIDER_H

// Qt includes
#include <QSlider>
#include <QTimer>

// Class definition
class AccelerationSlider
    : public QSlider
{
    Q_OBJECT
    
    
    
    // ============================================================== Lifecycle
public:
	// Default constructor
	AccelerationSlider();

	// Destructor
	virtual ~AccelerationSlider();



    // ================================================================== Stuff
private slots:
    // Check for scroll
    void CheckScroll();

    // Scroll ends (if released)
    void EndScroll();
    
signals:
    // Scroll Forward
    void ScrollForward();
    
    // Scroll Backward
    void ScrollBackward();
private:
    QTimer m_DelayTimer;
};

#endif

