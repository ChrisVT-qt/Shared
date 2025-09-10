// MediaPlayer.h
// Class definition file

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

// Qt includes
#include <QAudioOutput>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QSlider>
#include <QToolButton>
#include <QVideoFrame>
#include <QWidget>

// Forward declaration
class ClickableWidget;
class CoverArtWidget;
class VideoWidget;



class MediaPlayer
    : public QWidget
{
    Q_OBJECT



    // ============================================================== Lifecycle
public:
    // Constructor
    MediaPlayer();

    // Destructor
    virtual ~MediaPlayer();



    // ==================================================================== GUI
private:
    // Initialize GUI
    void InitGUI();

protected:
    // Widgets
    QMediaPlayer * m_Media;
    VideoWidget * m_Video;
    QAudioOutput * m_Audio;
    QMediaDevices * m_MediaDevices;
    CoverArtWidget * m_CoverArt;

private:
    QLabel * m_CurrentTime;
    QSlider * m_TimeSlider;
    QLabel * m_MaxTime;

    QToolButton * m_MuteButton;
    QSlider * m_Volume;
    QToolButton * m_PreviousButton;
    QToolButton * m_PlayPauseButton;
    QToolButton * m_NextButton;

    QWidget * m_PlayList;
    QWidget * m_PlayListContainerWidget;
    QVBoxLayout * m_PlayListLayout;
    QButtonGroup * m_PlayList_RepeatMode;

private slots:
    void StartDragFrame();
    void Time_Pressed();
    void Time_Moved();
    void Time_Released();
private:
    bool m_Time_WasPlaying;

private slots:
    void ToggleMute();
    void SetVolume(const int mcVolume);
    void TogglePlayPause();
    void PositionChanged(const qint64 mcNewPosition);
    void PreviousFile();
    void NextFile();

protected:
    virtual void closeEvent(QCloseEvent * mpEvent);


private:
    // Actions
    void InitActions();

private slots:
    void FrameForward();
    void FrameBackward();
    void CopyFrame();
    void CaptureFinalFrame(const QVideoFrame & mcrFrame);

    void ReplayPositionChanged(const qint64 mcNewPosition);
signals:
    void ReplayFinished();



    // =============================================================== Playlist
public:
    // Playlist yes/no
    void SetHasPlayList(const bool mcNewState);
    bool HasPlayList() const;
private:
    bool m_HasPlayList;

public:
    // Clear play list
    void ClearPlayList();

    // Current index
    int GetCurrentIndex() const;
private:
    int m_PlayList_CurrentIndex;

public:
    // Add media to playlist
    int AddMediaToPlayList(const QString & mcrLocalFilename,
        const QString & mcrTitle, const qint64 mcMinTimeMS = 0,
        const qint64 mcMaxTimeMS = -1,
        const QPixmap & mcrCoverArt = QPixmap());
    bool UpdateInformation(const int mcIndex, const QString & mcrLocalFilename,
        const QString & mcrTitle, const qint64 mcMinTimeMS = 0,
        const qint64 mcMaxTimeMS = -1,
        const QPixmap & mcrCoverArt = QPixmap());

    // Number of indices
    int GetPlayListCount() const;

    // The indices
    QList < int > GetAllPlayListIndices() const;

    // Play a particular song
    bool PlayPlayListIndex(const int mcIndex);

private:
    QList < int > m_PlayList_Indices;
    QHash < int, QString > m_PlayList_Filename;
    QHash < int, QString > m_PlayList_Title;
    QHash < int, qint64 > m_PlayList_MinTimeMS;
    QHash < int, qint64 > m_PlayList_MaxTimeMS;
    QHash < int, qint64 > m_PlayList_DurationMS;
    QHash < int, QPixmap > m_PlayList_CoverArt;
    int m_PlayList_NextIndex;

private slots:
    void Update_PlayList();

public:
    enum RepeatMode {
        Repeat_None,
        Repeat_Single,
        Repeat_All
    };

    // Repeat mode
public slots:
    void SetRepeatMode(const int mcNewState);
public:
    RepeatMode GetRepeatMode() const;
private:
    RepeatMode m_RepeatMode;



    // ================================================================= Access
public slots:
    // Play/Pause
    void Play();
    void Pause();

    // PlayMediaFromFile media file
    int PlayMediaFromFile(const QString & mcrLocalFilename,
        const QString & mcrTitle, const qint64 mcMinTimeMS = 0,
        const qint64 mcMaxTimeMS = -1,
        const QPixmap & mcrCoverArt = QPixmap());

    // Remove index
    bool RemoveMediaFile(const int mcIndex);

public:
    // Get current frame as an image
    QImage GetCurrentFrame() const;
};

#endif
