// MediaHelper.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MediaHelper.h"
#include "MessageLogger.h"
#include "StringHelper.h"

// Qt includes
#include <QEventLoop>
#include <QFileInfo>
#include <QMediaFormat>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMimeDatabase>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor (never to be called from outside)
MediaHelper::MediaHelper()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
MediaHelper::~MediaHelper()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



// ================================================================== Functions



///////////////////////////////////////////////////////////////////////////////
// Get all available metadata
QHash < MediaHelper::Metadata, QString > MediaHelper::GetMediaMetadata(
    const QString & mcrFilename)
{
    CALL_IN(QString("mcrFilename=%1")
        .arg(CALL_SHOW(mcrFilename)));

    QHash < Metadata, QString > metadata;

    // Filename and directory
    const QPair < QString, QString > split_filename =
        StringHelper::SplitFilename(mcrFilename);
    metadata[Metadata_LocalDirectory] = split_filename.first;
    metadata[Metadata_LocalFilename] = split_filename.second;


    // == File-related data
    QFileInfo file_info(mcrFilename);

    // File date/time
    metadata[Metadata_FileDateTime] =
        file_info.lastModified().toString("yyyy-MM-dd hh:mm:ss");

    // Size
    metadata[Metadata_FileSize] = QString::number(file_info.size());

    // MIME type
    QMimeDatabase mime_database;
    const QMimeType mime_type = mime_database.mimeTypeForFile(file_info);
    if (mime_type.isValid())
    {
        metadata[Metadata_MIMEType] = mime_type.name();
    }

    // == Video-related data
    QMediaPlayer * media = new QMediaPlayer();

    // Wait until source has been read; setSource() returns immediately
    // and does not wait for the media to be loaded.
    QEventLoop event_loop;
    connect (media, &QMediaPlayer::mediaStatusChanged,
        &event_loop, &QEventLoop::quit);
    media -> setSource(QUrl::fromLocalFile(mcrFilename));
    event_loop.exec();

    if (media -> error() != QMediaPlayer::NoError)
    {
        const QString error_text = media -> errorString();
        const QString reason = tr("An error occurred while opening video file "
            "\"%1\": %2")
            .arg(mcrFilename,
                 error_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < Metadata, QString >();
    }

    // Available keys
    const QMediaMetaData media_metadata = media -> metaData();
    QList < QMediaMetaData::Key > meta_keys = media_metadata.keys();

    // We don't need the media file anymore
    delete media;

    // Title
    if (meta_keys.contains(QMediaMetaData::Title))
    {
        const QString title =
            media_metadata.value(QMediaMetaData::Title).toString();
        metadata[Metadata_Title] = title;
    }

    // Author
    if (meta_keys.contains(QMediaMetaData::Author))
    {
        const QString author =
            media_metadata.value(QMediaMetaData::Author).toString();
        metadata[Metadata_Author] = author;
    }

    // Comments
    if (meta_keys.contains(QMediaMetaData::Comment))
    {
        const QStringList comments =
            media_metadata.value(QMediaMetaData::Comment).toStringList();
        metadata[Metadata_Comments] = comments.join("\n");;
    }

    // Description
    if (meta_keys.contains(QMediaMetaData::Description))
    {
        const QString description =
            media_metadata.value(QMediaMetaData::Description).toString();
        metadata[Metadata_Description] = description;
    }

    // Genres
    if (meta_keys.contains(QMediaMetaData::Genre))
    {
        const QStringList genres =
            media_metadata.value(QMediaMetaData::Genre).toStringList();
        metadata[Metadata_Genres] = genres.join("\n");
    }

    // Date/Time
    if (meta_keys.contains(QMediaMetaData::Date))
    {
        const QDateTime date_time =
            media_metadata.value(QMediaMetaData::Date).toDateTime();
        metadata[Metadata_DateTime] =
            date_time.toString("yyyy-MM-dd hh:mm:ss");
    }

    // Language
    if (meta_keys.contains(QMediaMetaData::Language))
    {
        const int language =
            media_metadata.value(QMediaMetaData::Language).toInt();
        static QHash < int, QString > format_mapper;
        if (format_mapper.isEmpty())
        {
            // !!! Only implementing the ones we actually used at some point
            format_mapper[QLocale::English] = tr("English");
            format_mapper[QLocale::German] = tr("German");
        }
        if (format_mapper.contains(language))
        {
            metadata[Metadata_Language] = format_mapper[language];
        } else
        {
            // Report unmapped language
            const QString reason = tr("Unknown language ID %1 encountered "
                "while reading \"%2\" (ignored)")
                .arg(QString::number(language),
                     mcrFilename);
            MessageLogger::Error(CALL_METHOD, reason);
        }
    }

    // Publisher
    if (meta_keys.contains(QMediaMetaData::Publisher))
    {
        const QString publisher =
            media_metadata.value(QMediaMetaData::Publisher).toString();
        metadata[Metadata_Publisher] = publisher;
    }

    // Copyright
    if (meta_keys.contains(QMediaMetaData::Copyright))
    {
        const QString copyright =
            media_metadata.value(QMediaMetaData::Copyright).toString();
        metadata[Metadata_Copyright] = copyright;
    }

    // Url
    if (meta_keys.contains(QMediaMetaData::Url))
    {
        const QUrl url = media_metadata.value(QMediaMetaData::Url).toUrl();
        metadata[Metadata_Url] = url.toString();
    }

    // File format
    if (meta_keys.contains(QMediaMetaData::FileFormat))
    {
        static QHash < int, QString > format_mapper;
        if (format_mapper.isEmpty())
        {
            format_mapper[QMediaFormat::AAC] = "AAC";
            format_mapper[QMediaFormat::AVI] = "AVI";
            format_mapper[QMediaFormat::FLAC] = "FLAC";
            format_mapper[QMediaFormat::Matroska] = "MKV";
            format_mapper[QMediaFormat::MP3] = "MP3";
            format_mapper[QMediaFormat::MPEG4] = "MPEG-4";
            format_mapper[QMediaFormat::Mpeg4Audio] = "MPEG-4 Audio";
            format_mapper[QMediaFormat::Ogg] = "OGG";
            format_mapper[QMediaFormat::QuickTime] = "Quicktime";
            format_mapper[QMediaFormat::Wave] = "WAV";
            format_mapper[QMediaFormat::WebM] = "WebM";
            format_mapper[QMediaFormat::WMA] = "WMA";
            format_mapper[QMediaFormat::WMV] = "WMV";

            format_mapper[QMediaFormat::UnspecifiedFormat] = "";
        }
        const int format_id =
            media_metadata.value(QMediaMetaData::FileFormat).toInt();
        metadata[Metadata_FileFormat] = format_mapper[format_id];
    }

    // Duration
    if (meta_keys.contains(QMediaMetaData::Duration))
    {
        qint64 duration =
            media_metadata.value(QMediaMetaData::Duration).toLongLong();
        metadata[Metadata_Duration_ms] = QString::number(duration);
        metadata[Metadata_Duration_s] = QString::number(duration/1000);
    }

    // Resolution
    if (meta_keys.contains(QMediaMetaData::Resolution))
    {
        const QSize size =
            media_metadata.value(QMediaMetaData::Resolution).toSize();
        metadata[Metadata_Width] = QString::number(size.width());
        metadata[Metadata_Height] = QString::number(size.height());
    }

    // Audio bitrate
    if (meta_keys.contains(QMediaMetaData::AudioBitRate))
    {
        const int rate =
            media_metadata.value(QMediaMetaData::AudioBitRate).toInt();
        metadata[Metadata_AudioBitRate] = QString::number(rate);
    }

    // Audio codec
    if (meta_keys.contains(QMediaMetaData::AudioCodec))
    {
        static QHash < QMediaFormat::AudioCodec, QString > format_mapper_short;
        static QHash < QMediaFormat::AudioCodec, QString > format_mapper_long;
        if (format_mapper_short.isEmpty())
        {
            format_mapper_short[QMediaFormat::AudioCodec::AAC] = "AAC";
            format_mapper_long[QMediaFormat::AudioCodec::AAC] =
                "Advanced Audio Coding";

            format_mapper_short[QMediaFormat::AudioCodec::AC3] = "AC3";
            format_mapper_long[QMediaFormat::AudioCodec::AC3] =
                "Dolby Digital";

            format_mapper_short[QMediaFormat::AudioCodec::ALAC] = "ALAC";
            format_mapper_long[QMediaFormat::AudioCodec::ALAC] =
                "Apple Lossless Audio Codec";

            format_mapper_short[QMediaFormat::AudioCodec::DolbyTrueHD] =
                "Dolby TrueHD";
            format_mapper_long[QMediaFormat::AudioCodec::DolbyTrueHD] =
                "Dolby TrueHD";

            format_mapper_short[QMediaFormat::AudioCodec::EAC3] = "EAC3";
            format_mapper_long[QMediaFormat::AudioCodec::EAC3] =
                "Dolby Digital Plus";

            format_mapper_short[QMediaFormat::AudioCodec::FLAC] = "FLAC";
            format_mapper_long[QMediaFormat::AudioCodec::FLAC] =
                "Free Lossless Audio Codec";

            format_mapper_short[QMediaFormat::AudioCodec::MP3] = "MP3";
            format_mapper_long[QMediaFormat::AudioCodec::MP3] =
                "MPEG-1/2 Audio Layer III";

            format_mapper_short[QMediaFormat::AudioCodec::Opus] = "Opus";
            format_mapper_long[QMediaFormat::AudioCodec::Opus] =
                "Opus Audio Format";

            format_mapper_short[QMediaFormat::AudioCodec::Vorbis] = "Vorbis";
            format_mapper_long[QMediaFormat::AudioCodec::Vorbis] =
                "OGG Vorbis";

            format_mapper_short[QMediaFormat::AudioCodec::Wave] = "Wave";
            format_mapper_long[QMediaFormat::AudioCodec::Wave] =
                "Waveform Audio File Format";

            format_mapper_short[QMediaFormat::AudioCodec::WMA] = "WMA";
            format_mapper_long[QMediaFormat::AudioCodec::WMA] =
                "Windows Media Audio";


            format_mapper_short[QMediaFormat::AudioCodec::Unspecified] = "";
            format_mapper_long[QMediaFormat::AudioCodec::Unspecified] = "";
        }
        const QMediaFormat::AudioCodec format_id =
            (QMediaFormat::AudioCodec)
            media_metadata.value(QMediaMetaData::AudioCodec).toInt();
        metadata[Metadata_AudioCodec] = format_mapper_short[format_id];
        metadata[Metadata_AudioCodecLong] = format_mapper_long[format_id];
    }

    // Video bitrate
    if (meta_keys.contains(QMediaMetaData::VideoBitRate))
    {
        const int rate =
            media_metadata.value(QMediaMetaData::VideoBitRate).toInt();
        metadata[Metadata_VideoBitRate] = QString::number(rate);
    }

    // Video frame rate
    if (meta_keys.contains(QMediaMetaData::VideoFrameRate))
    {
        const qreal rate =
            media_metadata.value(QMediaMetaData::VideoFrameRate).toReal();
        metadata[Metadata_VideoFrameRate] = QString::number(rate, 'f', 2);
    }

    // Video codec
    if (meta_keys.contains(QMediaMetaData::VideoCodec))
    {
        static QHash < QMediaFormat::VideoCodec, QString > format_mapper_short;
        static QHash < QMediaFormat::VideoCodec, QString > format_mapper_long;
        if (format_mapper_short.isEmpty())
        {
            format_mapper_short[QMediaFormat::VideoCodec::AV1] = "AV1";
            format_mapper_long[QMediaFormat::VideoCodec::AV1] =
                "AOMedia Video 1";

            format_mapper_short[QMediaFormat::VideoCodec::H264] = "H264";
            format_mapper_long[QMediaFormat::VideoCodec::H264] =
                "Advanced Video Coding";

            format_mapper_short[QMediaFormat::VideoCodec::H265] = "H265";
            format_mapper_long[QMediaFormat::VideoCodec::H265] =
                "High Efficiency Video Coding";

            format_mapper_short[QMediaFormat::VideoCodec::MotionJPEG] = "MJPG";
            format_mapper_long[QMediaFormat::VideoCodec::MotionJPEG] =
                "Motion JPEG";

            format_mapper_short[QMediaFormat::VideoCodec::MPEG1] = "MPEG-1";
            format_mapper_long[QMediaFormat::VideoCodec::MPEG1] =
                "MPEG-1";

            format_mapper_short[QMediaFormat::VideoCodec::MPEG2] = "MPEG-2";
            format_mapper_long[QMediaFormat::VideoCodec::MPEG2] =
                "MPEG-2";

            format_mapper_short[QMediaFormat::VideoCodec::MPEG4] = "MPEG-4";
            format_mapper_long[QMediaFormat::VideoCodec::MPEG4] =
                "MPEG-4";

            format_mapper_short[QMediaFormat::VideoCodec::Theora] = "Theora";
            format_mapper_long[QMediaFormat::VideoCodec::Theora] =
                "Theora";

            format_mapper_short[QMediaFormat::VideoCodec::VP8] = "VP8";
            format_mapper_long[QMediaFormat::VideoCodec::VP8] =
                "VP8";

            format_mapper_short[QMediaFormat::VideoCodec::VP9] = "VP9";
            format_mapper_long[QMediaFormat::VideoCodec::VP9] =
                "VP9";

            format_mapper_short[QMediaFormat::VideoCodec::WMV] = "WMV";
            format_mapper_long[QMediaFormat::VideoCodec::WMV] =
                "Windows Media Video";


            format_mapper_short[QMediaFormat::VideoCodec::Unspecified] = "";
            format_mapper_long[QMediaFormat::VideoCodec::Unspecified] = "";
        }
        const QMediaFormat::VideoCodec format_id =
            (QMediaFormat::VideoCodec)
            media_metadata.value(QMediaMetaData::VideoCodec).toInt();
        metadata[Metadata_VideoCodec] = format_mapper_short[format_id];
        metadata[Metadata_VideoCodecLong] = format_mapper_long[format_id];
    }

    CALL_OUT("");
    return metadata;
}


///////////////////////////////////////////////////////////////////////////////
// Get cover art image
QPixmap MediaHelper::GetCoverArt(const QString & mcrFilename)
{
    CALL_IN(QString("mcrFilename=%1")
        .arg(CALL_SHOW(mcrFilename)));

    // == Video-related data
    QMediaPlayer * media = new QMediaPlayer();

    // Wait until source has been read; setSource() returns immediately
    // and does not wait for the media to be loaded.
    QEventLoop event_loop;
    connect (media, &QMediaPlayer::mediaStatusChanged,
        &event_loop, &QEventLoop::quit);
    media -> setSource(QUrl::fromLocalFile(mcrFilename));
    event_loop.exec();

    if (media -> error() != QMediaPlayer::NoError)
    {
        const QString error_text = media -> errorString();
        const QString reason = tr("An error occurred while opening video file "
            "\"%1\": %2")
            .arg(mcrFilename,
                 error_text);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QPixmap();
    }

    // Available keys
    const QMediaMetaData media_metadata = media -> metaData();
    QList < QMediaMetaData::Key > meta_keys = media_metadata.keys();

    // We don't need the media file anymore
    delete media;

    QImage img_cover_art;
    if (meta_keys.contains(QMediaMetaData::CoverArtImage))
    {
        img_cover_art =
            media_metadata.value(QMediaMetaData::CoverArtImage)
                .value<QImage>();
    } else if (meta_keys.contains(QMediaMetaData::ThumbnailImage))
    {
        img_cover_art =
            media_metadata.value(QMediaMetaData::ThumbnailImage)
                .value<QImage>();
    } else
    {
        CALL_OUT("");
        return QPixmap();
    }

    const QPixmap cover_art = QPixmap::fromImage(img_cover_art);
    CALL_OUT("");
    return cover_art;
}

