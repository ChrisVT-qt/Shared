// MediaHelper.h
// Class definition file

/** \file
  * \todo Add Doxygen information
  */

// Just include once
#ifndef MEDIAHELPER_H
#define MEDIAHELPER_H

// Qt includes
#include <QObject>

// Class definition
class MediaHelper :
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Default constructor (never to be called from outside)
    MediaHelper();

public:
    // Destructor
    ~MediaHelper();



    // ============================================================== Functions
public:
    enum Metadata {
        Metadata_AudioBitRate,
        Metadata_AudioCodec,
        Metadata_AudioCodecLong,
        Metadata_Author,
        Metadata_Comments,
        Metadata_Copyright,
        Metadata_DateTime,
        Metadata_Description,
        Metadata_Duration_ms,
        Metadata_Duration_s,
        Metadata_FileDateTime,
        Metadata_FileFormat,
        Metadata_FileSize,
        Metadata_Genres,
        Metadata_Height,
        Metadata_Language,
        Metadata_LocalDirectory,
        Metadata_LocalFilename,
        Metadata_MIMEType,
        Metadata_Publisher,
        Metadata_Title,
        Metadata_Url,
        Metadata_VideoBitRate,
        Metadata_VideoCodec,
        Metadata_VideoCodecLong,
        Metadata_VideoFrameRate,
        Metadata_Width
    };
    static QHash < Metadata, QString > GetMediaMetadata(
        const QString & mcrFilename);

    // Get cover art image
    static QPixmap GetCoverArt(const QString & mcrFilename);
};

#endif
