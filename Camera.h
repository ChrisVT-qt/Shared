// Camera.h
// Class definition file

#pragma once

// Qt includes
#include <QCamera>
#include <QCameraDevice>
#include <QMediaDevices>



// Class definition
class Camera :
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    // Default constructor (never to be called from outside)
    Camera();

    // Destructor
    ~Camera();



    // ============================================================= Everything
public:
    void Test();
};
