// VideoWidget.h
// Class definition file

#pragma once

// Qt includes
#include <QImage>
#include <QPoint>
#include <QVideoFrame>
#include <QVideoSink>
#include <QWidget>



class VideoWidget :
    public QWidget
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    VideoWidget(QWidget * mpParentWidget);
    virtual ~VideoWidget();



    // ================================================================== Video
public:
    // Connect with QMediaPlayer::setVideoSink()
    QVideoSink * GetVideoSink() const;
private:
    QVideoSink * m_VideoSink;

public:
    static const QString ToHumanReadable(const QtVideo::Rotation mcRotation);

    // Rotation
public:
    void SetRotation(const QtVideo::Rotation mcRotation);
    QtVideo::Rotation GetRotation() const;
private:
    QtVideo::Rotation m_Rotation;

    // HDR mapping
public:
    void SetFixHDR(const bool mcNewState);
    void SetMaxLuminance(const int mcMaxLuminance);
private:
    QVideoFrame FixHDR(const QVideoFrame & mcrFrame);
    bool m_FixHDR;
    int m_MaxLuminance;

private slots:
    // Frame changed
    void FrameChanged(const QVideoFrame & mcrFrame);

public:
    void SetCurrentImage(const QImage & mcrNewImage = QImage());
    QImage GetCurrentImage() const;
private:
    bool m_CurrentImage_Fixed;
    QImage m_CurrentImage;

protected:
    void paintEvent(QPaintEvent * mpEvent) override;




    // ==================================================== Additional Controls
protected:
    virtual void mousePressEvent(QMouseEvent * mpEvent) override;
    virtual void mouseMoveEvent(QMouseEvent * mpEvent) override;
    QPoint m_DragStartPosition;

signals:
    void MouseButtonPressed();
    void StartDrag();
};
