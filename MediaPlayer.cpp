// MediaPlayer.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "ClickableWidget.h"
#include "CoverArtWidget.h"
#include "MediaPlayer.h"
#include "MessageLogger.h"
#include "Preferences.h"
#include "StringHelper.h"
#include "VideoWidget.h"

// Qt includes
#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QDrag>
#include <QEventLoop>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QMediaMetaData>
#include <QMimeData>
#include <QMouseEvent>
#include <QRadioButton>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>


#include <QAudioDevice>
#include <QMediaDevices>

// Default volume
#define DEFAULT_VOLUME 0.8

// Size of the pixmap when dragging
#define DRAG_PIXMAP_SIZE 200



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
MediaPlayer::MediaPlayer()
{
    CALL_IN("");

    // We do have a playlist per default
    m_HasPlayList = true;

    // Initialize GUI
    InitGUI();

    // ... and actions
    InitActions();

    // Turn off FFmpeg sending lots of debug information to the console window
    QLoggingCategory::setFilterRules("qt.multimedia.*=false");

    // Make sure we have a place to store dragged files
    Preferences * p = Preferences::Instance();
    const QString attachment_dir =
        p -> GetValue("Attachments:Storage:Directory");
    const QString database = p -> GetValue("Database:Name");
    const QString drag_directory =
        QString("%1/%2/drag/")
            .arg(attachment_dir,
                 database);
    QDir dir("/");
    if (!dir.exists(drag_directory))
    {
        dir.mkpath(drag_directory);
    }

    // Index counter
    m_PlayList_NextIndex = 0;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
MediaPlayer::~MediaPlayer()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



// ======================================================================== GUI



///////////////////////////////////////////////////////////////////////////////
// Initialize GUI
void MediaPlayer::InitGUI()
{
    CALL_IN("");

    setAttribute(Qt::WA_MacSmallSize);

    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);


    // == Video/cover art

    // Top layout
    QHBoxLayout * top_layout = new QHBoxLayout();
    layout -> addLayout(top_layout);

    // Media
    m_Media = new QMediaPlayer(this);
    m_Video = new VideoWidget(this);
    connect (m_Video, &VideoWidget::MouseButtonPressed,
        this, &MediaPlayer::TogglePlayPause);
    connect (m_Video, &VideoWidget::StartDrag,
        this, &MediaPlayer::StartDragFrame);
    m_Media -> setVideoOutput(m_Video);
    top_layout -> addWidget(m_Video);
    connect(m_Media, &QMediaPlayer::positionChanged,
        this, &MediaPlayer::PositionChanged);

    // Catch frame changes to avoid black final frame at then end of the video
    connect (m_Media -> videoSink(), &QVideoSink::videoFrameChanged,
        this, &MediaPlayer::CaptureFinalFrame);
    connect (m_Media, &QMediaPlayer::positionChanged,
        this, &MediaPlayer::ReplayPositionChanged);

    // Audio
    m_Audio = new QAudioOutput();
    m_MediaDevices = new QMediaDevices();
    // Always stick with the default system audio output device
    connect (m_MediaDevices, &QMediaDevices::audioOutputsChanged,
            this,
            [=](){m_Audio -> setDevice(
                m_MediaDevices -> defaultAudioOutput());});
    m_Media -> setAudioOutput(m_Audio);
    m_Audio -> setVolume(DEFAULT_VOLUME);

    // Cover art (for audio)
    m_CoverArt = new CoverArtWidget();
    connect (m_CoverArt, &CoverArtWidget::ClickOnImage,
            this, &MediaPlayer::TogglePlayPause);
    top_layout -> addWidget(m_CoverArt);
    m_CoverArt -> hide();


    // == Controls
    QHBoxLayout * time_layout = new QHBoxLayout();
    layout -> addLayout(time_layout);

    m_CurrentTime = new QLabel(this);
    time_layout -> addWidget(m_CurrentTime);

    m_TimeSlider = new QSlider(this);
    m_TimeSlider -> setOrientation(Qt::Horizontal);
    connect (m_TimeSlider, &QSlider::sliderPressed,
        this, &MediaPlayer::Time_Pressed);
    connect (m_TimeSlider, &QSlider::sliderMoved,
        this, &MediaPlayer::Time_Moved);
    connect (m_TimeSlider, &QSlider::sliderReleased,
        this, &MediaPlayer::Time_Released);
    time_layout -> addWidget(m_TimeSlider);

    m_MaxTime = new QLabel(this);
    time_layout -> addWidget(m_MaxTime);

    time_layout -> setStretch(0, 0);
    time_layout -> setStretch(1, 1);
    time_layout -> setStretch(2, 0);

    // Volume and controls
    QHBoxLayout * controls_layout = new QHBoxLayout();
    layout -> addLayout(controls_layout);

    m_MuteButton = new QToolButton(this);
    m_MuteButton -> setIcon(style() -> standardIcon(QStyle::SP_MediaVolume));
    connect (m_MuteButton, &QAbstractButton::clicked,
        this, &MediaPlayer::ToggleMute);
    controls_layout -> addWidget(m_MuteButton);

    m_Volume = new QSlider(this);
    m_Volume -> setOrientation(Qt::Horizontal);
    m_Volume -> setRange(0, 100);
    m_Volume -> setValue(DEFAULT_VOLUME * 100);
    connect (m_Volume, &QSlider::valueChanged,
        this, &MediaPlayer::SetVolume);
    controls_layout -> addWidget(m_Volume);

    m_PreviousButton = new QToolButton(this);
    m_PreviousButton -> setIcon(
        style() -> standardIcon(QStyle::SP_MediaSkipBackward));
    connect(m_PreviousButton, &QAbstractButton::clicked,
        this, &MediaPlayer::PreviousFile);
    controls_layout -> addWidget(m_PreviousButton);

    m_PlayPauseButton = new QToolButton(this);
    m_PlayPauseButton -> setIcon(
        style() -> standardIcon(QStyle::SP_MediaPlay));
    connect(m_PlayPauseButton, &QAbstractButton::clicked,
        this, &MediaPlayer::TogglePlayPause);
    controls_layout -> addWidget(m_PlayPauseButton);

    m_NextButton = new QToolButton(this);
    m_NextButton -> setIcon(
        style() -> standardIcon(QStyle::SP_MediaSkipForward));
    connect(m_NextButton, &QAbstractButton::clicked,
        this, &MediaPlayer::NextFile);
    controls_layout -> addWidget(m_NextButton);

    controls_layout -> setStretch(0, 1);
    controls_layout -> setStretch(1, 0);
    controls_layout -> setStretch(2, 0);
    controls_layout -> setStretch(3, 0);


    // == Playlist
    m_PlayList = new QWidget(this);
    QVBoxLayout * playlist_layout = new QVBoxLayout();
    m_PlayList -> setLayout(playlist_layout);
    QScrollArea * scroll_area = new QScrollArea();
    m_PlayListContainerWidget = new QWidget();
    scroll_area -> setWidget(m_PlayListContainerWidget);
    scroll_area -> setWidgetResizable(true);
    scroll_area -> setMinimumHeight(200);
    layout -> addWidget(scroll_area);
    layout -> addWidget(m_PlayList);

    m_PlayListLayout = new QVBoxLayout();
    m_PlayListLayout -> setSpacing(0);
    m_PlayListContainerWidget -> setLayout(m_PlayListLayout);

    QHBoxLayout * repeat_layout = new QHBoxLayout();
    QLabel * l_repeat = new QLabel(tr("Repeat"));
    repeat_layout -> addWidget(l_repeat);
    m_PlayList_RepeatMode = new QButtonGroup(this);
    connect (m_PlayList_RepeatMode, &QButtonGroup::idClicked,
        this, &MediaPlayer::SetRepeatMode);

    QRadioButton * rb_repeat = new QRadioButton(tr("None"));
    m_PlayList_RepeatMode -> addButton(rb_repeat, Repeat_None);
    repeat_layout -> addWidget(rb_repeat);

    rb_repeat = new QRadioButton(tr("Single"));
    m_PlayList_RepeatMode -> addButton(rb_repeat, Repeat_Single);
    repeat_layout -> addWidget(rb_repeat);

    rb_repeat = new QRadioButton(tr("All"));
    m_PlayList_RepeatMode -> addButton(rb_repeat, Repeat_All);
    repeat_layout -> addWidget(rb_repeat);
    SetRepeatMode(Repeat_All);

    repeat_layout -> addStretch(1);

    playlist_layout -> addLayout(repeat_layout);


    layout -> setStretch(0, 1);
    layout -> setStretch(1, 0);
    layout -> setStretch(2, 0);

    setMinimumSize(400, 400);
    resize(800, 600);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Time slider pressed
void MediaPlayer::Time_Pressed()
{
    CALL_IN("");

    // Preserve play state
    m_Time_WasPlaying = m_Media -> isPlaying();

    // Pause
    Pause();

    // Move to currently selected position
    m_Media -> setPosition(m_TimeSlider -> value());

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Start dragging a frame
void MediaPlayer::StartDragFrame()
{
    CALL_IN("");

    // Pause media replay
    Pause();

    // Filename
    Preferences * p = Preferences::Instance();
    const QString attachment_dir =
        p -> GetValue("Attachments:Storage:Directory");
    const QString database = p -> GetValue("Database:Name");
    const QString source = m_Media -> source().fileName();
    static const QRegularExpression format_extension("^(.*)\\.([^\\.]+)$");
    const QRegularExpressionMatch match_extension =
        format_extension.match(source);
    QString basename;
    if (match_extension.hasMatch())
    {
        basename = match_extension.captured(1);
    } else
    {
        basename = tr("Video");
    }
    const qint64 pos = m_Media -> position();
    const QString filename =
        QString("%1/%2/drag/%3 (frame at %4.%5s).png")
            .arg(attachment_dir,
                 database,
                 basename,
                 QString::number(pos/1000),
                 QString("00" + QString::number(pos % 1000)).right(3));

    // Save frame to file
    QImage frame = GetCurrentFrame();
    frame.save(filename, "png");

    QDrag * drag = new QDrag(this);
    QMimeData * mime_data = new QMimeData;
    QList < QUrl> urls;
    urls << QUrl(filename);
    mime_data -> setUrls(urls);
    drag -> setMimeData(mime_data);

    if (frame.width() > DRAG_PIXMAP_SIZE ||
        frame.height() > DRAG_PIXMAP_SIZE)
    {
        frame = frame.scaled(QSize(DRAG_PIXMAP_SIZE, DRAG_PIXMAP_SIZE),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    drag -> setPixmap(QPixmap::fromImage(frame));
    drag -> setHotSpot(QPoint(frame.width()/2, frame.height()/2));

    drag -> exec(Qt::CopyAction | Qt::MoveAction);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Time slider moved (while being pressed)
void MediaPlayer::Time_Moved()
{
    CALL_IN("");

    m_Media -> setPosition(m_TimeSlider -> value());

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Time slider released
void MediaPlayer::Time_Released()
{
    CALL_IN("");

    if (m_Time_WasPlaying)
    {
        Play();
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Toggle mute
void MediaPlayer::ToggleMute()
{
    CALL_IN("");

    if (m_Audio -> isMuted())
    {
        m_MuteButton -> setIcon(
            style() -> standardIcon(QStyle::SP_MediaVolume));
        m_Audio -> setMuted(false);
        m_Volume -> setEnabled(true);
    } else
    {
        m_MuteButton -> setIcon(
            style() -> standardIcon(QStyle::SP_MediaVolumeMuted));
        m_Audio -> setMuted(true);
        m_Volume -> setEnabled(false);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Set volume
void MediaPlayer::SetVolume(const int mcVolume)
{
    CALL_IN(QString("mcVolume=%1")
        .arg(CALL_SHOW(mcVolume)));

    const double log_volume = (exp(mcVolume * 0.01) - 1) / (exp(1) - 1);
    m_Audio -> setVolume(log_volume);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Pause/play
void MediaPlayer::TogglePlayPause()
{
    CALL_IN("");

    if (m_Media -> isPlaying())
    {
        Pause();
    } else
    {
        Play();
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void MediaPlayer::PositionChanged(const qint64 mcNewPosition)
{
    CALL_IN(QString("mcNewPosition=%1")
        .arg(CALL_SHOW(mcNewPosition)));

    // Update text
    static qint64 old_s = -1;
    qint64 new_s = mcNewPosition/1000;
    if (new_s != old_s)
    {
        old_s = new_s;

        // Update current time
        m_CurrentTime -> setText(StringHelper::ConvertToTime(new_s));
    }

    // Update slider
    m_TimeSlider -> blockSignals(true);
    m_TimeSlider -> setValue(mcNewPosition);
    m_TimeSlider -> blockSignals(false);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void MediaPlayer::PreviousFile()
{
    CALL_IN("");

    const int index_in_list =
        m_PlayList_Indices.indexOf(m_PlayList_CurrentIndex);
    if (index_in_list == 0)
    {
        PlayPlayListIndex(m_PlayList_Indices.last());
    } else
    {
        PlayPlayListIndex(m_PlayList_Indices[index_in_list - 1]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
void MediaPlayer::NextFile()
{
    CALL_IN("");

    const bool was_last_file =
        (m_PlayList_CurrentIndex == m_PlayList_Indices.last());
    if (was_last_file)
    {
        PlayPlayListIndex(0);
    } else
    {
        const int index_in_list =
            m_PlayList_Indices.indexOf(m_PlayList_CurrentIndex);
        PlayPlayListIndex(m_PlayList_Indices[index_in_list + 1]);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Close widget
void MediaPlayer::closeEvent(QCloseEvent * mpEvent)
{
    CALL_IN(QString("mpEvent=%1")
        .arg(CALL_SHOW(mpEvent)));

    // Stop playing
    Pause();

    QWidget::closeEvent(mpEvent);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Actions
void MediaPlayer::InitActions()
{
    CALL_IN("");

    QAction * action = new QAction(this);
    action -> setShortcut(QKeySequence(" "));
    connect (action, &QAction::triggered,
            this, &MediaPlayer::TogglePlayPause);
    addAction(action);

    action = new QAction(this);
    action -> setShortcut(QKeySequence(Qt::Key_Escape));
    connect (action, &QAction::triggered,
            this, &MediaPlayer::close);
    addAction(action);

    action = new QAction(this);
    action -> setShortcut(QKeySequence(Qt::Key_Left));
    connect (action, &QAction::triggered,
            this, &MediaPlayer::FrameBackward);
    addAction(action);

    action = new QAction(this);
    action -> setShortcut(QKeySequence(Qt::Key_Right));
    connect (action, &QAction::triggered,
            this, &MediaPlayer::FrameForward);
    addAction(action);

    action = new QAction(this);
    action -> setShortcut(QKeySequence("Ctrl+C"));
    connect (action, &QAction::triggered,
            this, &MediaPlayer::CopyFrame);
    addAction(action);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Skip single frame forward
void MediaPlayer::FrameForward()
{
    CALL_IN("");

    // Make sure media replay is paused
    if (m_Media -> isPlaying())
    {
        TogglePlayPause();
    }

    // Get next frame
    QMediaMetaData meta_data = m_Media -> metaData();
    qint64 position = m_Media -> position();
    const float fps = meta_data[QMediaMetaData::VideoFrameRate].toFloat();
    qint64 one_frame = int(1000 / fps);
    m_Media -> setPosition(position + one_frame);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Skip single from backward
void MediaPlayer::FrameBackward()
{
    CALL_IN("");

    // Make sure media file is paused
    if (m_Media -> isPlaying())
    {
        TogglePlayPause();
    }

    // Get previous frame
    QMediaMetaData meta_data = m_Media -> metaData();
    qint64 position = m_Media -> position();
    const float fps = meta_data[QMediaMetaData::VideoFrameRate].toFloat();
    qint64 one_frame = int(1000 / fps);
    m_Media -> setPosition(position - one_frame);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Grab current frame
void MediaPlayer::CopyFrame()
{
    CALL_IN("");

    const QImage image = GetCurrentFrame();

    // Put it in clipboard
    QClipboard * clipboard = QGuiApplication::clipboard();
    clipboard -> setPixmap(QPixmap::fromImage(image));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Try and capture final frame
void MediaPlayer::CaptureFinalFrame(const QVideoFrame & mcrFrame)
{
    CALL_IN(QString("mcrFrame=%1")
        .arg("..."));

    static QVideoFrame previous_frame;
    if (mcrFrame.isValid())
    {
        previous_frame = mcrFrame;
    } else
    {
        m_Media -> videoSink() -> setVideoFrame(previous_frame);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Position changed during replay
void MediaPlayer::ReplayPositionChanged(const qint64 mcNewPosition)
{
    CALL_IN(QString("mcNewPosition=%1")
        .arg(CALL_SHOW(mcNewPosition)));

    // Check if we reached the end
    qint64 max_time = m_PlayList_MaxTimeMS[m_PlayList_CurrentIndex];
    if (max_time == -1)
    {
        max_time = m_PlayList_DurationMS[m_PlayList_CurrentIndex];
    }
    if (mcNewPosition >= max_time)
    {
        emit ReplayFinished();
        bool was_last_file =
            (m_PlayList_CurrentIndex == m_PlayList_Indices.last());
        switch (m_RepeatMode)
        {
            case Repeat_None:
                if (was_last_file)
                {
                    Pause();
                } else
                {
                    NextFile();
                }
                break;

            case Repeat_Single:
                PlayPlayListIndex(m_PlayList_CurrentIndex);
                break;

            case Repeat_All:
                if (was_last_file)
                {
                    PlayPlayListIndex(0);
                } else
                {
                    NextFile();
                }
                break;

            default:
            {
                const QString reason = tr("Unhandled repeat option.");
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return;
            }
        }
    }

    CALL_OUT("");
}



// =================================================================== Playlist



///////////////////////////////////////////////////////////////////////////////
// Set if we have a playlist
void MediaPlayer::SetHasPlayList(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    // Set new state
    m_HasPlayList = mcNewState;
    m_PlayList -> setVisible(mcNewState);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Check if there is a playlist
bool MediaPlayer::HasPlayList() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_HasPlayList;
}



///////////////////////////////////////////////////////////////////////////////
// Clear play list
void MediaPlayer::ClearPlayList()
{
    CALL_IN("");

    // Check if playlist is empty already to avoid unnecessary redraw
    if (!m_PlayList_Indices.isEmpty())
    {
        m_PlayList_Indices.clear();
        m_PlayList_Filename.clear();
        m_PlayList_Title.clear();
        m_PlayList_MinTimeMS.clear();
        m_PlayList_MaxTimeMS.clear();
        m_PlayList_CoverArt.clear();
        Update_PlayList();
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Current index
int MediaPlayer::GetCurrentIndex() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_PlayList_CurrentIndex;
}



///////////////////////////////////////////////////////////////////////////////
// Add media to playlist (doesn't start playing it)
int MediaPlayer::AddMediaToPlayList(const QString & mcrLocalFilename,
    const QString & mcrTitle, const qint64 mcMinTimeMS,
    const qint64 mcMaxTimeMS, const QPixmap & mcrCoverArt)
{
    CALL_IN(QString("mcrLocalFilename=%1, mcrTitle=%2, mcMinTimeMS=%3, "
            "mcMaxTimeMS=%4, mcrCoverArt=%5")
        .arg(CALL_SHOW(mcrLocalFilename),
             CALL_SHOW(mcrTitle),
             CALL_SHOW(mcMinTimeMS),
             CALL_SHOW(mcMaxTimeMS),
             CALL_SHOW(mcrCoverArt)));

    // Check if we already have this file on the playlist
    for (int index : m_PlayList_Indices)
    {
        if (m_PlayList_Filename[index] == mcrLocalFilename &&
            mcMinTimeMS == m_PlayList_MinTimeMS[index] &&
            mcMaxTimeMS == m_PlayList_MaxTimeMS[index])
        {
            CALL_OUT("");
            return index;
        }
    }

    // Wait until source has been read; setSource() returns immediately
    // and does not wait for the media file to be loaded.
    QEventLoop event_loop;
    QMediaPlayer * media = new QMediaPlayer();
    connect (media, &QMediaPlayer::mediaStatusChanged,
        &event_loop, &QEventLoop::quit);
    media -> setSource(QUrl::fromLocalFile(mcrLocalFilename));
    event_loop.exec();

    if (media -> error() != QMediaPlayer::NoError)
    {
        const QString error_text = media -> errorString();
        const QString reason = tr("An error occurred while opening media file "
            "\"%1\": %2")
            .arg(mcrLocalFilename,
                error_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Available keys
    const QMediaMetaData meta_data = media -> metaData();
    QList < QMediaMetaData::Key > meta_keys = meta_data.keys();

    // Duration
    if (!meta_keys.contains(QMediaMetaData::Duration))
    {
        const QString reason =
            tr("File \"%1\" does not appear to be a video or audio file")
                .arg(mcrLocalFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }
    const qint64 duration_ms =
        meta_data.value(QMediaMetaData::Duration).toLongLong();

    // Start and end time (ms)
    if (mcMaxTimeMS != -1 &&
        mcMinTimeMS > mcMaxTimeMS)
    {
        const QString reason =
            tr("Start time (%1ms) is after end time (%2ms)")
                .arg(QString::number(mcMinTimeMS),
                     QString::number(mcMaxTimeMS));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }
    if (mcMinTimeMS > duration_ms)
    {
        const QString reason =
            tr("Start time (%1ms) exceeds duration of the media file (%2ms)")
                .arg(QString::number(mcMinTimeMS),
                     QString::number(duration_ms));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    // Add media data
    const int new_index = m_PlayList_NextIndex++;
    m_PlayList_Indices << new_index;
    m_PlayList_Filename[new_index] = mcrLocalFilename;
    m_PlayList_Title[new_index] = mcrTitle;
    m_PlayList_MinTimeMS[new_index] = mcMinTimeMS;
    m_PlayList_MaxTimeMS[new_index] = mcMaxTimeMS;
    m_PlayList_DurationMS[new_index] = duration_ms;
    m_PlayList_CoverArt[new_index] = mcrCoverArt;

    Update_PlayList();

    CALL_OUT("");
    return new_index;
}



///////////////////////////////////////////////////////////////////////////////
// Update media information
bool MediaPlayer::UpdateInformation(const int mcIndex,
    const QString & mcrLocalFilename, const QString & mcrTitle,
    const qint64 mcMinTimeMS, const qint64 mcMaxTimeMS,
    const QPixmap & mcrCoverArt)
{
    CALL_IN(QString("mcIndex=%1, mcrLocalFilename=%2, mcrTitle=%3, "
        "mcMinTimeMS=%4, mcMaxTimeMS=%5, mcrCoverArt=%6")
        .arg(CALL_SHOW(mcIndex),
             CALL_SHOW(mcrLocalFilename),
             CALL_SHOW(mcrTitle),
             CALL_SHOW(mcMinTimeMS),
             CALL_SHOW(mcMaxTimeMS),
             CALL_SHOW(mcrCoverArt)));

    // Check if index exists
    if (!m_PlayList_Indices.contains(mcIndex))
    {
        const QString reason = tr("Invalid index %1.")
            .arg(QString::number(mcIndex));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Wait until source has been read; setSource() returns immediately
    // and does not wait for the media file to be loaded.
    QEventLoop event_loop;
    QMediaPlayer * media = new QMediaPlayer();
    connect (media, &QMediaPlayer::mediaStatusChanged,
        &event_loop, &QEventLoop::quit);
    media -> setSource(QUrl::fromLocalFile(mcrLocalFilename));
    event_loop.exec();

    if (media -> error() != QMediaPlayer::NoError)
    {
        const QString error_text = media -> errorString();
        const QString reason = tr("An error occurred while opening media file "
            "\"%1\": %2")
            .arg(mcrLocalFilename,
                error_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Available keys
    const QMediaMetaData meta_data = media -> metaData();
    QList < QMediaMetaData::Key > meta_keys = meta_data.keys();

    // Duration
    if (!meta_keys.contains(QMediaMetaData::Duration))
    {
        const QString reason =
            tr("File \"%1\" does not appear to be a video or audio file")
                .arg(mcrLocalFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    const qint64 duration_ms =
        meta_data.value(QMediaMetaData::Duration).toLongLong();

    // Start and end time (ms)
    if (mcMaxTimeMS != -1 &&
        mcMinTimeMS > mcMaxTimeMS)
    {
        const QString reason =
            tr("Start time (%1ms) is after end time (%2ms)")
                .arg(QString::number(mcMinTimeMS),
                     QString::number(mcMaxTimeMS));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    if (mcMinTimeMS > duration_ms)
    {
        const QString reason =
            tr("Start time (%1ms) exceeds duration of the media file (%2ms)")
                .arg(QString::number(mcMinTimeMS),
                     QString::number(duration_ms));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Add media data
    m_PlayList_Filename[mcIndex] = mcrLocalFilename;
    m_PlayList_Title[mcIndex] = mcrTitle;
    m_PlayList_MinTimeMS[mcIndex] = mcMinTimeMS;
    m_PlayList_MaxTimeMS[mcIndex] = mcMaxTimeMS;
    m_PlayList_DurationMS[mcIndex] = duration_ms;
    m_PlayList_CoverArt[mcIndex] = mcrCoverArt;

    Update_PlayList();

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Number of indices
int MediaPlayer::GetPlayListCount() const
{
    CALL_IN("");

    const int count = m_PlayList_Indices.size();

    CALL_OUT("");
    return count;
}



///////////////////////////////////////////////////////////////////////////////
// The indices
QList < int > MediaPlayer::GetAllPlayListIndices() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_PlayList_Indices;
}



///////////////////////////////////////////////////////////////////////////////
// Play a particular song
bool MediaPlayer::PlayPlayListIndex(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Check for "no media"
    if (mcIndex == -1)
    {
        Pause();
        m_PlayList_CurrentIndex = mcIndex;
        m_Media -> setSource(QString());
        m_CoverArt -> SetPixmap(QPixmap());
        m_Video -> hide();
        m_CoverArt -> show();
        setWindowTitle(tr("No media file"));
        CALL_OUT("");
        return true;
    }

    // Check if this is a valid index
    if (!m_PlayList_Indices.contains(mcIndex))
    {
        const QString reason = tr("Invalid playlist index %1.")
            .arg(QString::number(mcIndex));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Otherwise, play media file
    // Preserve volume and muted state
    bool is_muted = m_Audio -> isMuted();
    float volume = m_Audio -> volume();

    // This makes the media file restart if the same media file is to be played
    // again
    m_Media -> setSource(QString());

    // Abbreviations
    const QString filename = m_PlayList_Filename[mcIndex];
    const QString title = m_PlayList_Title[mcIndex];
    const qint64 min_time = m_PlayList_MinTimeMS[mcIndex];
    qint64 max_time = m_PlayList_MaxTimeMS[mcIndex];
    if (max_time == -1)
    {
        max_time = m_PlayList_DurationMS[mcIndex];
    }
    const QPixmap & cover_art = m_PlayList_CoverArt[mcIndex];

    // Wait until source has been read; setSource() returns immediately
    // and does not wait for the media file to be loaded.
    QEventLoop event_loop;
    connect (m_Media, &QMediaPlayer::mediaStatusChanged,
            &event_loop, &QEventLoop::quit);
    m_Media -> setSource(QUrl::fromLocalFile(filename));
    event_loop.exec();

    if (m_Media -> error() != QMediaPlayer::NoError)
    {
        const QString error_text = m_Media -> errorString();
        const QString reason = tr("An error occurred while opening media file "
            "\"%1\": %2")
            .arg(filename,
                error_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    m_TimeSlider -> setMinimum(min_time);
    m_TimeSlider -> setMaximum(max_time);
    m_MaxTime -> setText(StringHelper::ConvertToTime(max_time/1000));


    // Set window title
    setWindowTitle(title);

    // Set volume
    m_Audio -> setVolume(volume);
    if (is_muted)
    {
        m_Audio -> setMuted(true);
    }

    // Media type
    if (m_Media -> hasVideo())
    {
        m_CoverArt -> hide();
        m_Video -> show();
    } else if (m_Media -> hasAudio())
    {
        m_Video -> hide();
        m_CoverArt -> show();
    } else
    {
        m_Video -> hide();
        m_CoverArt -> hide();
    }

    // Sert cover art
    if (!cover_art.isNull())
    {
        // Check if it's an audio file we're playing
        if (m_Media -> hasVideo())
        {
            const QString reason = tr("Cannot set cover art for a video.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }

        // Set cover art
        m_CoverArt -> SetPixmap(cover_art);
        m_CoverArt -> update();
    } else
    {
        // Check if it's an audio file we're playing
        if (!m_Media -> hasVideo())
        {
            m_CoverArt -> hide();
        }
    }

    // Play media file
    m_PlayList_CurrentIndex = mcIndex;
    m_Media -> setPosition(min_time);
    Play();
    Update_PlayList();

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Update playlist
void MediaPlayer::Update_PlayList()
{
    CALL_IN("");

    // We regenerate the entire list because I don't know how to handle a
    // dynamic stretch at the bottom of the list (to fill any unused space
    // in the ScrollArea)


    // Alternating colors
    QList < QColor > alternating_colors =
        {QColor(230,230,230), QColor(240,240,240)};

    // Empty layout
    while (true)
    {
        QLayoutItem * child = m_PlayListLayout -> takeAt(0);
        if (!child)
        {
            break;
        }
        delete child -> widget();
        delete child;
    }

    // Check if we need to add the headers
    for (int row = 0;
         row < m_PlayList_Indices.size();
         row++)
    {
        const int index = m_PlayList_Indices[row];

        ClickableWidget * container = new ClickableWidget();
        container -> setContentsMargins(0, 0, 0, 0);
        QPalette palette = container -> palette();
        palette.setColor(QPalette::Active, QPalette::Window,
            alternating_colors[row % 2]);
        palette.setColor(QPalette::Inactive, QPalette::Window,
            alternating_colors[row % 2]);
        container -> setPalette(palette);

        QHBoxLayout * layout = new QHBoxLayout(container);
        layout -> setSpacing(0);
        layout -> setContentsMargins(0, 0, 0, 0);
        container -> setLayout(layout);
        connect (container, &ClickableWidget::SingleClicked,
            this, [=](){PlayPlayListIndex(index);});

        QLabel * label = new QLabel(container);
        label -> setFixedWidth(16);
        layout -> addWidget(label);
        if (index == m_PlayList_CurrentIndex)
        {
            label -> setPixmap(
                style() -> standardIcon(QStyle::SP_MediaPlay).pixmap(15,15));
        } else
        {
            label -> setPixmap(QPixmap());
        }

        label = new QLabel(container);
        label -> setAlignment(Qt::AlignRight);
        label -> setFixedWidth(20);
        layout -> addWidget(label);
        label -> setText(QString::number(row+1));

        layout -> addSpacing(5);

        label = new QLabel(container);
        layout -> addWidget(label);
        label -> setText(m_PlayList_Title[index]);

        label = new QLabel(container);
        label -> setFixedWidth(50);
        layout -> addWidget(label);
        qint64 max_time = m_PlayList_MaxTimeMS[index];
        if (max_time == -1)
        {
            max_time = m_PlayList_DurationMS[index];
        }
        const qint64 effective_duration_ms =
            max_time - m_PlayList_MinTimeMS[index];
        const QString effective_duration =
            StringHelper::ConvertToTime(effective_duration_ms/1000);
        label -> setText(effective_duration);

        layout -> setStretch(0, 0);
        layout -> setStretch(1, 0);
        layout -> setStretch(2, 1);
        layout -> setStretch(3, 0);

        m_PlayListLayout -> addWidget(container, row);
        m_PlayListLayout -> setStretch(row, 0);
    }

    // Fill any unused space
    m_PlayListLayout -> addStretch(1);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Set repeat mode
void MediaPlayer::SetRepeatMode(const int mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    // Check if state changes
    if (RepeatMode(mcNewState) == m_RepeatMode)
    {
        // No change
        CALL_OUT("");
        return;
    }

    // Set new state
    m_RepeatMode = RepeatMode(mcNewState);
    m_PlayList_RepeatMode -> blockSignals(true);
    m_PlayList_RepeatMode -> button(m_RepeatMode) -> setChecked(true);
    m_PlayList_RepeatMode -> blockSignals(false);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get repeat mode
MediaPlayer::RepeatMode MediaPlayer::GetRepeatMode() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_RepeatMode;
}



// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Play
void MediaPlayer::Play()
{
    CALL_IN("");

    // Check if we have a current song
    if (m_PlayList_CurrentIndex == -1)
    {
        CALL_OUT("");
        return;
    }

    // Check if we're at the end
    const qint64 position = m_Media -> position();
    qint64 max_time = m_PlayList_MaxTimeMS[m_PlayList_CurrentIndex];
    if (max_time == -1)
    {
        max_time = m_PlayList_DurationMS[m_PlayList_CurrentIndex];
    }
    if (position >= max_time)
    {
        // Start at the beginning
        const qint64 min_time = m_PlayList_MinTimeMS[m_PlayList_CurrentIndex];
        m_Media -> setPosition(min_time);
    }
    m_Media -> play();
    m_PlayPauseButton -> setIcon(
        style() -> standardIcon(QStyle::SP_MediaPause));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Pause
void MediaPlayer::Pause()
{
    CALL_IN("");

    // Check if we have a current song
    if (m_PlayList_CurrentIndex == -1)
    {
        CALL_OUT("");
        return;
    }

    m_Media -> pause();
    m_PlayPauseButton -> setIcon(
        style() -> standardIcon(QStyle::SP_MediaPlay));

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// PlayMediaFromFile media file
int MediaPlayer::PlayMediaFromFile(const QString & mcrLocalFilename,
    const QString & mcrTitle, const qint64 mcMinTimeMS,
    const qint64 mcMaxTimeMS, const QPixmap & mcrCoverArt)
{
    CALL_IN(QString("mcrLocalFilename=%1, mcrTitle=%2, mcMinTimeMS=%3, "
        "mcMaxTimeMS=%4, mcrCoverArt=%5")
        .arg(CALL_SHOW(mcrLocalFilename),
             CALL_SHOW(mcrTitle),
             CALL_SHOW(mcMinTimeMS),
             CALL_SHOW(mcMaxTimeMS),
             CALL_SHOW(mcrCoverArt)));

    if (!m_HasPlayList)
    {
        m_PlayList_Indices.clear();
        m_PlayList_Filename.clear();
        m_PlayList_Title.clear();
        m_PlayList_MinTimeMS.clear();
        m_PlayList_MaxTimeMS.clear();
        m_PlayList_CoverArt.clear();
    }

    const int index = AddMediaToPlayList(mcrLocalFilename, mcrTitle,
        mcMinTimeMS, mcMaxTimeMS, mcrCoverArt);
    if (index != -1)
    {
        PlayPlayListIndex(index);
    }

    CALL_OUT("");
    return index;
}



///////////////////////////////////////////////////////////////////////////////
// Remove index
bool MediaPlayer::RemoveMediaFile(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Check if index exists
    if (!m_PlayList_Indices.contains(mcIndex))
    {
        const QString reason = tr("Invalid index %1.")
            .arg(QString::number(mcIndex));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Deal with if this is the current song
    if (mcIndex == m_PlayList_CurrentIndex)
    {
        // Picking the next song:
        // 1. If this is the only song, invalidate the current index.
        // 1. If this is not the last song, skip to the next
        // 2. If this is the last song, pick the second-to-last
        if (m_PlayList_Indices.size() == 1)
        {
            PlayPlayListIndex(-1);
        } else
        {
            const int index_in_list = m_PlayList_Indices.indexOf(mcIndex);
            if (mcIndex == m_PlayList_Indices.last())
            {
                PlayPlayListIndex(m_PlayList_Indices[index_in_list - 1]);
            } else
            {
                PlayPlayListIndex(m_PlayList_Indices[index_in_list + 1]);
            }
        }

        // Not playing if the media file changed
        Pause();
    }

    // Remove index
    m_PlayList_Indices.removeAll(mcIndex);
    m_PlayList_Filename.remove(mcIndex);
    m_PlayList_Title.remove(mcIndex);
    m_PlayList_MinTimeMS.remove(mcIndex);
    m_PlayList_MaxTimeMS.remove(mcIndex);
    m_PlayList_CoverArt.remove(mcIndex);

    // Update playlist
    Update_PlayList();

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Get current frame as an image
QImage MediaPlayer::GetCurrentFrame() const
{
    CALL_IN("");

    QVideoSink * video_sink = m_Media -> videoSink();
    QVideoFrame video_frame = video_sink -> videoFrame();
    QImage image = video_frame.toImage();

    CALL_OUT("");
    return image;
}
