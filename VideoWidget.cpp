// VideoWidget.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "VideoWidget.h"

// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

// Standard includes
#include <cstring>
#include <algorithm>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
VideoWidget::VideoWidget(QWidget * mpParentWidget)
    : QWidget(mpParentWidget)
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Initialize video sink
    m_VideoSink = new QVideoSink(this);
    // New frame arrived
    connect (m_VideoSink, &QVideoSink::videoFrameChanged,
        this, &VideoWidget::FrameChanged);

    // Set no rotation
    m_Rotation = QtVideo::Rotation::None;

    // Not fixing HDR
    m_FixHDR = false;
    m_MaxLuminance = -1;

    // Current image is not fixed (we're expecting to show video)
    m_CurrentImage_Fixed = false;

    // Size policy
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // We paint the whole area ourselves
    setAttribute(Qt::WA_OpaquePaintEvent);

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



// ====================================================================== Video



///////////////////////////////////////////////////////////////////////////////
// Connect with QMediaPlayer::setVideoSink()
QVideoSink * VideoWidget::GetVideoSink() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_VideoSink;
}



///////////////////////////////////////////////////////////////////////////////
// Rotation
const QString VideoWidget::ToHumanReadable(const QtVideo::Rotation mcRotation)
{
    CALL_IN(QString("mcMode=%1")
        .arg("..."));

    // Mapper
    static QHash < QtVideo::Rotation, QString > mapper;
    if (mapper.isEmpty())
    {
        mapper[QtVideo::Rotation::None] = "0 degrees";
        mapper[QtVideo::Rotation::Clockwise90] = "90 degrees";
        mapper[QtVideo::Rotation::Clockwise180] = "180 degrees";
        mapper[QtVideo::Rotation::Clockwise270] = "270 degrees";
    }
    if (!mapper.contains(mcRotation))
    {
        const QString reason = tr("Unknown rotation encountered.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    CALL_OUT("");
    return mapper[mcRotation];
}



///////////////////////////////////////////////////////////////////////////////
// Set rotation
void VideoWidget::SetRotation(QtVideo::Rotation mcRotation)
{
    CALL_IN(QString("mcRotation=%1")
        .arg(CALL_SHOW(ToHumanReadable(mcRotation))));

    m_Rotation = mcRotation;
    update();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get rotation
QtVideo::Rotation VideoWidget::GetRotation() const
{
    CALL_IN("");
    CALL_OUT("");
    return m_Rotation;
}



///////////////////////////////////////////////////////////////////////////////
// Set if HDR mapping is active
void VideoWidget::SetFixHDR(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    m_FixHDR = mcNewState;
    update();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Set max luminance
void VideoWidget::SetMaxLuminance(const int mcMaxLuminance)
{
    CALL_IN(QString("mcMaxLuminance=%1")
        .arg(CALL_SHOW(mcMaxLuminance)));

    // !!! Check if luminance value is valid
    m_MaxLuminance = mcMaxLuminance;
    update();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Fix incorect color space information on some videos
QVideoFrame VideoWidget::FixHDR(const QVideoFrame & mcrFrame)
{
    // Note this fixes an issue with ffmpeg 9.0.1 (Qt 6.12.0)
    // Problem can be fixed be reverting back to darwin video handling
    // instead of ffmpeg:
    //
    // Put this in main.cpp before the Qt App gets instanciated:
    //   qputenv("QT_MEDIA_BACKEND", "darwin");
    //
    // We'll lose additional formats being recognized by ffmpeg, though,
    // so that's not what we'll do, and this is what this method is for.

    CALL_IN(QString("mcrFrame=%1")
        .arg("..."));

    // Check if we are being asked to fix in the first place
    if (!m_FixHDR)
    {
        // No
        CALL_OUT("");
        return mcrFrame;
    }

    // We don't care about invalid frames
    if (!mcrFrame.isValid())
    {
        CALL_OUT("");
        return mcrFrame;
    }

    QVideoFrameFormat fmt = mcrFrame.surfaceFormat();

    // Only intervene when the metadata is actually bad.
    const bool needs_space =
        (fmt.colorSpace() == QVideoFrameFormat::ColorSpace_Undefined);
    const bool needs_luminance =
        (fmt.maxLuminance() > 20000.f); // PQ tops out at 10,000 nits
    if (!needs_space &&
        !needs_luminance)
    {
        CALL_OUT("");
        return mcrFrame;
    }

    // Correct the metadata
    if (needs_space)
    {
        fmt.setColorSpace(QVideoFrameFormat::ColorSpace_BT2020);
    }
    if (needs_luminance &&
        m_MaxLuminance != -1)
    {
        fmt.setMaxLuminance(m_MaxLuminance);
    }

    QVideoFrame src(mcrFrame);
    QVideoFrame dst(fmt);
    if (!src.map(QVideoFrame::ReadOnly))
    {
        CALL_OUT("");
        return mcrFrame;
    }
    if (!dst.map(QVideoFrame::WriteOnly))
    {
        src.unmap();
        CALL_OUT("");
        return mcrFrame;
    }

    // P010
    // - plane 0 = Y (full height),
    // - plane 1 = interleaved CbCr (half height)
    const int h = fmt.frameHeight();
    for (int p = 0; p < src.planeCount(); p++)
    {
        const int rows = (p == 0) ? h : (h + 1) / 2;
        const int srcStride = src.bytesPerLine(p);
        const int dstStride = dst.bytesPerLine(p);
        const int rowBytes = std::min(srcStride, dstStride);
        for (int y = 0; y < rows; ++y)
            std::memcpy(dst.bits(p) + y * dstStride,
                        src.bits(p) + y * srcStride, rowBytes);
    }

    dst.unmap();
    src.unmap();

    dst.setStartTime(mcrFrame.startTime());
    dst.setEndTime(mcrFrame.endTime());

    // Maintain orientation
    dst.setRotation(mcrFrame.rotation());
    dst.setMirrored(mcrFrame.mirrored());

    CALL_OUT("");
    return dst;
}



///////////////////////////////////////////////////////////////////////////////
// Frame changed
void VideoWidget::FrameChanged(const QVideoFrame & mcrFrame)
{
    CALL_IN(QString("mcrFrame=%1")
        .arg(CALL_SHOW(mcrFrame)));

    if (!mcrFrame.isValid())
    {
        CALL_OUT("");
        return;
    }

    // Don't change the image if we have a fixed on (cover art)
    if (m_CurrentImage_Fixed)
    {
        CALL_OUT("");
        return;
    }

    // Fix HDR
    QVideoFrame frame = FixHDR(mcrFrame);

    // Remember image
    m_CurrentImage = frame.toImage();
    update();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Set current image
void VideoWidget::SetCurrentImage(const QImage & mcrNewImage)
{
    CALL_IN(QString("mcrNewImage=%1")
        .arg(CALL_SHOW(mcrNewImage)));

    m_CurrentImage_Fixed = !mcrNewImage.isNull();
    m_CurrentImage = mcrNewImage;
    update();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get current frame
QImage VideoWidget::GetCurrentImage() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_CurrentImage;
}



///////////////////////////////////////////////////////////////////////////////
// Paint video frame
void VideoWidget::paintEvent(QPaintEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    Q_UNUSED(mpEvent);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_CurrentImage.isNull())
    {
        CALL_OUT("");
        return;
    }

    // Size of the video after rotation
    QSizeF shown = m_CurrentImage.size();
    if (m_Rotation == QtVideo::Rotation::Clockwise90 ||
        m_Rotation == QtVideo::Rotation::Clockwise270)
    {
        shown.transpose();
    }
    static QHash < QtVideo::Rotation, int > rotation_mapper;
    if (rotation_mapper.isEmpty())
    {
        rotation_mapper[QtVideo::Rotation::None] = 0;
        rotation_mapper[QtVideo::Rotation::Clockwise90] = 90;
        rotation_mapper[QtVideo::Rotation::Clockwise180] = 180;
        rotation_mapper[QtVideo::Rotation::Clockwise270] = 270;
    }

    const qreal scale = qMin(1.0,
        qMin(width()  / shown.width(), height() / shown.height()));

    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.translate(rect().center());
    painter.rotate(rotation_mapper[m_Rotation]);
    painter.scale(scale, scale);
    painter.drawImage(
        QPointF(-m_CurrentImage.width()/2., -m_CurrentImage.height()/2.),
        m_CurrentImage);

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
