// AccelerationSlider.cpp
// Class implementation file

// Project includes
#include "AccelerationSlider.h"
#include "CallTracer.h"

// Qt includes
#include <QDebug>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor
AccelerationSlider::AccelerationSlider()
{
    CALL_IN("");

    setMinimum(-3);
    setMaximum(3);
    setValue(0);
    setTracking(true);
    connect (this, SIGNAL(valueChanged(int)),
        this, SLOT(CheckScroll()));
    connect (this, SIGNAL(sliderReleased()),
        this, SLOT(EndScroll()));
    
    // Configure timer
    connect (&m_DelayTimer, SIGNAL(timeout()),
        this, SLOT(CheckScroll()));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
AccelerationSlider::~AccelerationSlider()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



// ====================================================================== Stuff



///////////////////////////////////////////////////////////////////////////////
// Check for scroll
void AccelerationSlider::CheckScroll()
{
    CALL_IN("");

    // Stop timer if it's running
    // (that's the case when we move to a new value while we still delay for
    // the old value)
    if (m_DelayTimer.isActive())
    {
        m_DelayTimer.stop();
    }
    
    // Scroll forward/backward
    if (value() > 0)
    {
        emit ScrollForward();
    } else if (value() < 0)
    {
        emit ScrollBackward();
    }

    // Start timer anew
    switch (value())
    {
    case -3:
        // Falls thru
    case 3:
        m_DelayTimer.start(60);
        break;

    case -2:
        // Falls thru
    case 2:
        m_DelayTimer.start(300);
        break;

    case -1:
        // Falls thru
    case 1:
        m_DelayTimer.start(600);
        break;

    case -0:
        // Do nothing.
        break;
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Scroll ends (if released)
void AccelerationSlider::EndScroll()
{
    CALL_IN("");

    // Stop timer
    m_DelayTimer.stop();
    
    // Reset value
    setValue(0);

    CALL_OUT("");
}
