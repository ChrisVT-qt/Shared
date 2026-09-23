// Camera.cpp
// Class implementation file

// Project includes
#include "Application.h"
#include "CallTracer.h"
#include "Camera.h"
#include "MessageLogger.h"

// Qt includes
#include <QPermission>
#include <QDebug>
#include <QImageCapture>
#include <QMediaCaptureSession>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor (never to be called from outside)
Camera::Camera()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
Camera::~Camera()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do

    CALL_OUT("");
}



// ================================================================= Everything



///////////////////////////////////////////////////////////////////////////////
void Camera::Test()
{

    Application * a = Application::Instance();
    a -> requestPermission(QCameraPermission{},
        [](const QPermission & permission)
        {
            if (permission.status() == Qt::PermissionStatus::Granted)
            {
                QCamera * camera = new QCamera(QCameraDevice::FrontFace);
                QImageCapture * image_capture = new QImageCapture;
                QMediaCaptureSession * capture_session = new QMediaCaptureSession;

                capture_session -> setCamera(camera);
                capture_session -> setImageCapture(image_capture);

                // Start camera
                camera -> start();

                // Capture a picture
                image_capture -> captureToFile("/Users/shimaron/Desktop/capture.jpg");
            }
        });



#if 0
    const QList < QCameraDevice > cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice & cameraDevice : cameras)
    {
        if (cameraDevice.
        if cameraDevice.description();
    }
#endif
}
