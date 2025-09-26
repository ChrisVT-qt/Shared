// ExifInfo.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "MessageLogger.h"
#include "ExifInfo.h"

// Qt includes
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QObject>
#include <QRegularExpression>

// Exiv info
#include <exiv2/exiv2.hpp>

// System includes
#include <cmath>

// STL includes
#include <string>



// Debugging
#define DEBUG false
#define DEBUG_LEVEL 1



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor (never to be called from the outside)
ExifInfo::ExifInfo()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
ExifInfo::~ExifInfo()
{
    CALL_IN("");

    // Nothing to do.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Factory
ExifInfo * ExifInfo::CreateExifInfo(const QString & mcrFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW(mcrFilename)));

    ExifInfo * info = nullptr;
    try
    {
        auto image =
            Exiv2::ImageFactory::open(mcrFilename.toLocal8Bit().data());
        image -> readMetadata();
        if (!image -> good())
        {
            // Something didn't work reading the file; probably not an image
            CALL_OUT("");
            return nullptr;
        }

        Exiv2::ExifData & data = image -> exifData();
        if (data.empty())
        {
            // Not really an error, just no EXIF data
            CALL_OUT("");
            return nullptr;
        }

        // Return value
        info = new ExifInfo();

        // Store filename
        info -> m_Filename = mcrFilename;

        // MIME type
        info -> m_MIMEType = image -> mimeType().c_str();

        // Get all tags
        for (auto tag_iterator = data.begin();
             tag_iterator != data.end();
             tag_iterator++)
        {
            const QString group = tag_iterator -> groupName().c_str();
            const QString tag = tag_iterator -> tagName().c_str();
            const QString type = tag_iterator -> typeName();
            const QString value = tag_iterator -> value().toString().c_str();
            info -> m_ExifTypes[group][tag] = type;
            info -> m_ExifData[group][tag] = value;
        }

    }
    catch (Exiv2::Error error)
    {
        // A (fatal) error while reading the file
        const QString reason =
            tr("An error occurred while reading the EXIF info of \"%1\":\n"
                "\t%2")
                .arg(mcrFilename,
                     error.what());
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }

    if (DEBUG &&
        DEBUG_LEVEL >= 2)
    {
        info -> Dump();
    }

    // Register all tags
    info -> RegisterData();

    // No info if there are no data
    if (info -> m_ExifData.isEmpty())
    {
        delete info;
        info = nullptr;
    }

    CALL_OUT("");
    return info;
}



// =========================================================== EXIF Data Access



///////////////////////////////////////////////////////////////////////////////
// Get MIME type
QString ExifInfo::GetMIMEType() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_MIMEType;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a particular tag is defined
bool ExifInfo::HasTag(const QString & mcrGroup, const QString & mcrTag) const
{
    CALL_IN(QString("mcrGroup=%1, mcrTag=%2")
        .arg(CALL_SHOW(mcrGroup),
             CALL_SHOW(mcrTag)));

    const bool exists =
        m_ExifData.contains(mcrGroup) &&
        m_ExifData[mcrGroup].contains(mcrTag);

    CALL_OUT("");
    return exists;
}



///////////////////////////////////////////////////////////////////////////////
// Get a particular tag value
QString ExifInfo::GetValue(const QString & mcrGroup,
    const QString & mcrTag, const QString & mcrDefault) const
{
    CALL_IN(QString("mcrGroup=%1, mcrTag=%2, mcrDefault=%3")
        .arg(CALL_SHOW(mcrGroup),
             CALL_SHOW(mcrTag),
             CALL_SHOW(mcrDefault)));

    QString value = mcrDefault;
    if (m_ExifData.contains(mcrGroup) &&
        m_ExifData[mcrGroup].contains(mcrTag))
    {
        value = m_ExifData[mcrGroup][mcrTag];
    }

    CALL_OUT("");
    return value;
}



// ======= Camera



///////////////////////////////////////////////////////////////////////////////
// Camera Maker
QString ExifInfo::GetCameraMaker() const
{
    CALL_IN("");

    if (!m_ExifData.contains("Image") ||
        !m_ExifData["Image"].contains("Make"))
    {
        // No camera maker given
        CALL_OUT("");
        return QString();
    }

    // Get maker
    QString make = m_ExifData["Image"]["Make"].trimmed();

    // Empty maker
    if (make.isEmpty())
    {
        CALL_OUT("");
        return QString();
    }

    // Post-Process
    if (m_CameraMakerMapper.contains(make))
    {
        // Known camera maker
        make = m_CameraMakerMapper[make];
    } else
    {
        // Check unknown maker
        const QString reason = tr("%1: Unknown camera maker: %2")
            .arg(m_Filename,
                make);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return make;
}



///////////////////////////////////////////////////////////////////////////////
// Camera Model
QString ExifInfo::GetCameraModel() const
{
    CALL_IN("");

    if (!m_ExifData.contains("Image") ||
        !m_ExifData["Image"].contains("Model"))
    {
        // No camera model given
        CALL_OUT("");
        return QString();
    }

    // Get model
    QString model =
        GetCameraMaker() + "." + m_ExifData["Image"]["Model"].trimmed();

    // Empty model
    if (model.isEmpty())
    {
        CALL_OUT("");
        return QString();
    }

    // Post-Process
    if (m_CameraModelMapper.contains(model))
    {
        // Known camera model
        model = m_CameraModelMapper[model];
    } else
    {
        // Check unknown model
        const QString reason = tr("%1: Unknown camera model: %2")
            .arg(m_Filename,
                model);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return model;
}



///////////////////////////////////////////////////////////////////////////////
// Unknown camera model?
bool ExifInfo::IsCameraModelKnown() const
{
    CALL_IN("");

    // Make
    if (!m_ExifData.contains("Image") ||
        !m_ExifData["Image"].contains("Make"))
    {
        // If maker and/or model do not exist in EXIF info, we cannot
        // say we don't know them! ;-) (Rare thing to happen anyway)
        CALL_OUT("");
        return true;
    }
    const QString make = m_ExifData["Image"]["Make"].trimmed();
    if (!m_CameraMakerMapper.contains(make))
    {
        // Unknown camera make
        CALL_OUT("");
        return false;
    }

    // Model
    if (!m_ExifData.contains("Image") ||
        !m_ExifData["Image"].contains("Model"))
    {
        // No camera model given
        CALL_OUT("");
        return false;
    }
    QString model =
        GetCameraMaker() + "." + m_ExifData["Image"]["Model"].trimmed();
    if (!m_CameraModelMapper.contains(model))
    {
        // Unknown camera model
        CALL_OUT("");
        return false;
    }

    // We know maker and model
    CALL_OUT("");
    return true;
}



// ======= Lens



///////////////////////////////////////////////////////////////////////////////
// Lens Maker
QString ExifInfo::GetLensMaker() const
{
    CALL_IN("");

    if (!m_ExifData.contains("Photo") ||
        !m_ExifData["Photo"].contains("LensMake"))
    {
        // No lens maker given
        CALL_OUT("");
        return QString();
    }

    // Get maker
    QString make = m_ExifData["Photo"]["LensMake"].trimmed();

    // Empty maker
    if (make.isEmpty())
    {
        CALL_OUT("");
        return QString();
    }

    // Post-Process
    if (m_LensMakerMapper.contains(make))
    {
        // Known lens maker
        make = m_LensMakerMapper[make];
    } else
    {
        // Check unknown maker
        const QString reason = tr("%1: Unknown lens maker: %2")
            .arg(m_Filename,
                make);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return make;
}



///////////////////////////////////////////////////////////////////////////////
// Lens Model
QString ExifInfo::GetLensModel() const
{
    CALL_IN("");

    if (!m_ExifData.contains("Photo") ||
        !m_ExifData["Photo"].contains("LensModel"))
    {
        // No lens model given
        CALL_OUT("");
        return QString();
    }

    // !!! Could be Canon.0x0095
    // !!! Could be Canon.LensModel
    // !!! Could be CanonCs.Lens
    // !!! Could be CanonCs.LensType

    // Get model
    QString model =
        GetLensMaker() + "." + m_ExifData["Photo"]["LensModel"].trimmed();

    // Empty model
    if (model.isEmpty())
    {
        CALL_OUT("");
        return QString();
    }

    // Post-Process
    if (m_LensModelMapper.contains(model))
    {
        // Known lens model
        model = m_LensModelMapper[model];
    } else
    {
        // Check unknown model
        const QString reason = tr("%1: Unknown lens model: %2")
            .arg(m_Filename,
                model);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return model;
}



///////////////////////////////////////////////////////////////////////////////
// Lens Min Focal Length
QString ExifInfo::GetLensMinFocalLength() const
{
    CALL_IN("");

    // Canon
    if (m_ExifData.contains("CanonCs") &&
        m_ExifData["CanonCs"].contains("Lens"))
    {
        // Format
        static const QRegularExpression format_lens("^([0-9]+) ([0-9]+) 1$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(m_ExifData["CanonCs"]["Lens"]);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     m_ExifData["CanonCs"]["Lens"]);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        const int min_length = match_lens.captured(1).toInt();
        const int max_length = match_lens.captured(2).toInt();
        if (min_length > max_length)
        {
            // Not a problem, but noteworthy
            const QString reason = tr("%1: Min and max focal length of the "
                "lens are incorrectly ordered: \"%2\"")
                .arg(m_Filename,
                     m_ExifData["CanonCs"]["Lens"]);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }
        CALL_OUT("");
        return QString::number(min_length);
    }

    // Others
    QString lens_spec;
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("LensSpecification"))
    {
        lens_spec = m_ExifData["Photo"]["LensSpecification"];
    }

    // Nikon
    if (m_ExifData.contains("Nikon3") &&
        m_ExifData["Nikon3"].contains("Lens"))
    {
        lens_spec = m_ExifData["Nikon3"]["Lens"];
    }

    if (!lens_spec.isEmpty())
    {
        // Format
        // 180/10 2700/10 35/10 63/10
        static const QRegularExpression format_lens(
            "^([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+)$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(lens_spec);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     lens_spec);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        CALL_OUT("");
        return ConvertRational(match_lens.captured(1));
    }

    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No min focal length")
            .arg(m_Filename);
    }

    // Unknown or no info
    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Lens Max Focal Length
QString ExifInfo::GetLensMaxFocalLength() const
{
    CALL_IN("");

    if (m_ExifData.contains("CanonCs") &&
        m_ExifData["CanonCs"].contains("Lens"))
    {
        // Format
        static const QRegularExpression format_lens("^([0-9]+) ([0-9]+) 1$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(m_ExifData["CanonCs"]["Lens"]);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     m_ExifData["CanonCs"]["Lens"]);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        const int min_length = match_lens.captured(1).toInt();
        const int max_length = match_lens.captured(2).toInt();
        if (min_length > max_length)
        {
            // Not a problem, but noteworthy
            const QString reason = tr("%1: Min and max focal length of the "
                "lens are incorrectly ordered: \"%2\"")
                .arg(m_Filename,
                     m_ExifData["CanonCs"]["Lens"]);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }
        CALL_OUT("");
        return QString::number(max_length);
    }

    // Others
    QString lens_spec;
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("LensSpecification"))
    {
        lens_spec = m_ExifData["Photo"]["LensSpecification"];
    }

    // Nikon
    if (m_ExifData.contains("Nikon3") &&
        m_ExifData["Nikon3"].contains("Lens"))
    {
        lens_spec = m_ExifData["Nikon3"]["Lens"];
    }

    if (!lens_spec.isEmpty())
    {
        // Format
        // 180/10 2700/10 35/10 63/10
        static const QRegularExpression format_lens(
            "^([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+)$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(lens_spec);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     lens_spec);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        CALL_OUT("");
        return ConvertRational(match_lens.captured(2));
    }

    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No max focal length")
            .arg(m_Filename);
    }

    // Unknown or no info
    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Lens Min F Stop at minimum focal length
QString ExifInfo::GetLensMinFStopAtMinFocalLength() const
{
    CALL_IN("");

    // Others
    QString lens_spec;
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("LensSpecification"))
    {
        lens_spec = m_ExifData["Photo"]["LensSpecification"];
    }

    // Nikon
    if (m_ExifData.contains("Nikon3") &&
        m_ExifData["Nikon3"].contains("Lens"))
    {
        lens_spec = m_ExifData["Nikon3"]["Lens"];
    }

    if (!lens_spec.isEmpty())
    {
        // Format
        // 180/10 2700/10 35/10 63/10
        static const QRegularExpression format_lens(
            "^([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+)$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(lens_spec);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     lens_spec);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        CALL_OUT("");
        return ConvertRational(match_lens.captured(3));
    }

    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No min f stop at min focal length")
            .arg(m_Filename);
    }

    // Unknown or unavailable info
    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Lens Min F Stop at maximum focal length
QString ExifInfo::GetLensMinFStopAtMaxFocalLength() const
{
    CALL_IN("");

    // Others
    QString lens_spec;
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("LensSpecification"))
    {
        lens_spec = m_ExifData["Photo"]["LensSpecification"];
    }

    // Nikon
    if (m_ExifData.contains("Nikon3") &&
        m_ExifData["Nikon3"].contains("Lens"))
    {
        lens_spec = m_ExifData["Nikon3"]["Lens"];
    }

    if (!lens_spec.isEmpty())
    {
        // Format
        // 180/10 2700/10 35/10 63/10
        static const QRegularExpression format_lens(
            "^([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+) "
            "([0-9]+/[0-9]+)$");
        const QRegularExpressionMatch match_lens =
            format_lens.match(lens_spec);
        if (!match_lens.hasMatch())
        {
            const QString reason =
                tr("%1: Lens information has incorrect format: \"%2\"")
                .arg(m_Filename,
                     lens_spec);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QString();
        }

        CALL_OUT("");
        return ConvertRational(match_lens.captured(4));
    }

    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No min f stop at max focal length")
            .arg(m_Filename);
    }

    // Unknown or unavailable info
    CALL_OUT("");
    return QString();
}



// ======= Exposure



///////////////////////////////////////////////////////////////////////////////
// Exposure: Date & Time
QString ExifInfo::GetExposureDateTime() const
{
    CALL_IN("");

    QString date_time_str;
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("DateTimeOriginal"))
    {
        date_time_str = m_ExifData["Photo"]["DateTimeOriginal"];
    } else if (m_ExifData.contains("Image") &&
        m_ExifData["Image"].contains("DateTime"))
    {
        date_time_str = m_ExifData["Image"]["DateTime"];
    } else
    {
        // No date time information
        if (DEBUG)
        {
            qDebug().noquote() << tr("%1: No exposure date/time.")
                .arg(m_Filename);
        }
        CALL_OUT("");
        return QString();
    }

    // Catch the known invalids
    if (date_time_str.isEmpty() ||
        date_time_str == "0000:00:00 00:00:00" ||
        date_time_str == "    :  :     :  :  " ||
        date_time_str == "                   ")
    {
        // No date time
        CALL_OUT("");
        return QString();
    }

    QDateTime date_time =
        QDateTime::fromString(date_time_str, "yyyy:MM:dd hh:mm:ss");
    if (!date_time.isValid())
    {
        date_time =
            QDateTime::fromString(date_time_str, "yyyy/MM/dd hh:mm:ss");
        if (!date_time.isValid())
        {
            // "2009:09:24 21:16: 9"
            date_time =
                QDateTime::fromString(date_time_str, "yyyy:MM:dd hh:mm: s");
        }
    }

    if (!date_time.isValid())
    {
        // Could not be converted
        const QString reason =
            tr("%1: Date/time does not have the correct format: \"%2\"")
            .arg(m_Filename,
                 m_ExifData["Photo"]["DateTimeOriginal"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    const QString dt_str = date_time.toString("yyyy-MM-dd hh:mm:ss");

    CALL_OUT("");
    return dt_str;
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: Focal Length
QString ExifInfo::GetExposureFocalLength() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("FocalLength"))
    {
        QString length = m_ExifData["Photo"]["FocalLength"];
        if (m_FocalLengthMapper.contains(length))
        {
            length = m_FocalLengthMapper[length];
        } else
        {
            const QString reason = tr("%1: Focal length not in mapper: \"%2\"")
                .arg(m_Filename,
                     length);
            MessageLogger::Message(CALL_METHOD, reason);
        }

        CALL_OUT("");
        return length;
    }

    // !!! Could also be Canon.FocalLength

    // No date time information
    if (DEBUG)
    {
        qDebug().noquote() << tr("%1: No forcal length.")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: F Stop
QString ExifInfo::GetExposureFStop() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("FNumber"))
    {
        // Get F stop
        const QString f_stop = m_ExifData["Photo"]["FNumber"];

        // Check for missing info
        if (f_stop.isEmpty())
        {
            CALL_OUT("");
            return QString();
        }

        // Use mapper
        if (m_FStopMapper.contains(f_stop))
        {
            CALL_OUT("");
            return "f/" + m_FStopMapper[f_stop];
        } else
        {
            // Should be in mapper
            const QString reason = tr("%1: F Stop \"%2\" is not in mapper.")
                .arg(m_Filename,
                     f_stop);
            MessageLogger::Message(CALL_METHOD, reason);

            // Convert rational
            QString rational = ConvertRational(f_stop);
            if (!rational.isEmpty())
            {
                rational = "f/" + rational;
            }

            CALL_OUT("");
            return rational;
        }
    }

    // !!! Could alternatively be Photo.ApertureValue

    if (DEBUG)
    {
        const QString reason = tr("%1: No F Stop")
            .arg(m_Filename);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: Time
QString ExifInfo::GetExposureTime() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("ExposureTime"))
    {
        // Get exposure time
        const QString exposure = m_ExifData["Photo"]["ExposureTime"];

        // Check for missing info
        if (exposure.isEmpty())
        {
            CALL_OUT("");
            return QString();
        }

        // Deal with "1/n" exposure times separately
        if (exposure.startsWith("1/"))
        {
            CALL_OUT("");
            return exposure;
        }

        // Use mapper
        if (m_ExposureTimeMapper.contains(exposure))
        {
            CALL_OUT("");
            return m_ExposureTimeMapper[exposure];
        }

        // Should be in mapper
        const QString reason =
            tr("%1: Exposure time \"%2\" is not in mapper.")
            .arg(m_Filename,
                 exposure);
        MessageLogger::Message(CALL_METHOD, reason);

        // Convert rational
        QString rational = ConvertRational(exposure);
        CALL_OUT("");
        return rational;
    }

    // !!! Could alternatively be Photo.ShutterSpeedValue

    if (DEBUG)
    {
        const QString reason = tr("%1: No exposure time")
            .arg(m_Filename);
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: Bias
QString ExifInfo::GetExposureBias() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("ExposureBiasValue"))
    {
        const QString bias = m_ExifData["Photo"]["ExposureBiasValue"];

        // Check for missing info
        if (bias.isEmpty())
        {
            CALL_OUT("");
            return QString();
        }

        QString rational = ConvertRational(bias);
        if (rational.size() == 1)
        {
            rational += ".00";
        }
        if (rational.toDouble() > 0)
        {
            rational = "+" + rational;
        }

        CALL_OUT("");
        return rational;
    }

    // No exposure bias information
    if (DEBUG)
    {
        qDebug().noquote() << tr("%1: No exposure bias information.")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: ISO Rating
QString ExifInfo::GetISORating() const
{
    CALL_IN("");

    static QSet < QString > acceptable_values;
    if (acceptable_values.isEmpty())
    {
        // Initialize acceptable values
        acceptable_values
            << "16" << "32" << "64" << "125" << "250" << "500" << "1000"
            << "2000"
            << "20" << "40" << "80" << "160" << "320" << "640" << "1280"
            << "2500"
            << "25" << "50" << "100" << "200" << "400" << "800" << "1600"
            << "3200" << "6400" << "12800";
    }

    // Check if we have this information
    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("ISOSpeedRatings"))
    {
        const QString iso = m_ExifData["Photo"]["ISOSpeedRatings"];
        if (!acceptable_values.contains(iso))
        {
            const QString reason = tr("Unknown ISO speed rating: %1 (%2)")
                .arg(iso,
                    m_Filename);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return iso;
        }

        CALL_OUT("");
        return iso;
    }

    // No ISO info
    if (DEBUG)
    {
        qDebug().noquote() << QString("No ISO speed rating (%1)")
            .arg(m_Filename);
    }

    // !!! Could be Nikon3.ISOSpeed
    // !!! Could be CanonCs1.ISOSpeed

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Exposure: Subject Distance
QString ExifInfo::GetExposureSubjectDistance() const
{
    CALL_IN("");

    if (m_ExifData.contains("CanonCs2") &&
        m_ExifData["CanonCs2"].contains("SubjectDistance"))
    {
        const QString dist = m_ExifData["CanonCs2"]["SubjectDistance"];

        // Check for missing info
        if (dist.isEmpty())
        {
            CALL_OUT("");
            return QString();
        }

        // Create subject distance
        CALL_OUT("");
        return QString("%1").arg(dist.toInt() * 0.01, 0, 'f', 2);
    }

    // !!! Could be Photo.SubjectDistance

    // No subject distance info
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No subject distance")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Flash Fired?
QString ExifInfo::GetFlashFired() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("Flash"))
    {
        const QString flash = m_ExifData["Photo"]["Flash"];

        // Check for missing info
        if (flash.isEmpty())
        {
            CALL_OUT("");
            return QString();
        }

        // This field is made up of bits:
        //  Bit 0: Flash fired (0: No, 1: Yes)
        //  Bit 1,2: Flash return
        //  Bit 3,4: Flash mode
        //  Bit 5: Flash function
        //  Bit 6: Red-eye reduction
        // We use only bit 0 here.

        // Check value
        CALL_OUT("");
        return (flash.toInt() & 1) ? "yes" : "no";
    }

    // No flash info
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No flash information")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();

}



///////////////////////////////////////////////////////////////////////////////
// Flash Bias
QString ExifInfo::GetFlashBias() const
{
    CALL_IN("");

    // No flash bias
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No flash bias")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Picture Number
QString ExifInfo::GetPictureNumber() const
{
    CALL_IN("");

    // No picture number
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No picture number")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



// ======= Image




///////////////////////////////////////////////////////////////////////////////
// Width of real picture
QString ExifInfo::GetRealPictureWidth() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("PixelXDimension"))
    {
        const QString width = m_ExifData["Photo"]["PixelXDimension"];
        CALL_OUT("");
        return width;
    }

    // No picture width
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No picture width")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Height of real picture
QString ExifInfo::GetRealPictureHeight() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("PixelYDimension"))
    {
        const QString height = m_ExifData["Photo"]["PixelYDimension"];
        CALL_OUT("");
        return height;
    }

    // No picture height
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No picture height")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Orientation
int ExifInfo::GetOrientation() const
{
    CALL_IN("");

    if (m_ExifData.contains("Image") &&
        m_ExifData["Image"].contains("Orientation"))
    {
        const int orientation = m_ExifData["Image"]["Orientation"].toInt();
        switch (orientation)
        {
        case 1:
            // Falls thru
        case 2:
            CALL_OUT("");
            return 0;

        case 3:
            // Falls thru
        case 4:
            CALL_OUT("");
            return 180;

        case 5:
            // Falls thru
        case 6:
            CALL_OUT("");
            return 90;

        case 7:
            // Falls thru
        case 8:
            CALL_OUT("");
            return 270;

        default:
        {
            const QString reason = tr("%1: Unknown orientation %2")
                .arg(m_Filename,
                    orientation);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT("");
            return -1;
        }
        }
    }

    // No orientation found
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No orientation")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return -1;
}



///////////////////////////////////////////////////////////////////////////////
// Subject area
QString ExifInfo::GetSubjectArea() const
{
    CALL_IN("");

    // No subject area found
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No subject area")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Software
QString ExifInfo::GetSoftware() const
{
    CALL_IN("");

    if (m_ExifData.contains("Image") &&
        m_ExifData["Image"].contains("Software"))
    {
        const QString software = m_ExifData["Image"]["Software"];
        CALL_OUT("");
        return software;
    }

    // No software information
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No software information")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



// ======= Owner/Copyright



///////////////////////////////////////////////////////////////////////////////
// Owner
QString ExifInfo::GetOwner() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("CameraOwnerName"))
    {
        CALL_OUT("");
        return m_ExifData["Photo"]["CameraOwnerName"];
    }

    // !!! Could also be Photo.OwnerName

    // No owner
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No owner information")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



///////////////////////////////////////////////////////////////////////////////
// Camera serial number
QString ExifInfo::GetCameraSerialNumber() const
{
    CALL_IN("");

    if (m_ExifData.contains("Photo") &&
        m_ExifData["Photo"].contains("BodySerialNumber"))
    {
        const QString serial = m_ExifData["Photo"]["BodySerialNumber"];
        CALL_OUT("");
        return serial;
    }

    // No serial number
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No serial number")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return QString();
}



// ======= GPS Position



///////////////////////////////////////////////////////////////////////////////
// Latitude
double ExifInfo::GetGPSLatitude() const
{
    CALL_IN("");

    // Check if tags exist
    if (!m_ExifData.contains("GPSInfo") ||
        !m_ExifData["GPSInfo"].contains("GPSLatitude") ||
        !m_ExifData["GPSInfo"].contains("GPSLatitudeRef"))
    {
        // Nope.
        if (DEBUG)
        {
            qDebug().noquote() << QString("%1: No GPS latitude")
                .arg(m_Filename);
        }

        CALL_OUT("");
        return NAN;
    }

    // Parse it.
    static const QRegularExpression parse(
        "^([0-9]+)/([0-9]+) ([0-9]+)/([0-9]+) ([0-9]+)/([0-9]+)$");
    QRegularExpressionMatch parse_match =
        parse.match(m_ExifData["GPSInfo"]["GPSLatitude"]);
    if (!parse_match.hasMatch())
    {
        const QString reason =
            tr("%1: Unknown GPS Latitude format: \"%2\". Ignored.")
                .arg(m_Filename,
                    m_ExifData["GPSInfo"]["GPSLatitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Matches
    const int deg_ctr = parse_match.captured(1).toInt();
    const int deg_den = parse_match.captured(2).toInt();
    const int min_ctr = parse_match.captured(3).toInt();
    const int min_den = parse_match.captured(4).toInt();
    const int sec_ctr = parse_match.captured(5).toInt();
    const int sec_den = parse_match.captured(6).toInt();

    // Check for zero denominators
    // ("0/0" is okay for the last group)
    if (deg_den == 0 ||
        min_den == 0 ||
        (sec_den == 0 && sec_ctr != 0))
    {
        const QString reason =
            tr("%1: Zero denominators in GPS Latitude: \"%2\". Ignored.")
                .arg(m_Filename,
                    m_ExifData["GPSInfo"]["GPSLatitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Convert
    const double degrees = deg_ctr * 1. / deg_den;
    const double minutes = min_ctr * 1. / min_den;;
    const double seconds = (sec_den == 0 ? 0. : sec_ctr * 1./ sec_den);
    const double deg = degrees + minutes/60 + seconds/3600;
    const QString ns = m_ExifData["GPSInfo"]["GPSLatitudeRef"];
    if (ns == "S")
    {
        CALL_OUT("");
        return -deg;
    } else if (ns == "N")
    {
        CALL_OUT("");
        return deg;
    }

    const QString reason = tr("%1: Invalid hemisphere (N/S): \"%2\".")
        .arg(m_Filename,
            ns);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Longitude
double ExifInfo::GetGPSLongitude() const
{
    CALL_IN("");

    // Check if tags exist
    if (!m_ExifData.contains("GPSInfo") ||
        !m_ExifData["GPSInfo"].contains("GPSLongitude") ||
        !m_ExifData["GPSInfo"].contains("GPSLongitudeRef"))
    {
        // Nope.
        if (DEBUG)
        {
            qDebug().noquote() << QString("%1: No GPS longitude")
                .arg(m_Filename);
        }

        CALL_OUT("");
        return NAN;
    }

    // Parse it.
    static const QRegularExpression parse(
        "^([0-9]+)/([0-9]+) ([0-9]+)/([0-9]+) ([0-9]+)/([0-9]+)$");
    QRegularExpressionMatch parse_match =
        parse.match(m_ExifData["GPSInfo"]["GPSLongitude"]);
    if (!parse_match.hasMatch())
    {
        const QString reason =
            tr("%1: Unknown GPS longitude format: \"%2\". Ignored.")
                .arg(m_Filename,
                    m_ExifData["GPSInfo"]["GPSLongitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Matches
    const int deg_ctr = parse_match.captured(1).toInt();
    const int deg_den = parse_match.captured(2).toInt();
    const int min_ctr = parse_match.captured(3).toInt();
    const int min_den = parse_match.captured(4).toInt();
    const int sec_ctr = parse_match.captured(5).toInt();
    const int sec_den = parse_match.captured(6).toInt();

    // Check for zero denominators
    // ("0/0" is okay for the last group)
    if (deg_den == 0 ||
        min_den == 0 ||
        (sec_den == 0 && sec_ctr != 0))
    {
        const QString reason =
            tr("%1: Zero denominators in GPS Longitude: \"%2\". Ignored.")
                .arg(m_Filename,
                    m_ExifData["GPSInfo"]["GPSLongitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Convert
    const double degrees = deg_ctr * 1. / deg_den;
    const double minutes = min_ctr * 1. / min_den;;
    const double seconds = (sec_den == 0 ? 0. : sec_ctr * 1./ sec_den);
    const double deg = degrees + minutes/60 + seconds/3600;
    const QString ew = m_ExifData["GPSInfo"]["GPSLongitudeRef"];
    if (ew == "W")
    {
        CALL_OUT("");
        return -deg;
    } else if (ew == "E")
    {
        CALL_OUT("");
        return deg;
    }

    const QString reason = tr("%1: Invalid hemisphere (E/W): \"%2\".")
        .arg(m_Filename,
            ew);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Elevation
double ExifInfo::GetGPSElevation() const
{
    CALL_IN("");

    // Check if tags exist
    if (!m_ExifData.contains("GPSInfo") ||
        !m_ExifData["GPSInfo"].contains("GPSAltitude") ||
        !m_ExifData["GPSInfo"].contains("GPSAltitudeRef"))
    {
        // Nope.
        if (DEBUG)
        {
            qDebug().noquote() << QString("%1: No GPS elevation")
                .arg(m_Filename);
        }

        CALL_OUT("");
        return NAN;
    }

    // Parse it.
    static const QRegularExpression parse("^([0-9]+)/([0-9]+)$");
    QRegularExpressionMatch parse_match =
        parse.match(m_ExifData["GPSInfo"]["GPSAltitude"]);
    if (!parse_match.hasMatch())
    {
        const QString reason =
            tr("Unknown GPS altitude format: \"%1\". Ignored.")
                .arg(m_ExifData["GPSInfo"]["GPSAltitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Check for zero denominators
    if (parse_match.captured(2).toInt() == 0)
    {
        const QString reason =
            tr("Zero denominators in GPS altitude: \"%1\". Ignored.")
                .arg(m_ExifData["GPSInfo"]["GPSAltitude"]);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return NAN;
    }

    // Convert
    const double elevation = parse_match.captured(1).toDouble() /
        parse_match.captured(2).toDouble();
    if (m_ExifData["GPSInfo"]["GPSAltitudeRef"].toInt() == 1)
    {
        CALL_OUT("");
        return -elevation;
    } else
    {
        CALL_OUT("");
        return elevation;
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Direction
double ExifInfo::GetGPSDirection() const
{
    CALL_IN("");

    // No GPS direction
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No GPS direction")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Temperature
double ExifInfo::GetCameraTemperature() const
{
    CALL_IN("");

    // This is acutally the DIGIC processor temperature (in F)
    if (m_ExifData.contains("CanonSi") &&
        m_ExifData["CanonSi"].contains("CameraTemperature"))
    {
        const double temperature_f =
            m_ExifData["CanonSi"]["CameraTemperature"].toInt();
        const double temperature_c = (temperature_f - 32.) * 5./9.;
        CALL_OUT("");
        return temperature_c;
    }

    // No temperature
    if (DEBUG)
    {
        qDebug().noquote() << QString("%1: No temperature")
            .arg(m_Filename);
    }

    CALL_OUT("");
    return NAN;
}



///////////////////////////////////////////////////////////////////////////////
// Convert rational to string
QString ExifInfo::ConvertRational(const QString & mcrValue) const
{
    CALL_IN(QString("mcrValue=%1")
        .arg(CALL_SHOW(mcrValue)));

    static const QRegularExpression format_rational(
        "^((\\+|-)?[0-9]+)/([0-9]+)$");
    const QRegularExpressionMatch match_rational =
        format_rational.match(mcrValue);
    if (!match_rational.hasMatch())
    {
        // Not a rational
        const QString reason = tr("%1 is not valid rational value.")
            .arg(mcrValue);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    const int counter = match_rational.captured(1).toInt();
    const int denominator = match_rational.captured(3).toInt();

    // Denominator cannot be zero
    if (denominator == 0)
    {
        const QString reason = tr("%1 has a zero denominator.")
            .arg(mcrValue);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QString();
    }

    // Convert
    if (counter % denominator == 0)
    {
        // Integer result
        CALL_OUT("");
        return QString("%1").arg(counter / denominator);
    } else
    {
        // Decimal result
        CALL_OUT("");
        return QString("%1").arg(counter * 1. / denominator, 0, 'f', 1);
    }

    // We never get here
}



// ============================================================== Known Cameras



///////////////////////////////////////////////////////////////////////////////
// Initialize all mappers
bool ExifInfo::InitializeMappers()
{
    CALL_IN("");

    Init_CameraMakerMapper();
    Init_CameraModelMapper();
    Init_LensMakerMapper();
    Init_LensModelMapper();
    Init_FStopMapper();
    Init_FocalLengthMapper();
    Init_ExposureTimeMapper();

    CALL_OUT("");
    return true;
}

const bool ExifInfo::m_MappersInitialized = ExifInfo::InitializeMappers();



///////////////////////////////////////////////////////////////////////////////
// Camera maker mapper
void ExifInfo::Init_CameraMakerMapper()
{
    CALL_IN("");

    // Add makers
    m_CameraMakerMapper["Apple"] = "Apple";
    m_CameraMakerMapper["Canon"] = "Canon";
    m_CameraMakerMapper["CASIO"] = "Casio";
    m_CameraMakerMapper["CASIO COMPUTER CO.,LTD"] = "Casio";
    m_CameraMakerMapper["CASIO COMPUTER CO.,LTD."] = "Casio";
    m_CameraMakerMapper["CONCORD  OPTICAL CO,LTD"] = "Concord";
    m_CameraMakerMapper["Cruse Scanner"] = "Cruse Scanner";
    m_CameraMakerMapper["EASTMAN KODAK COMPANY"] = "Kodak";
    m_CameraMakerMapper["Eastman Kodak Company"] = "Kodak";
    m_CameraMakerMapper["EPSON"] = "Epson";
    m_CameraMakerMapper["FUJIFILM"] = "Fujifilm";
    m_CameraMakerMapper["FUJIFILM Corporation"] = "Fujifilm";
    m_CameraMakerMapper["Gateway"] = "Gateway";
    m_CameraMakerMapper["General Imaging Co."] = "General Imaging";
    m_CameraMakerMapper["Google"] = "Google";
    m_CameraMakerMapper["Hasselblad"] = "Hasselblad";
    m_CameraMakerMapper["Hewlett-Packard"] = "Hewlett-Packard";
    m_CameraMakerMapper["Hewlett-Packard"] = "Hewlett-Packard";
    m_CameraMakerMapper["HP"] = "Hewlett-Packard";
    m_CameraMakerMapper["HTC"] = "HTC";
    m_CameraMakerMapper["HUAWEI"] = "Huawei";
    m_CameraMakerMapper["KONICA"] = "Konica";
    m_CameraMakerMapper["KONICA MINOLTA"] = "Konica Minolta";
    m_CameraMakerMapper["KONICA MINOLTA"] = "Konica Minolta";
    m_CameraMakerMapper["Konica Minolta Camera, Inc."] = "Konica Minolta";
    m_CameraMakerMapper["KYOCERA"] = "Kyocera";
    m_CameraMakerMapper["LEICA"] = "Leica";
    m_CameraMakerMapper["Leica Camera AG"] = "Leica";
    m_CameraMakerMapper["LG Electronics"] = "LG Electronics";
    m_CameraMakerMapper["Microsoft"] = "Microsoft";
    m_CameraMakerMapper["MINOLTA CO.,LTD"] = "Minolta";
    m_CameraMakerMapper["Minolta Co., Ltd"] = "Minolta";
    m_CameraMakerMapper["Minolta Co., Ltd."] = "Minolta";
    m_CameraMakerMapper["Motorola"] = "Motorola";
    m_CameraMakerMapper["motorola"] = "Motorola";
    m_CameraMakerMapper["NIKON"] = "Nikon";
    m_CameraMakerMapper["NIKON CORPORATION"] = "Nikon";
    m_CameraMakerMapper["Nikon Inc.."] = "Nikon";
    m_CameraMakerMapper["Nokia"] = "Nokia";
    m_CameraMakerMapper["OLYMPUS CORPORATION"] = "Olympus";
    m_CameraMakerMapper["OLYMPUS CORPORATION"] = "Olympus";
    m_CameraMakerMapper["OLYMPUS IMAGING CORP."] = "Olympus";
    m_CameraMakerMapper["OLYMPUS IMAGING CORP."] = "Olympus";
    m_CameraMakerMapper["OLYMPUS OPTICAL CO.,LTD"] = "Olympus";
    m_CameraMakerMapper["PENTAX"] = "Pentax";
    m_CameraMakerMapper["PENTAX"] = "Pentax";
    m_CameraMakerMapper["PENTAX Corporation"] = "Pentax";
    m_CameraMakerMapper["PENTAX Corporation"] = "Pentax";
    m_CameraMakerMapper["Panasonic"] = "Panasonic";
    m_CameraMakerMapper["Phase One"] = "Phase One";
    m_CameraMakerMapper["Polaroid"] = "Polaroid";
    m_CameraMakerMapper["RICOH"] = "Ricoh";
    m_CameraMakerMapper["SAMSUNG"] = "Samsung";
    m_CameraMakerMapper["samsung"] = "Samsung";
    m_CameraMakerMapper["Samsung Techwin"] = "Samsung";
    m_CameraMakerMapper["SAMSUNG TECHWIN CO., LTD."] = "Samsung";
    m_CameraMakerMapper["SANYO Electric Co.,Ltd."] = "Sanyo";
    m_CameraMakerMapper["SONY"] = "Sony";
    m_CameraMakerMapper["Sony"] = "Sony";
    m_CameraMakerMapper["SONY"] = "Sony";
    m_CameraMakerMapper["Sony Ericsson"] = "Sony Ericsson";
    m_CameraMakerMapper["Supra"] = "Supra";
    m_CameraMakerMapper["TOSHIBA"] = "Toshiba";
    m_CameraMakerMapper["Xiaomi"] = "Xiaomi";
    m_CameraMakerMapper["ZTE"] = "ZTE";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Camera model mapper
void ExifInfo::Init_CameraModelMapper()
{
    CALL_IN("");

    // No model
    m_CameraModelMapper["."] = "";

    // Apple
    m_CameraModelMapper["Apple.iPad"] = "iPad";
    m_CameraModelMapper["Apple.iPad mini"] = "iPad mini";
    m_CameraModelMapper["Apple.iPhone 3GS"] = "iPhone 3GS";
    m_CameraModelMapper["Apple.iPhone 4"] = "iPhone 4";
    m_CameraModelMapper["Apple.iPhone 4S"] = "iPhone 4S";
    m_CameraModelMapper["Apple.iPhone 5"] = "iPhone 5";
    m_CameraModelMapper["Apple.iPhone 5s"] = "iPhone 5s";
    m_CameraModelMapper["Apple.iPhone 6"] = "iPhone 6";
    m_CameraModelMapper["Apple.iPhone 6s"] = "iPhone 6s";
    m_CameraModelMapper["Apple.iPhone 6s Plus"] = "iPhone 6s Plus";
    m_CameraModelMapper["Apple.iPhone 7"] = "iPhone 7";
    m_CameraModelMapper["Apple.iPhone 8"] = "iPhone 8";
    m_CameraModelMapper["Apple.iPhone 11"] = "iPhone 11";
    m_CameraModelMapper["Apple.iPhone 11 Pro Max"] = "iPhone 11 Pro Max";
    m_CameraModelMapper["Apple.iPhone 12"] = "iPhone 12";
    m_CameraModelMapper["Apple.iPhone 12 Pro Max"] = "iPhone 12 Pro Max";
    m_CameraModelMapper["Apple.iPhone 15 Pro Max"] = "iPhone 15 Pro Max";
    m_CameraModelMapper["Apple.iPhone 16e"] = "iPhone 16e";
    m_CameraModelMapper["Apple.iPhone SE (2nd generation)"] = "iPhone SE 2";
    m_CameraModelMapper["Apple.iPhone SE (3rd generation)"] = "iPhone SE 3";

    // Canon
    m_CameraModelMapper["Canon.CanoScan 5600F"] = "CanoScan 5600F";
    m_CameraModelMapper["Canon.CanoScan LiDE 25"] = "CanoScan LiDE 25";
    m_CameraModelMapper["Canon.CanoScan LiDE 100"] = "CanoScan LiDE 100";
    m_CameraModelMapper["Canon.CanoScan LiDE 120"] = "CanoScan LiDE 120";
    m_CameraModelMapper["Canon.CanoScan LiDE 400"] = "CanoScan LiDE 400";
    m_CameraModelMapper["Canon.CanoScan LiDE 700F"] = "CanoScan LiDE 700F";

    m_CameraModelMapper["Canon.Canon DC20"] = "DC20";

    m_CameraModelMapper["Canon.Canon DIGITAL IXUS 110 IS"] =
        "Digital Ixus 110 IS";
    m_CameraModelMapper["Canon.Canon DIGITAL IXUS v2"] = "Digital Ixus V2";
    m_CameraModelMapper["Canon.Canon IXUS 240 HS"] = "Ixus 240 HS";

    m_CameraModelMapper["Canon.Canon EOS-1D Mark II"] = "EOS 1D Mark II";
    m_CameraModelMapper["Canon.Canon EOS-1DS"] = "EOS 1Ds";
    m_CameraModelMapper["Canon.Canon EOS-1Ds Mark II"] = "EOS 1Ds Mark II";
    m_CameraModelMapper["Canon.Canon EOS-1Ds Mark III"] = "EOS 1Ds Mark III";
    m_CameraModelMapper["Canon.Canon EOS-1D Mark III"] = "EOS 1D Mark III";
    m_CameraModelMapper["Canon.Canon EOS-1D Mark IV"] = "EOS 1D Mark IV";
    m_CameraModelMapper["Canon.Canon EOS-1D X"] = "EOS 1D X";
    m_CameraModelMapper["Canon.Canon EOS 5D"] = "EOS 5D";
    m_CameraModelMapper["Canon.Canon EOS 5D Mark II"] = "EOS 5D Mark II";
    m_CameraModelMapper["Canon.Canon EOS 5D Mark III"] = "EOS 5D Mark III";
    m_CameraModelMapper["Canon.Canon EOS 5D Mark IV"] = "EOS 5D Mark IV";
    m_CameraModelMapper["Canon.Canon EOS 6D"] = "EOS 6D";
    m_CameraModelMapper["Canon.Canon EOS 7D"] = "EOS 7D";
    m_CameraModelMapper["Canon.Canon EOS 7D Mark II"] = "EOS 7D Mark II";
    m_CameraModelMapper["Canon.Canon EOS Rebel T6s"] = "EOS Rebel T6s";
    m_CameraModelMapper["Canon.Canon EOS 10D"] = "EOS 10D";
    m_CameraModelMapper["Canon.Canon EOS 20D"] = "EOS 20D";
    m_CameraModelMapper["Canon.Canon EOS 30D"] = "EOS 30D";
    m_CameraModelMapper["Canon.Canon EOS 40D"] = "EOS 40D";
    m_CameraModelMapper["Canon.Canon EOS 50D"] = "EOS 50D";
    m_CameraModelMapper["Canon.Canon EOS 60D"] = "EOS 60D";
    m_CameraModelMapper["Canon.Canon EOS 70D"] = "EOS 70D";
    m_CameraModelMapper["Canon.Canon EOS 77D"] = "EOS 77D";
    m_CameraModelMapper["Canon.Canon EOS 80D"] = "EOS 80D";
    m_CameraModelMapper["Canon.Canon EOS 90D"] = "EOS 90D";
    m_CameraModelMapper["Canon.Canon EOS 300D DIGITAL"] = "EOS 300D";
    m_CameraModelMapper["Canon.Canon EOS 350D DIGITAL"] = "EOS 350D";
    m_CameraModelMapper["Canon.Canon EOS 400D DIGITAL"] = "EOS 400D";
    m_CameraModelMapper["Canon.Canon EOS 450D"] = "EOS 450D";
    m_CameraModelMapper["Canon.Canon EOS 500D"] = "EOS 500D";
    m_CameraModelMapper["Canon.Canon EOS 550D"] = "EOS 550D";
    m_CameraModelMapper["Canon.Canon EOS 600D"] = "EOS 600D";
    m_CameraModelMapper["Canon.Canon EOS 700D"] = "EOS 700D";
    m_CameraModelMapper["Canon.Canon EOS 760D"] = "EOS 760D";
    m_CameraModelMapper["Canon.Canon EOS 1000D"] = "EOS 1000D";
    m_CameraModelMapper["Canon.Canon EOS 1100D"] = "EOS 1100D";
    m_CameraModelMapper["Canon.Canon EOS 1200D"] = "EOS 1200D";
    m_CameraModelMapper["Canon.Canon EOS D30"] = "EOS D30";
    m_CameraModelMapper["Canon.Canon EOS DIGITAL REBEL"] = "EOS Digital Rebel";
    m_CameraModelMapper["Canon.Canon EOS DIGITAL REBEL XS"] =
        "EOS Digital Rebel XS";
    m_CameraModelMapper["Canon.Canon EOS DIGITAL REBEL XSi"] =
        "EOS Digital Rebel XSi";
    m_CameraModelMapper["Canon.Canon EOS DIGITAL REBEL XT"] =
        "EOS Digital Rebel XT";
    m_CameraModelMapper["Canon.Canon EOS DIGITAL REBEL XTi"] =
        "EOS Digital Rebel XTi";
    m_CameraModelMapper["Canon.Canon EOS Kiss X4"] =
        "EOS Kiss X4"; // Same as 550D
    m_CameraModelMapper["Canon.Canon EOS M50"] = "EOS M50";
    m_CameraModelMapper["Canon.Canon EOS M50m2"] = "EOS M50 Mark II";
    m_CameraModelMapper["Canon.Canon EOS R6"] = "EOS R6";
    m_CameraModelMapper["Canon.Canon EOS R6m2"] = "EOS R6 Mark II";
    m_CameraModelMapper["Canon.Canon EOS REBEL T1i"] = "EOS Digital Rebel T1i";
    m_CameraModelMapper["Canon.Canon EOS REBEL T3"] = "EOS Digital Rebel T3";
    m_CameraModelMapper["Canon.Canon EOS REBEL T2i"] = "EOS Digital Rebel T2i";
    m_CameraModelMapper["Canon.Canon EOS REBEL T3i"] = "EOS Digital Rebel T3i";

    m_CameraModelMapper["Canon.Canon MF3200 Series"] = "imageCLASS MF3200";
    m_CameraModelMapper["Canon.Canon MG6200 series"] = "imageCLASS MG6200";
    m_CameraModelMapper["Canon.MP250 series"] = "PIXMA MP250";
    m_CameraModelMapper["Canon.MP610 series"] = "PIXMA MP610";
    m_CameraModelMapper["Canon.MP560 series"] = "PIXMA MP560";
    m_CameraModelMapper["Canon.MX330 series"] = "PIXMA MX330";
    m_CameraModelMapper["Canon.MX450 series"] = "PIXMA MX450";
    m_CameraModelMapper["Canon.MX870 series"] = "PIXMA MX870";

    m_CameraModelMapper["Canon.Canon PowerShot A60"] = "PowerShot A60";
    m_CameraModelMapper["Canon.Canon PowerShot A70"] = "PowerShot A70";
    m_CameraModelMapper["Canon.Canon PowerShot A75"] = "PowerShot A75";
    m_CameraModelMapper["Canon.Canon PowerShot A80"] = "PowerShot A80";
    m_CameraModelMapper["Canon.Canon PowerShot A95"] = "PowerShot A95";
    m_CameraModelMapper["Canon.Canon PowerShot A400"] = "PowerShot A400";
    m_CameraModelMapper["Canon.Canon PowerShot A410"] = "PowerShot A410";
    m_CameraModelMapper["Canon.Canon PowerShot A430"] = "PowerShot A430";
    m_CameraModelMapper["Canon.Canon PowerShot A510"] = "PowerShot A510";
    m_CameraModelMapper["Canon.Canon PowerShot A520"] = "PowerShot A520";
    m_CameraModelMapper["Canon.Canon PowerShot A530"] = "PowerShot A530";
    m_CameraModelMapper["Canon.Canon PowerShot A550"] = "PowerShot A550";
    m_CameraModelMapper["Canon.Canon PowerShot A560"] = "PowerShot A560";
    m_CameraModelMapper["Canon.Canon PowerShot A570 IS"] = "PowerShot A570 IS";
    m_CameraModelMapper["Canon.Canon PowerShot A610"] = "PowerShot A610";
    m_CameraModelMapper["Canon.Canon PowerShot A620"] = "PowerShot A620";
    m_CameraModelMapper["Canon.Canon PowerShot A650 IS"] = "PowerShot A650 IS";
    m_CameraModelMapper["Canon.Canon PowerShot A720 IS"] = "PowerShot A720 IS";
    m_CameraModelMapper["Canon.Canon PowerShot A800"] = "PowerShot A800";
    m_CameraModelMapper["Canon.Canon PowerShot A3100 IS"] =
        "PowerShot A3100 IS";
    m_CameraModelMapper["Canon.Canon PowerShot A4000 IS"] =
        "PowerShot A4000 IS";
    m_CameraModelMapper["Canon.Canon PowerShot G2"] = "PowerShot G2";
    m_CameraModelMapper["Canon.Canon PowerShot G6"] = "PowerShot G6";
    m_CameraModelMapper["Canon.Canon PowerShot G9"] = "PowerShot G9";
    m_CameraModelMapper["Canon.Canon PowerShot G11"] = "PowerShot G11";
    m_CameraModelMapper["Canon.Canon PowerShot G12"] = "PowerShot G12";
    m_CameraModelMapper["Canon.Canon PowerShot S1 IS"] = "PowerShot S1 IS";
    m_CameraModelMapper["Canon.Canon PowerShot S2 IS"] = "PowerShot S2 IS";
    m_CameraModelMapper["Canon.Canon PowerShot S3 IS"] = "PowerShot S3 IS";
    m_CameraModelMapper["Canon.Canon PowerShot S5 IS"] = "PowerShot S5 IS";
    m_CameraModelMapper["Canon.Canon PowerShot S30"] = "PowerShot S30";
    m_CameraModelMapper["Canon.Canon PowerShot S50"] = "PowerShot S50";
    m_CameraModelMapper["Canon.Canon PowerShot S60"] = "PowerShot S60";
    m_CameraModelMapper["Canon.Canon PowerShot S90"] = "PowerShot S90";
    m_CameraModelMapper["Canon.Canon PowerShot SD780 IS"] =
        "PowerShot SD780 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SD800 IS"] =
        "PowerShot SD800 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SD900"] = "PowerShot SD900";
    m_CameraModelMapper["Canon.Canon PowerShot SD1400 IS"] =
        "PowerShot SD1400 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX10 IS"] = "PowerShot SX10 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX20 IS"] = "PowerShot SX20 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX100 IS"] =
        "PowerShot SX100 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX120 IS"] =
        "PowerShot SX120 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX130 IS"] =
        "PowerShot SX130 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX160 IS"] =
        "PowerShot SX160 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX200 IS"] =
        "PowerShot SX200 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX210 IS"] =
        "PowerShot SX210 IS";
    m_CameraModelMapper["Canon.Canon PowerShot SX260 HS"] =
        "PowerShot SX260 HS";

    // Casio
    m_CameraModelMapper["Casio.EX-FH20"] = "Exilim EX-FH20";
    m_CameraModelMapper["Casio.EX-P505"] = "Exilim EX-P505";
    m_CameraModelMapper["Casio.EX-S600"] = "Exilim EX-S600";
    m_CameraModelMapper["Casio.EX-Z15"] = "Exilim EX-Z15";
    m_CameraModelMapper["Casio.EX-Z29"] = "Exilim EX-Z29";
    m_CameraModelMapper["Casio.EX-Z33"] = "Exilim EX-Z33";
    m_CameraModelMapper["Casio.EX-Z40"] = "Exilim EX-Z40";
    m_CameraModelMapper["Casio.EX-Z120"] = "Exilim EX-Z120";
    m_CameraModelMapper["Casio.EX-Z400"] = "Exilim EX-Z400";
    m_CameraModelMapper["Casio.QV-3500EX"] = "Exilim QV-3500EX";

    // Concord
    m_CameraModelMapper["Concord.41Z0"] = "41Z0";

    // Cruse Scanner
    m_CameraModelMapper["Cruse Scanner."] = "";

    // Epson
    m_CameraModelMapper["Epson.Expression 12000XL"] = "Expression 12000XL";

    // Google
    m_CameraModelMapper["Google.Pixel 6 Pro"] = "Pixel 6 Pro";

    // Kodak
    m_CameraModelMapper["Kodak.KODAK EASYSHARE C182 Digital Camera"] =
        "EasyShare C182";
    m_CameraModelMapper["Kodak.KODAK CX4200 DIGITAL CAMERA"] =
        "EasyShare CX4200";
    m_CameraModelMapper["Kodak.KODAK CX6330 ZOOM DIGITAL CAMERA"] =
        "EasyShare CX6330 Zoom";
    m_CameraModelMapper["Kodak.KODAK CX7330 ZOOM DIGITAL CAMERA"] =
        "EasyShare CX7330 Zoom";
    m_CameraModelMapper["Kodak.KODAK CX7530 ZOOM DIGITAL CAMERA"] =
        "EasyShare CX7530 Zoom";
    m_CameraModelMapper["Kodak.KODAK DX4330 DIGITAL CAMERA"] =
        "EasyShare DX4330";
    m_CameraModelMapper["Kodak.KODAK DX6490 ZOOM DIGITAL CAMERA"] =
        "EasyShare DX6490 Zoom";
    m_CameraModelMapper["Kodak.KODAK DX7440 ZOOM DIGITAL CAMERA"] =
        "EasyShare DX7440 Zoom";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE C300 DIGITAL CAMERA"] =
        "EasyShare C300";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE C743 ZOOM DIGITAL CAMERA"] =
        "EasyShare C743 Zoom";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE C813 ZOOM DIGITAL CAMERA"] =
        "EasyShare C813 Zoom";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE Camera, C1450"] =
        "EasyShare C1450";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE M340 Digital Camera"] =
        "EasyShare M340";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE M1063 DIGITAL CAMERA"] =
        "EasyShare M1063";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE V1003 ZOOM DIGITAL CAMERA"] =
        "EasyShare V1003 Zoom";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE V1073 DIGITAL CAMERA"] =
        "EasyShare V1073";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE Z710 ZOOM DIGITAL CAMERA"] =
        "EasyShare Z710 Zoom";
    m_CameraModelMapper["Kodak.KODAK Z712 IS ZOOM DIGITAL CAMERA"] =
        "EasyShare Z712 IS Zoom";
    m_CameraModelMapper["Kodak.KODAK Z760 ZOOM DIGITAL CAMERA"] =
        "EasyShare Z760 Zoom";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE Z915 DIGITAL CAMERA"] =
        "EasyShare Z915 Zoom";
    m_CameraModelMapper["Kodak.KODAK Z7590 ZOOM DIGITAL CAMERA"] =
        "EasyShare Z7590 Zoom";

    // Epson
    m_CameraModelMapper["Epson.Expression 1640XL"] = "Expression 1640 XL";
    m_CameraModelMapper["Epson.GT-15000"] = "GT-15000";

    // Fujifilm
    m_CameraModelMapper["Fujifilm.DS-7"] = "DS-7";
    m_CameraModelMapper["Fujifilm.FinePix2600Zoom"] = "FinePix 2600 Zoom";
    m_CameraModelMapper["Fujifilm.FinePix2650"] = "FinePix 2650";
    m_CameraModelMapper["Fujifilm.FinePix2800ZOOM"] = "FinePix 2800 Zoom";
    m_CameraModelMapper["Fujifilm.FinePixA101"] = "FinePix A101";
    m_CameraModelMapper["Fujifilm.FinePix A203"] = "FinePix A203";
    m_CameraModelMapper["Fujifilm.FinePix A330"] = "FinePix A330";
    m_CameraModelMapper["Fujifilm.FinePix A340"] = "FinePix A340";
    m_CameraModelMapper["Fujifilm.FinePix F470"] = "FinePix F470";
    m_CameraModelMapper["Fujifilm.FinePix JZ300"] = "FinePix JZ300";
    m_CameraModelMapper["Fujifilm.FinePix S5Pro"] = "FinePix S5 Pro";
    m_CameraModelMapper["Fujifilm.FinePix S602 ZOOM"] = "FinPix S602 Zoom";
    m_CameraModelMapper["Fujifilm.FinePix S1500"] = "FinePix S1500";
    m_CameraModelMapper["Fujifilm.FinePix S2000HD S2100HD"] =
        "FinePix S200HD or S2100HD";
    m_CameraModelMapper["Fujifilm.FinePix S3500"] = "FinePix S3500";
    m_CameraModelMapper["Fujifilm.FinePix S5500"] = "FinePix S5500";
    m_CameraModelMapper["Fujifilm.FinePix S5700 S700"] =
        "FinePix S5700 or S700";
    m_CameraModelMapper["Fujifilm.FinePix S5800 S800"] =
        "FinePix S5800 or S800";
    m_CameraModelMapper["Fujifilm.FinePix XP10"] = "FinePix XP10";
    m_CameraModelMapper["Fujifilm.FinePix XP20"] = "FinePix XP20";
    m_CameraModelMapper["Fujifilm.FinePix Z5fd"] = "FinePix Z5fd";
    m_CameraModelMapper["Fujifilm.FinePix Z20fd"] = "FinePix Z20fd";
    m_CameraModelMapper["Fujifilm.Frontier SP-3000"] =
        "Frontier Film Scanner SP-3000";
    m_CameraModelMapper["Fujifilm.X-T1"] = "X-T1";

    // Gateway
    m_CameraModelMapper["Gateway.DC-M42"] = "DC-M42";

    // General Imaging
    m_CameraModelMapper["General Imaging.E1035"] = "E1035";

    // Google
    m_CameraModelMapper["Google.Nexus One"] = "Nexus One";
    m_CameraModelMapper["Google.Pixel 2"] = "Pixel 2";

    // Hasselblad
    m_CameraModelMapper["Hasselblad.Hasselblad H3D-39"] = "H3D-39";

    // Hewlett-Packard
    m_CameraModelMapper["Hewlett-Packard.HP PhotoSmart 215"] =
        "PhotoSmart 215";
    m_CameraModelMapper["Hewlett-Packard.HP Photosmart M437"] =
        "PhotoSmart M437";
    m_CameraModelMapper["Hewlett-Packard.HP Photosmart M440"] =
        "PhotoSmart M440";
    m_CameraModelMapper["Hewlett-Packard.HP PhotoSmart R707 (V01.00)"] =
        "PhotoSmart R707";
    m_CameraModelMapper["Hewlett-Packard.HP psc1300"] = "PhotoSmart C1300";
    m_CameraModelMapper["Hewlett-Packard.HP psc1400"] = "PhotoSmart C1400";
    m_CameraModelMapper["Hewlett-Packard.HP psc1500"] = "PhotoSmart C1500";
    m_CameraModelMapper["Hewlett-Packard.HP psc1600"] = "PhotoSmart C1600";
    m_CameraModelMapper["Hewlett-Packard.HP pstc4200"] = "PhotoSmart C4200";
    m_CameraModelMapper["Hewlett-Packard.HP pstc4400"] = "PhotoSmart C4400";
    m_CameraModelMapper["Hewlett-Packard.HP pstc6200"] = "PhotoSmart C6200";
    m_CameraModelMapper["Hewlett-Packard.HP pstc7200"] = "PhotoSmart C7200";
    m_CameraModelMapper["Hewlett-Packard.HP ScanJet 2400"] = "Scanjet 2400";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet 4370"] = "Scanjet 4370";
    m_CameraModelMapper["Hewlett-Packard.HP ScanJet 4600"] = "Scanjet 4600";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet a909g"] =
        "Officejet Pro 8500 Premier";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet djf300"] = "Deskjet F300";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet djf2100"] =
        "Deskjet F2100";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet djf4100"] =
        "Deskjet F4100";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet djf4200"] =
        "Deskjet F4200";
    m_CameraModelMapper["Hewlett-Packard.HP Scanjet e709n"] = "Scanjet 6500";

    // HTC
    m_CameraModelMapper["HTC.HTC Desire 626"] = "Desire 626";
    m_CameraModelMapper["HTC.HTC One"] = "One";
    m_CameraModelMapper["HTC.myTouch_4G_Slide"] = "myTouch 4G Slide";

    // Huawei
    m_CameraModelMapper["Huawei.HUAWEI GRA-L09"] = "P8 GRA-L09";

    // Kodak
    m_CameraModelMapper["Kodak.DC200      (V02.20)"] = "DC200";
    m_CameraModelMapper["Kodak.KODAK DC280 ZOOM DIGITAL CAMERA"] =
        "DC280 Zoom";
    m_CameraModelMapper["Kodak.KODAK DC3800 DIGITAL CAMERA"] =
        "EasyShare DC3800";
    m_CameraModelMapper["Kodak.KODAK EASYSHARE Z1012 IS Digital Camera"] =
        "EasyShare Z1012 IS";
    m_CameraModelMapper["Kodak.KODAK V530 ZOOM DIGITAL CAMERA"] =
        "EasyShare V530 Zoom";
    m_CameraModelMapper["Kodak.KODAK Z650 ZOOM DIGITAL CAMERA"] =
        "EasyShare Z650 Zoom";
    m_CameraModelMapper["Kodak.PIXPRO FZ151"] = "PixPro FZ151";

    // Konica
    m_CameraModelMapper["Konica.KD-300Z"] = "KD-300 Zoom";

    // Konica Minolta
    m_CameraModelMapper["Konica Minolta.DiMAGE X50"] = "DiMAGE X50";
    m_CameraModelMapper["Konica Minolta.DiMAGE Z2"] = "DiMAGE Z2";
    m_CameraModelMapper["Konica Minolta.DiMAGE Z5"] = "DiMAGE Z5";
    m_CameraModelMapper["Konica Minolta.DiMAGE Z10"] = "DiMAGE Z10";
    m_CameraModelMapper["Konica Minolta.DiMAGE Z20"] = "DiMAGE Z20";

    // Kyocera
    m_CameraModelMapper["Kyocera.KC-S701"] = "Torque KC-S701";

    // Leica
    m_CameraModelMapper["Leica.D-LUX 3"] = "D-LUX 3";
    m_CameraModelMapper["Leica.D-LUX 5"] = "D-LUX 5";
    m_CameraModelMapper["Leica.M8 Digital Camera"] = "M8";

    // LG Electronics
    m_CameraModelMapper["LG Electronics.LG-D410"] = "D410";
    m_CameraModelMapper["LG Electronics.LG-K428"] = "K428";
    m_CameraModelMapper["LG Electronics.LGLS775"] = "Stylo 2 LS775";
    m_CameraModelMapper["LG Electronics.LGUS991"] = "US991";

    // Microsoft
    m_CameraModelMapper["Microsoft.Lumia 950 XL Dual SIM"] =
        "Lumia 950 XL Dual SIM";

    // Minolta
    m_CameraModelMapper["Minolta.Dimage 2330 Zoom"] = "DiMAGE 2330 Zoom";
    m_CameraModelMapper["Minolta.DiMAGE S414"] = "DiMAGE S414";
    m_CameraModelMapper["Minolta.DiMAGE X"] = "DiMAGE X";

    // Motorola
    m_CameraModelMapper["Motorola.moto g stylus 5G"] = "Moto G Stylus 5G";
    m_CameraModelMapper["Motorola.Nexus 6"] = "Nexus 6";
    m_CameraModelMapper["Motorola.XT1080"] = "DROID Ultra";
    m_CameraModelMapper["Motorola.XT1254"] = "DROID Turbo";
    m_CameraModelMapper["Motorola.XT1585"] = "DROID Turbo 2";

    // Nikon
    m_CameraModelMapper["Nikon.NIKON D2X"] = "D2X";
    m_CameraModelMapper["Nikon.NIKON D2Xs"] = "D2Xs";
    m_CameraModelMapper["Nikon.NIKON D3S"] = "D3s";
    m_CameraModelMapper["Nikon.NIKON D4"] = "D4";
    m_CameraModelMapper["Nikon.NIKON D5"] = "D5";
    m_CameraModelMapper["Nikon.NIKON D6"] = "D6";
    m_CameraModelMapper["Nikon.NIKON D40"] = "D40";
    m_CameraModelMapper["Nikon.NIKON D40X"] = "D40X";
    m_CameraModelMapper["Nikon.NIKON D50"] = "D50";
    m_CameraModelMapper["Nikon.NIKON D70"] = "D70";
    m_CameraModelMapper["Nikon.NIKON D80"] = "D80";
    m_CameraModelMapper["Nikon.NIKON D90"] = "D90";
    m_CameraModelMapper["Nikon.NIKON D100"] = "D100";
    m_CameraModelMapper["Nikon.NIKON D200"] = "D200";
    m_CameraModelMapper["Nikon.NIKON D300"] = "D300";
    m_CameraModelMapper["Nikon.NIKON D300S"] = "D300S";
    m_CameraModelMapper["Nikon.NIKON D600"] = "D600";
    m_CameraModelMapper["Nikon.NIKON D700"] = "D700";
    m_CameraModelMapper["Nikon.NIKON D800"] = "D800";
    m_CameraModelMapper["Nikon.NIKON D800E"] = "D800E";
    m_CameraModelMapper["Nikon.NIKON D810"] = "D810";
    m_CameraModelMapper["Nikon.NIKON D850"] = "D850";
    m_CameraModelMapper["Nikon.NIKON D3000"] = "D3000";
    m_CameraModelMapper["Nikon.NIKON D3100"] = "D3100";
    m_CameraModelMapper["Nikon.NIKON D3200"] = "D3200";
    m_CameraModelMapper["Nikon.NIKON D3300"] = "D3300";
    m_CameraModelMapper["Nikon.NIKON D5000"] = "D5000";
    m_CameraModelMapper["Nikon.NIKON D5100"] = "D5100";
    m_CameraModelMapper["Nikon.NIKON D5200"] = "D5200";
    m_CameraModelMapper["Nikon.NIKON D7000"] = "D7000";
    m_CameraModelMapper["Nikon.NIKON D7100"] = "D7100";
    m_CameraModelMapper["Nikon.E880"] = "Coolpix 880";
    m_CameraModelMapper["Nikon.E885"] = "Coolpix 885";
    m_CameraModelMapper["Nikon.E900"] = "Coolpix 900";
    m_CameraModelMapper["Nikon.E950"] = "Coolpix 950";
    m_CameraModelMapper["Nikon.E995"] = "Coolpix 995";
    m_CameraModelMapper["Nikon.E2000"] = "Coolpix 2000";
    m_CameraModelMapper["Nikon.E3200"] = "Coolpix 3200";
    m_CameraModelMapper["Nikon.E4300"] = "Coolpix 4300";
    m_CameraModelMapper["Nikon.E4600"] = "Coolpix 4600";
    m_CameraModelMapper["Nikon.E5200"] = "Coolpix 5200";
    m_CameraModelMapper["Nikon.COOLPIX L1"] = "Coolpix L1";
    m_CameraModelMapper["Nikon.COOLPIX L18"] = "Coolpix L18";
    m_CameraModelMapper["Nikon.COOLPIX L320"] = "Coolpix L320";
    m_CameraModelMapper["Nikon.COOLPIX L810"] = "Coolpix L810";
    m_CameraModelMapper["Nikon.COOLPIX P510"] = "Collpix P510";
    m_CameraModelMapper["Nikon.COOLPIX P520"] = "Coolpix P520";
    m_CameraModelMapper["Nikon.COOLPIX P5000"] = "Coolpix P5000";
    m_CameraModelMapper["Nikon.COOLPIX P5100"] = "Coolpix P5100";
    m_CameraModelMapper["Nikon.COOLPIX S550"] = "Coolpix S550";
    m_CameraModelMapper["Nikon.COOLPIX S6100"] = "Coolpix S6100";
    m_CameraModelMapper["Nikon.COOLPIX S6300"] = "Coolpix S6300";
    m_CameraModelMapper["Nikon.COOLPIX S8100"] = "Coolpix S8100";
    m_CameraModelMapper["Nikon.COOLPIX S8200"] = "Coolpix S8200";
    m_CameraModelMapper["Nikon.COOLPIX S9100"] = "Coolpix S9100";
    m_CameraModelMapper["Nikon.COOLPIX S9300"] = "Coolpix S9300";

    // Nokia
    m_CameraModelMapper["Nokia.5300"] = "5300";
    m_CameraModelMapper["Nokia.6555b"] = "6555b";
    m_CameraModelMapper["Nokia.E71"] = "E71";
    m_CameraModelMapper["Nokia.Lumia 630"] = "Lumia 630";
    m_CameraModelMapper["Nokia.Lumia 1020"] = "Lumia 1020";
    m_CameraModelMapper["Nokia.N8-00"] = "N8-00";
    m_CameraModelMapper["Nokia.N82"] = "N82";
    m_CameraModelMapper["Nokia.N95"] = "N95";

    // Olympus
    m_CameraModelMapper["Olympus.C180,D435"] = "Camedia C-180, Camedia D-435";
    m_CameraModelMapper["Olympus.C300Z,D550Z"] =
        "Camedia C-300 Zoom, Camedia D-500 Zoom";
    m_CameraModelMapper["Olympus.C860L,D360L"] =
        "Camedia C860L, Camedia D360L";
    m_CameraModelMapper["Olympus.C900Z,D400Z"] =
        "Camedia C900 Zoom, Camedia D400 Zoom";
    m_CameraModelMapper["Olympus.C2000Z"] = "Camedia C2000 Zoom";
    m_CameraModelMapper["Olympus.C2040Z"] = "Camedia C2040 Zoom";
    m_CameraModelMapper["Olympus.C2100UZ"] = "Camedia C2100 UltraZoom";
    m_CameraModelMapper["Olympus.C3000Z"] = "Camedia C3000 Zoom";
    m_CameraModelMapper["Olympus.C3030Z"] = "Camedia C3030 Zoom";
    m_CameraModelMapper["Olympus.C3040Z"] = "Camedia C3040 Zoom";
    m_CameraModelMapper["Olympus.C4100Z,C4000Z"] =
        "Camedia C4100Z, Camedia C4000Z Zoom";
    m_CameraModelMapper["Olympus.C-5000Z"] = "Camedia C5000 Zoom";
    m_CameraModelMapper["Olympus.C5050Z"] = "Camedia C5050 Zoom";
    m_CameraModelMapper["Olympus.D555Z,C315Z"] =
        "Camedia D555 Zoom, C315 Zoom";
    m_CameraModelMapper["Olympus.E-300"] = "Evolt E-300";
    m_CameraModelMapper["Olympus.E-M1"] = "Evolt E-M1";
    m_CameraModelMapper["Olympus.E-M5"] = "Evolt E-M5";
    m_CameraModelMapper["Olympus.E-M10"] = "Evolt E-M10";
    m_CameraModelMapper["Olympus.E-PL1"] = "Evolt E-PL1";
    m_CameraModelMapper["Olympus.SP510UZ"] = "SP-510 UltraZoom";
    m_CameraModelMapper["Olympus.SP560UZ"] = "SP-560 UltraZoom";
    m_CameraModelMapper["Olympus.SZ-20"] = "SZ-20";
    m_CameraModelMapper["Olympus.SZ-30MR"] = "SZ-30MR";
    m_CameraModelMapper["Olympus.TG-5"] = "Tough TG-5";
    m_CameraModelMapper["Olympus.uD800,S800"] = "uD800, S800";
    m_CameraModelMapper["Olympus.X300,D565Z,C450Z"] =
        "X300, Camedia D565 Zoom, Camedia C450 Zoom";

    // Panasonic
    m_CameraModelMapper["Panasonic.DMC-F2"] = "Lumix DMC-F 2";
    m_CameraModelMapper["Panasonic.DMC-FH20"] = "Lumix DMC-FH 20";
    m_CameraModelMapper["Panasonic.DMC-FP3"] = "Lumix DMC-FP 3";
    m_CameraModelMapper["Panasonic.DMC-FS10"] = "Lumix DMC-FS 10";
    m_CameraModelMapper["Panasonic.DMC-FS35"] = "Lumix DMC-FS 35";
    m_CameraModelMapper["Panasonic.DMC-FS45"] = "Lumix DMC-FS 45";
    m_CameraModelMapper["Panasonic.DMC-FS62"] = "Lumix DMC-FS 62";
    m_CameraModelMapper["Panasonic.DMC-FX8"] = "Lumix DMC-FX 8";
    m_CameraModelMapper["Panasonic.DMC-FZ8"] = "Lumix DMC-FZ 8";
    m_CameraModelMapper["Panasonic.DMC-FZ38"] = "Lumix DMC-FZ 38";
    m_CameraModelMapper["Panasonic.DMC-FZ100"] = "Lumix DMC-FZ 100";
    m_CameraModelMapper["Panasonic.DMC-FZ200"] = "Lumix DMC-FZ 200";
    m_CameraModelMapper["Panasonic.DMC-FZ1000"] = "Lumix DMC-FZ 1000";
    m_CameraModelMapper["Panasonic.DMC-G3"] = "Lumix DMC-G 3";
    m_CameraModelMapper["Panasonic.DMC-G5"] = "Lumix DMC-G 5";
    m_CameraModelMapper["Panasonic.DMC-GF1"] = "Lumix DMC-GF 1";
    m_CameraModelMapper["Panasonic.DMC-LS75"] = "Lumix DMC-LS 75";
    m_CameraModelMapper["Panasonic.DMC-LZ6"] = "Lumix DMC-LZ 6";
    m_CameraModelMapper["Panasonic.DMC-LZ8"] = "Lumix DMC-LZ 8";
    m_CameraModelMapper["Panasonic.DMC-TS1"] = "Lumix DMC-TS 1";
    m_CameraModelMapper["Panasonic.DMC-TZ3"] = "Lumix DMC-TZ 3";
    m_CameraModelMapper["Panasonic.DMC-TZ5"] = "Lumix DMC-TZ 5";
    m_CameraModelMapper["Panasonic.DMC-TZ10"] = "Lumix DMC-TZ 10";
    m_CameraModelMapper["Panasonic.DMC-ZS10"] = "Lumix DMC-ZS 10";

    // Pentax
    m_CameraModelMapper["Pentax.PENTAX K-5"] = "K-5";
    m_CameraModelMapper["Pentax.PENTAX K-5 II s"] = "K-5 IIS";
    m_CameraModelMapper["Pentax.PENTAX K20D"] = "K20D";
    m_CameraModelMapper["Pentax.PENTAX K10D"] = "K10D";
    m_CameraModelMapper["Pentax.PENTAX K100D"] = "K100D";
    m_CameraModelMapper["Pentax.PENTAX K-m"] = "K-m";
    m_CameraModelMapper["Pentax.PENTAX K-x"] = "K-x";
    m_CameraModelMapper["Pentax.PENTAX Optio 33WR"] = "Optio 33WR";
    m_CameraModelMapper["Pentax.PENTAX Optio 60"] = "Optio 60";
    m_CameraModelMapper["Pentax.PENTAX Optio MX"] = "Optio MX";
    m_CameraModelMapper["Pentax.PENTAX Optio T30"] = "Optio T30";
    m_CameraModelMapper["Pentax.PENTAX Optio W20"] = "Optio W20";
    m_CameraModelMapper["Pentax.PENTAX *ist D"] = "*ist D";

    // Phase One
    m_CameraModelMapper["Phase One.P40+"] = "P40+";

    // Polaroid
    m_CameraModelMapper["Polaroid.i1037"] = "i1037";

    // Ricoh
    m_CameraModelMapper["Ricoh.Caplio G4"] = "Caplio G4";

    // Samsung
    m_CameraModelMapper["Samsung.Digimax L60"] = "Digimax L60";
    m_CameraModelMapper["Samsung.Galaxy S23 Ultra"] = "Galaxy S23 Ultra";
    m_CameraModelMapper["Samsung.Galaxy S24 Ultra"] = "Galaxy S24 Ultra";
    m_CameraModelMapper["Samsung.GT-I9100"] = "Galaxy S II GT-I9100";
    m_CameraModelMapper["Samsung.GT-I9295"] = "Galaxy S IV Active GT-I9295";
    m_CameraModelMapper["Samsung.GT-I9300"] = "Galaxy S III GT-I9300";
    m_CameraModelMapper["Samsung.GT-P5110"] = "Galaxy Tab 2 GT-P5110";
    m_CameraModelMapper["Samsung.GT-S7272"] = "Galaxy Ace 3";
    m_CameraModelMapper["Samsung.NX100"] = "NX100";
    m_CameraModelMapper["Samsung.SAMSUNG-SGH-I337"] = "Galaxy S4 SGH-I337";
    m_CameraModelMapper["Samsung.SAMSUNG-SM-G900A"] = "Galaxy S6 SM-G900A";
    m_CameraModelMapper["Samsung.SAMSUNG-SM-G928A"] =
        "Galaxy S6 Edge+ SM-G928A";
    m_CameraModelMapper["Samsung.SAMSUNG-SM-G935A"] = "Galaxy S7 Edge (AT&T)";
    m_CameraModelMapper["Samsung.SGH-M919"] = "Galaxy S4 SGH-M919";
    m_CameraModelMapper["Samsung.SGH-T989"] = "Galaxy S II SGH-T989";
    m_CameraModelMapper["Samsung.SM-A526B"] = "Galaxy SM-A526B";
    m_CameraModelMapper["Samsung.SM-G900F"] =
        "Galaxy S5 SM-G900F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G900I"] =
        "Galaxy S5 SM-G900I (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G900V"] = "Galaxy S5 SM-G900V (Verizon)";
    m_CameraModelMapper["Samsung.SM-G920I"] =
        "Galaxy S6 SM-G920I (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G930F"] =
        "Galaxy S7 SM-G930F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G935F"] =
        "Galaxy S7 SM-G935F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G935P"] = "Galaxy S7 SM-G935P";
    m_CameraModelMapper["Samsung.SM-G950F"] =
        "Galaxy S8 SM-G950F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G920F"] =
        "Galaxy S6 SM-G920F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G920T"] = "Galaxy S6 SM-G920T (T-mobile)";
    m_CameraModelMapper["Samsung.SM-G925F"] =
        "Galaxy S6 SM-G925F (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G928F"] =
        "Galaxy S6 Edge+ (Factory Unlocked)";
    m_CameraModelMapper["Samsung.SM-G930V"] = "Galaxy S7 SM-G930V (Verizon)";
    m_CameraModelMapper["Samsung.SM-G965U"] = "Galaxy S9+ SM-G965U (Unlocked)";
    m_CameraModelMapper["Samsung.SM-J500FN"] = "Galaxy J5 SM-J500FN";
    m_CameraModelMapper["Samsung.SM-J500M"] = "Galaxy J5 SM-J500M";
    m_CameraModelMapper["Samsung.SM-N920T"] = "Galaxy Note 5 SM-N920T";
    m_CameraModelMapper["Samsung.SM-N9005"] = "Galaxy Note 3 SM-N9005";
    m_CameraModelMapper["Samsung.SM-N9020"] = "Galaxy Note 3 SM-N9020";
    m_CameraModelMapper["Samsung.SM-S820L"] = "Galaxy Core Prime";
    m_CameraModelMapper["Samsung.<Digimax D53>"] = "Digimax D53";
    m_CameraModelMapper[
        "Samsung.<Digimax S500 / Kenox S500 / Digimax Cyber 530>"] =
        "Digimax S500, Kenox S500, Digimax Cyber 530";
    m_CameraModelMapper[
        "Samsung.<Digimax S600 / Kenox S600 / Digimax Cyber 630>"] =
        "Digimax S600, Kenox S600, Digimax Cyber 630";
    m_CameraModelMapper["Samsung.<KENOX S630  / Samsung S630>"] =
        "Kenox S630, Digimax S630";
    m_CameraModelMapper["Samsung.<VLUU L730  / Samsung L730>"] =
        "Vluu L730, Digimax L739";

    // Sanyo
    m_CameraModelMapper["Sanyo.S4"] = "Xacti DSC-S4";

    // Sony
    m_CameraModelMapper["Sony.C6603"] = "Xperia Z C6603";
    m_CameraModelMapper["Sony.CD MAVICA"] = "CD Mavica";
    m_CameraModelMapper["Sony.CYBERSHOT"] = "CyberShot";
    m_CameraModelMapper["Sony.DCR-TRV20E"] = "DCR-TRV20E";
    m_CameraModelMapper["Sony.DSC-H7"] = "CyberShot DSC-H7";
    m_CameraModelMapper["Sony.DSC-HX100V"] = "CyberShot DSC-HX100V";
    m_CameraModelMapper["Sony.DSC-P8"] = "CyberShot DSC-P8";
    m_CameraModelMapper["Sony.DSC-P72"] = "CyberShot DSC-P72";
    m_CameraModelMapper["Sony.DSC-P200"] = "CyberShot DSC-P200";
    m_CameraModelMapper["Sony.DSC-S40"] = "CyberShot DSC-S40";
    m_CameraModelMapper["Sony.DSC-S650"] = "CyberShot DSC-S650";
    m_CameraModelMapper["Sony.DSC-S730"] = "CyberShot DSC-S730";
    m_CameraModelMapper["Sony.DSC-S780"] = "CyberShot DSC-S780";
    m_CameraModelMapper["Sony.DSC-T1"] = "CyberShot DSC-T1";
    m_CameraModelMapper["Sony.DSC-T5"] = "CyberShot DSC-T5";
    m_CameraModelMapper["Sony.DSC-T200"] = "CyberShot DSC-T200";
    m_CameraModelMapper["Sony.DSC-T50"] = "CyberShot DSC-T50";
    m_CameraModelMapper["Sony.DSC-W1"] = "CyberShot DSC-W1";
    m_CameraModelMapper["Sony.DSC-W7"] = "CyberShot DSC-W7";
    m_CameraModelMapper["Sony.DSC-W80"] = "CyberShot DSC-W80";
    m_CameraModelMapper["Sony.DSC-W90"] = "CyberShot DSC-W90";
    m_CameraModelMapper["Sony.DSC-W100"] = "CyberShot DSC-W100";
    m_CameraModelMapper["Sony.DSC-W120"] = "CyberShot DSC-W120";
    m_CameraModelMapper["Sony.DSC-W300"] = "CyberShot DSC-W300";
    m_CameraModelMapper["Sony.DSLR-A100"] = "Alpha DSLR-A100";
    m_CameraModelMapper["Sony.DSLR-A500"] = "Alpha DSLR-A500";
    m_CameraModelMapper["Sony.DSLR-A700"] = "Alpha DSLR-A700";
    m_CameraModelMapper["Sony.DSLR-A900"] = "Alpha DSLR-A700";
    m_CameraModelMapper["Sony.ILCE-7M3"] = "Alpha ILCE-7 Mark 3";
    m_CameraModelMapper["Sony.ILCE-7R"] = "Alpha ILCE-7R";
    m_CameraModelMapper["Sony.ILCE-7RM5"] = "Alpha ILCE-7R Mark 5";
    m_CameraModelMapper["Sony.ILCE-6000"] = "Alpha 6000";
    m_CameraModelMapper["Sony.ILCE-6300"] = "Alpha 6300";
    m_CameraModelMapper["Sony.ILCE-6500"] = "Alpha 6500";
    m_CameraModelMapper["Sony.NEX-5R"] = "Alpha NEX 5R";
    m_CameraModelMapper["Sony.SLT-A37"] = "Alpha SLT-A37";
    m_CameraModelMapper["Sony.SLT-A57"] = "Alpha SLT-A57";
    m_CameraModelMapper["Sony.SLT-A65V"] = "Alpha SLT-A65V";
    m_CameraModelMapper["Sony.SLT-A77V"] = "Alpha SLT-A55V";
    m_CameraModelMapper["Sony.SLT-A99"] = "Alpha SLT-A99";
    m_CameraModelMapper["Sony.SLT-A99V"] = "Alpha SLT-A99V";

    // Sony Ericsson
    m_CameraModelMapper["Sony Ericsson.C905"] = "C905";
    m_CameraModelMapper["Sony Ericsson.SK17a"] = "SK17a";
    m_CameraModelMapper["Sony Ericsson.U5i"] = "U5i";
    m_CameraModelMapper["Sony Ericsson.W595"] = "W595";

    // Supra
    m_CameraModelMapper["Supra.Super Slim XS70"] = "Super Slim XS 70";

    // Toshiba
    m_CameraModelMapper["Toshiba.PDRM5"] = "PDR-M5";

    // Xiaomi
    m_CameraModelMapper["Xiaomi.2312DRA50G"] = "Redmi Note 13 Pro 5G";
    m_CameraModelMapper["Xiaomi.Redmi Note 8 Pro"] = "Redmi Note 8 Pro";
    m_CameraModelMapper["Xiaomi.Redmi Note 8T"] = "Redmi Note 8T";

    // ZTE
    m_CameraModelMapper["ZTE.Z959"] = "Grand X3 Z959";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Lens maker mapper
void ExifInfo::Init_LensMakerMapper()
{
    CALL_IN("");

    // Add makers
    m_LensMakerMapper["Apple"] = "Apple";
    m_LensMakerMapper["Google"] = "Google";
    m_LensMakerMapper["NIKON"] = "Nikon";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Lens model mapper
void ExifInfo::Init_LensModelMapper()
{
    CALL_IN("");

    // No model information
    m_LensModelMapper["."] = "";
    m_LensModelMapper[".----"] = "";
    m_LensModelMapper[".0.0 mm f/0.0"] = "";

    // Unknown maker
    m_LensModelMapper[".6.1-30.5 mm"] = "6.1-30.5mm";
    m_LensModelMapper[".10-20mm"] = "10-20mm";
    m_LensModelMapper[".17-70mm"] = "17-70mm";
    m_LensModelMapper[".17.0-55.0 mm f/2.8"] = "17-55mm f/2.8";
    m_LensModelMapper[".18-250mm"] = "18-250mm";
    m_LensModelMapper[".18.0-105.0 mm f/3.5-5.6"] = "18-105mm f/3.5-5.6";
    m_LensModelMapper[".28-80mm F3.5-5.6"] = "28-80mm f/3.5-5.6";
    m_LensModelMapper[".28.0-105.0 mm"] = "28-105mm";
    m_LensModelMapper[".28.0-300.0 mm f/3.5-5.6"] = "28-300mm f/3.5-5.6";
    m_LensModelMapper[".50.0 mm f/1.8"] = "50mm f/1.8";
    m_LensModelMapper[".50-500mm"] = "50-500mm";
    m_LensModelMapper[".70-200mm"] = "70-200mm";
    m_LensModelMapper[".70.0-200.0 mm"] = "70-200mm";
    m_LensModelMapper[".70.0-200.0 mm f/2.8"] = "70-200mm f/2.8";
    m_LensModelMapper[".100-200mm F4.5"] = "100-200mm f/4.5";
    m_LensModelMapper[".105.0 mm f/2.8"] = "105mm f/2.8";
    m_LensModelMapper[".135.0-400.0 mm f/4.5-5.6"] = "135-400mm f/4.5-5.6";
    m_LensModelMapper[".150.0-500.0 mm f/5.0-6.3"] = "150-500mm f/5-6.3";
    m_LensModelMapper[".180.0-400.0 mm f/4.0"] = "180-400mm f/4";
    m_LensModelMapper[".200.0-400.0 mm f/4.0"] = "200-400mm f/4.0";
    m_LensModelMapper[".250.0-560.0 mm f/5.6"] = "250-560mm f/5.6";
    m_LensModelMapper[".600.0 mm f/4.0"] = "600mm f/4";

    // Apple
    m_LensModelMapper["Apple.iPad back camera 4.28mm f/2.4"] =
        "Apple iPad Back Camera 4.28mm f/2.4";
    m_LensModelMapper["Apple.iPad mini back camera 3.3mm f/2.4"] =
        "Apple iPad Mini Back Camera 3.3mm f/2.4";
    m_LensModelMapper[
        "Apple.iPhone SE (2nd generation) back camera 3.99mm f/1.8"] =
        "Apple iPhone SE (2nd Generation) Back Camera 3.99mm f/1.8";
    m_LensModelMapper[
        "Apple.iPhone SE (3rd generation) back camera 3.99mm f/1.8"] =
        "Apple iPhone SE (3rd Generation) Back Camera 3.99mm f/1.8";
    m_LensModelMapper["Apple.iPhone 5 back camera 4.12mm f/2.4"] =
        "Apple iPhone 5 Back Camera 4.12mm f/2.4";
    m_LensModelMapper["Apple.iPhone 5s back camera 4.15mm f/2.2"] =
        "Apple iPhone 5s Back Camera 4.15mm f/2.2";
    m_LensModelMapper["Apple.iPhone 6 back camera 4.15mm f/2.2"] =
        "Apple iPhone 6 Back Camera 4.15mm f/2.2";
    m_LensModelMapper["Apple.iPhone 6 front camera 2.65mm f/2.2"] =
        "Apple iPhone 6 Front Camera 2.65mm f/2.2";
    m_LensModelMapper["Apple.iPhone 6s back camera 4.15mm f/2.2"] =
        "Apple iPhone 6s Back Camera 4.15mm f/2.2";
    m_LensModelMapper["Apple.iPhone 6s Plus back camera 4.15mm f/2.2"] =
        "Apple iPhone 6s Plus Back Camera 4.15mm f/2.2";
    m_LensModelMapper["Apple.iPhone 7 back camera 3.99mm f/1.8"] =
        "Apple iPhone 7 Back Camera 3.99mm f/1.8";
    m_LensModelMapper["Apple.iPhone 8 back camera 3.99mm f/1.8"] =
        "Apple iPhone 8 Back Camera 3.99mm f/1.8";
    m_LensModelMapper["Apple.iPhone 11 Pro Max back triple camera 6mm f/2"] =
        "Apple iPhone 11 Pro Max Back Triple Camera 6mm f/2";
    m_LensModelMapper["Apple.iPhone 11 back dual wide camera 4.25mm f/1.8"] =
        "Apple iPhone 11 Back Dual Wide Camera 4.25mm f/1.8";
    m_LensModelMapper["Apple.iPhone 12 back camera 4.2mm f/1.6"] =
        "Apple iPhone 12 Back Camera 4.2mm f/1.6";
    m_LensModelMapper["Apple.iPhone 12 back dual wide camera 1.55mm f/2.4"] =
        "Apple iPhone 12 Back Dual Wide Camera 1.55mm f/2.4";
    m_LensModelMapper["Apple.iPhone 12 back dual wide camera 4.2mm f/1.6"] =
        "Apple iPhone 12 Back Camera 4.2mm f/1.6";
    m_LensModelMapper["Apple.iPhone 12 front camera 2.71mm f/2.2"] =
        "Apple iPhone 12 Front Camera 2.71mm f/2.2";
    m_LensModelMapper["Apple.iPhone 12 Pro Max back camera 5.1mm f/1.6"] =
        "Apple iPhone 12 Pro Max Back Camera 5.1mm f/1.6";
    m_LensModelMapper["Apple.iPhone 15 Pro Max back triple camera 6.765mm "
        "f/1.78"] =
        "Apple iPhone 15 Pro Max back triple camera 6.765mm f/1.78";
    m_LensModelMapper["Apple.iPhone 16e back camera 4.2mm f/1.64"] =
        "Apple iPhone 16e Back Camera 4.2mm f/1.64";
    m_LensModelMapper["Apple.iPhone 16e front camera 2.69mm f/1.9"] =
        "Apple iPhone 16e Front Camera 2.69mm f/1.9";

    // Canon
    m_LensModelMapper[".EF16-35mm f/2.8L II USM"] =
        "Canon EF 16-35mm f/2.8 L II USM";
    m_LensModelMapper[".EF17-40mm f/4L USM"] = "Canon EF 17-40mm f/4 L USM";
    m_LensModelMapper[".EF-M55-200mm f/4.5-6.3 IS STM"] =
        "Canon EF-M 55-200mm f/4.5-6.3 IS STM";
    m_LensModelMapper[".EF-S17-55mm f/2.8 IS USM"] =
        "Canon EF-S 17-55mm f/2.8 IS USM";
    m_LensModelMapper[".EF-S18-55mm f/3.5-5.6 IS"] =
        "Canon EF-S 18-55mm f/3.5-5.6 IS";
    m_LensModelMapper[".EF-S18-55mm f/3.5-5.6 IS II"] =
        "Canon EF-S 18-55mm f/3.5-5.6 IS II";
    m_LensModelMapper[".EF-S18-55mm f/3.5-5.6 III"] =
        "Canon EF-S 18-55mm f/3.5-5.6 III";
    m_LensModelMapper[".EF-S18-135mm f/3.5-5.6 IS"] =
        "Canon EF-S 18-135mm f/3.5-5.6 IS";
    m_LensModelMapper[".EF-S18-135mm f/3.5-5.6 IS USM"] =
        "Canon EF-S 18-135mm f/3.5-5.6 IS USM";
    m_LensModelMapper[".EF-S18-200mm f/3.5-5.6 IS"] =
        "Canon EF-S 18-200mm f/3.5-5.6 IS";
    m_LensModelMapper[".EF24-105mm f/4L IS USM"] =
        "Canon EF 24-105mm f/4 L IS USM";
    m_LensModelMapper[".EF50mm f/2.5 Compact Macro"] =
        "Canon EF 50mm f/2.5 Compact Macro";
    m_LensModelMapper[".EF50mm f/1.4 USM"] = "Canon EF 50mm f/1.4 USM";
    m_LensModelMapper[".EF-S55-250mm f/4-5.6 IS II"] =
        "Canon EF-S 55-250mm f/4-5.6 IS II";
    m_LensModelMapper[".EF70-200mm f/4L USM"] = "Canon EF 70-200mm f/4 L USM";
    m_LensModelMapper[".EF70-300mm f/4-5.6 IS USM"] =
        "Canon EF 70-300mm f/4-5.6 IS USM";
    m_LensModelMapper[".EF70-300mm f/4-5.6 IS II USM"] =
        "Canon EF 70-300mm f/4-5.6 IS II USM";
    m_LensModelMapper[".EF75-300mm f/4-5.6"] = "Canon EF 75-300mm f/4-5.6";
    m_LensModelMapper[".EF75-300mm f/4-5.6 IS USM"] =
        "Canon EF 75-300mm f/4-5.6 IS USM";
    m_LensModelMapper[".EF100-400mm f/4.5-5.6L IS USM"] =
        "Canon EF 100-400mm f/4.5-5.6L IS USM";
    m_LensModelMapper[".EF180mm f/3.5L Macro USM"] =
        "Canon EF 180mm f/3.5 L Macro USM";
    m_LensModelMapper[".EF180mm f/3.5L Macro USM +1.4x III"] =
        "Canon EF 180mm f/3.5 L Macro USM (with 1.4x Converter Mark III)";
    m_LensModelMapper[".EF300mm f/4L IS USM"] = "Canon EF 300mm f/4 L IS USM";
    m_LensModelMapper[".EF400mm f/5.6L USM +1.4x III"] =
        "Canon EF 400mm f/5.6L USM (with 1.4x Converter III)";
    m_LensModelMapper[".EF500mm f/4L IS USM"] = "Canon EF 500mm f/4 L IS USM";
    m_LensModelMapper[".EF600mm f/4L IS USM"] = "Canon EF 600mm f/4 L IS USM";
    m_LensModelMapper[".EF600mm f/4L IS II USM"] =
        "Canon EF 600mm f/4 L IS II USM";
    m_LensModelMapper[".EF600mm f/4L IS USM +1.4x III"] =
        "EF 600mm f/4 L IS USM (with 1.4x Converter III)";
    m_LensModelMapper[".RF16mm F2.8 STM"] = "Canon RF 16mm f/2.8 STM";

    // Fujifilm
    m_LensModelMapper[".XF56mmF1.2 R"] = "Fujifilm Fujinon XF 56mm f/1.2 R";

    // Google
    m_LensModelMapper["Google.Pixel 6 Pro back camera 6.81mm f/1.85"] =
        "Google Pixel 6 Pro Back Camera 6.81mm f/1.85";

    // Nikon
    m_LensModelMapper["Nikon.AF-S NIKKOR 180-400mm f/4E TC1.4 FL ED VR"] =
        "Nikon AF-S Nikkor 180-400mm f/4 E TC 1.4 FL ED VR";

    // Lumix
    m_LensModelMapper[".LUMIX G VARIO 45-200/F4.0-5.6"] =
        "Lumix G Vario 45-200mm f/4-5.6";
    m_LensModelMapper[".LUMIX G VARIO PZ 45-175/F4.0-5.6"] =
        "Lumix G Vario PZ 45-175mm f/4-5.6";

    // Olympus
    m_LensModelMapper["OLYMPUS M.12-40mm F2.8"] =
        "Olympus M.Zuiko 12-40mm f/2.8";
    m_LensModelMapper["OLYMPUS M.40-150mm F4.0-5.6"] =
        "Olympus M.Zuiko 40-150mm f/4-5.6";
    m_LensModelMapper["OLYMPUS M.75-300mm F4.8-6.7 II"] =
        "Olympus M.Zuiko 75-300mm f/4.8-6.7 II";

    // Sigma
    m_LensModelMapper[".DT 18-35mm F1.8"] = "Sigma DT 18-35mm f/1.8";
    m_LensModelMapper[".60-600mm F4.5-6.3 DG OS HSM | Sports 018"] =
        "Sigma 60-600mm f/4.5-6.3 DG OS HSM Sports";
    m_LensModelMapper[".150-600mm F5-6.3 DG OS HSM | Contemporary 015"] =
        "Sigma 150-600mm f/5-6.3 DG OS HSM | Contemporary 015";
    m_LensModelMapper[".150-600mm F5-6.3 DG OS HSM | Contemporary 015 +1.4x"] =
        "Sigma 150-600mm f/5-6.3 DG OS HSM | Contemporary 015 "
        "(with 1.4x Converter)";

    // Sony
    m_LensModelMapper[".16-35mm F2.8 ZA SSM"] =
        "Sony Zeiss Sonar 16-35mm f/2.8 ZA SSM";
    m_LensModelMapper[".FE 20mm F1.8 G"] = "Sony FE 20mm f/1.8 G";
    m_LensModelMapper[".DT 70-300mm F4-5.6 SAM"] =
        "Sony DT 70-300mm f/4-5.6 SAM";
    m_LensModelMapper[".85mm F1.4 ZA"] = "Sony Zeiss Planar 85mm f/1.4 ZA";
    m_LensModelMapper[".150-600mm F5-6.3 SSM"] = "Sony 150-600mm f/5-6.3 SSM";
    m_LensModelMapper[".DT 18-55mm F3.5-5.6 SAM"] =
        "Sony DT 18-55mm f/3.5-5.6 SAM";
    m_LensModelMapper[".E 18-55mm F3.5-5.6 OSS"] =
        "Sony E 18-55mm f/3.5-5.6 OSS";
    m_LensModelMapper[".E 35mm F1.8 OSS"] = "Sony E 35mm f/1.8 OSS";
    m_LensModelMapper[".FE 70-200mm F4 G OSS"] = "Sony FE 70-200mm f/4 G OSS";
    m_LensModelMapper[".FE 200-600mm F5.6-6.3 G OSS"] =
        "FE 200-600mm f/5.6-6.3 G OSS";

    // Tamron
    m_LensModelMapper[".E 17-70mm F2.8 B070"] = "Tamron 17-70mm f/2.8 B070";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// F Stop mapper
void ExifInfo::Init_FStopMapper()
{
    CALL_IN("");

    // For values from Photo.FNumber

    m_FStopMapper["0/1"] = "";
    m_FStopMapper["1/1"] = "1";
    m_FStopMapper["2/1"] = "2";
    m_FStopMapper["3/1"] = "3";
    m_FStopMapper["4/1"] = "4";
    m_FStopMapper["5/1"] = "5";
    m_FStopMapper["8/1"] = "8";
    m_FStopMapper["9/1"] = "9";
    m_FStopMapper["10/1"] = "10";
    m_FStopMapper["11/1"] = "11";
    m_FStopMapper["12/1"] = "12";
    m_FStopMapper["13/1"] = "13";
    m_FStopMapper["14/1"] = "14";
    m_FStopMapper["16/1"] = "16";
    m_FStopMapper["18/1"] = "18";
    m_FStopMapper["20/1"] = "20";
    m_FStopMapper["22/1"] = "22";
    m_FStopMapper["25/1"] = "25";
    m_FStopMapper["29/1"] = "29";

    m_FStopMapper["7/2"] = "3.5";
    m_FStopMapper["9/2"] = "4.5";

    m_FStopMapper["8/5"] = "1.6";
    m_FStopMapper["9/5"] = "1.8";
    m_FStopMapper["11/5"] = "2.2";
    m_FStopMapper["12/5"] = "2.4";
    m_FStopMapper["14/5"] = "2.8";
    m_FStopMapper["28/5"] = "5.6";

    m_FStopMapper["17/10"] = "1.7";
    m_FStopMapper["18/10"] = "1.8";
    m_FStopMapper["19/10"] = "1.9";
    m_FStopMapper["20/10"] = "2";
    m_FStopMapper["23/10"] = "2.3";
    m_FStopMapper["24/10"] = "2.4";
    m_FStopMapper["25/10"] = "2.5";
    m_FStopMapper["26/10"] = "2.6";
    m_FStopMapper["27/10"] = "2.7";
    m_FStopMapper["28/10"] = "2.8";
    m_FStopMapper["29/10"] = "2.9";
    m_FStopMapper["30/10"] = "3";
    m_FStopMapper["31/10"] = "3.1";
    m_FStopMapper["32/10"] = "3.2";
    m_FStopMapper["33/10"] = "3.3";
    m_FStopMapper["34/10"] = "3.4";
    m_FStopMapper["35/10"] = "3.5";
    m_FStopMapper["36/10"] = "3.6";
    m_FStopMapper["37/10"] = "3.7";
    m_FStopMapper["38/10"] = "3.8";
    m_FStopMapper["40/10"] = "4";
    m_FStopMapper["41/10"] = "4.1";
    m_FStopMapper["42/10"] = "4.2";
    m_FStopMapper["43/10"] = "4.3";
    m_FStopMapper["44/10"] = "4.4";
    m_FStopMapper["45/10"] = "4.5";
    m_FStopMapper["46/10"] = "4.6";
    m_FStopMapper["47/10"] = "4.7";
    m_FStopMapper["48/10"] = "4.8";
    m_FStopMapper["49/10"] = "4.9";
    m_FStopMapper["50/10"] = "5";
    m_FStopMapper["51/10"] = "5.1";
    m_FStopMapper["53/10"] = "5.3";
    m_FStopMapper["56/10"] = "5.6";
    m_FStopMapper["57/10"] = "5.7";
    m_FStopMapper["58/10"] = "5.8";
    m_FStopMapper["59/10"] = "5.9";
    m_FStopMapper["63/10"] = "6.3";
    m_FStopMapper["65/10"] = "6.5";
    m_FStopMapper["66/10"] = "6.6";
    m_FStopMapper["67/10"] = "6.7";
    m_FStopMapper["70/10"] = "7.0";
    m_FStopMapper["71/10"] = "7.1";
    m_FStopMapper["74/10"] = "7.4";
    m_FStopMapper["76/10"] = "7.6";
    m_FStopMapper["77/10"] = "7.7";
    m_FStopMapper["80/10"] = "8";
    m_FStopMapper["81/10"] = "8.1";
    m_FStopMapper["90/10"] = "9.0";
    m_FStopMapper["95/10"] = "9.5";
    m_FStopMapper["100/10"] = "10";
    m_FStopMapper["110/10"] = "11";
    m_FStopMapper["130/10"] = "13";
    m_FStopMapper["180/10"] = "18";
    m_FStopMapper["220/10"] = "22";
    m_FStopMapper["250/10"] = "25";

    m_FStopMapper["41/25"] = "1.6";

    m_FStopMapper["150/100"] = "1.5";

    m_FStopMapper["165/100"] = "1.7";
    m_FStopMapper["170/100"] = "1.7";
    m_FStopMapper["179/100"] = "1.8";
    m_FStopMapper["180/100"] = "1.8";
    m_FStopMapper["185/100"] = "1.9";
    m_FStopMapper["189/100"] = "1.9";
    m_FStopMapper["190/100"] = "1.9";
    m_FStopMapper["200/100"] = "2";
    m_FStopMapper["220/100"] = "2.2";
    m_FStopMapper["240/100"] = "2.4";
    m_FStopMapper["260/100"] = "2.6";
    m_FStopMapper["265/100"] = "2.7";
    m_FStopMapper["270/100"] = "2.7";
    m_FStopMapper["280/100"] = "2.8";
    m_FStopMapper["288/100"] = "2.9";
    m_FStopMapper["310/100"] = "3.1";
    m_FStopMapper["317/100"] = "3.2";
    m_FStopMapper["330/100"] = "3.3";
    m_FStopMapper["340/100"] = "3.4";
    m_FStopMapper["350/100"] = "3.5";
    m_FStopMapper["360/100"] = "3.6";
    m_FStopMapper["380/100"] = "3.8";
    m_FStopMapper["390/100"] = "3.9";
    m_FStopMapper["400/100"] = "4";
    m_FStopMapper["403/100"] = "4";
    m_FStopMapper["425/100"] = "4.3";
    m_FStopMapper["450/100"] = "4.5";
    m_FStopMapper["470/100"] = "4.7";
    m_FStopMapper["500/100"] = "5";
    m_FStopMapper["550/100"] = "5.5";
    m_FStopMapper["700/100"] = "7";
    m_FStopMapper["800/100"] = "8";
    m_FStopMapper["870/100"] = "8.7";
    m_FStopMapper["900/100"] = "9";
    m_FStopMapper["950/100"] = "9.5";
    m_FStopMapper["970/100"] = "9.7";
    m_FStopMapper["2200/100"] = "22";
    m_FStopMapper["2800/1000"] = "28";

    m_FStopMapper["358/128"] = "2.8";

    m_FStopMapper["3100/1000"] = "3.1";

    m_FStopMapper["17000/10000"] = "1.7";
    m_FStopMapper["22000/10000"] = "2.2";
    m_FStopMapper["24000/10000"] = "2.4";

    m_FStopMapper["200000/100000"] = "2";
    m_FStopMapper["240000/100000"] = "2.4";

    m_FStopMapper["1244236/699009"] = "1.8";

    m_FStopMapper["2000000/1000000"] = "2";
    m_FStopMapper["4400000/1000000"] = "4.4";
    m_FStopMapper["4500000/1000000"] = "4.5";

    m_FStopMapper["6606029/1048576"] = "6.3";

    m_FStopMapper["939524096/67108864"] = "14";

    m_FStopMapper["4294967295/766958458"] = "5.6";
    m_FStopMapper["4294967295/954437176"] = "4.5";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Focal length mapper
void ExifInfo::Init_FocalLengthMapper()
{
    CALL_IN("");

    m_FocalLengthMapper["0/1"] = "";


    for (int counter = 3;
         counter < 500;
         counter++)
    {
        const QString key = QString::number(counter) + "/1";
        m_FocalLengthMapper[key] = key;
    }
    m_FocalLengthMapper["500/1"] = "500";
    m_FocalLengthMapper["550/1"] = "550";
    m_FocalLengthMapper["560/1"] = "560";
    m_FocalLengthMapper["600/1"] = "600";
    m_FocalLengthMapper["840/1"] = "840";

    m_FocalLengthMapper["11/2"] = "5.5";
    m_FocalLengthMapper["15/2"] = "7.5";
    m_FocalLengthMapper["17/2"] = "8.5";

    m_FocalLengthMapper["17/4"] = "4.3";

    m_FocalLengthMapper["21/5"] = "4.2";
    m_FocalLengthMapper["24/5"] = "4.8";
    m_FocalLengthMapper["29/5"] = "5.8";
    m_FocalLengthMapper["33/5"] = "6.6";
    m_FocalLengthMapper["67/5"] = "13.4";

    for (int counter = 3;
         counter < 500;
         counter++)
    {
        const QString key = QString::number(counter) + "/10";
        m_FocalLengthMapper[key] = QString::number(counter * .1);
    }
    m_FocalLengthMapper["500/10"] = "50";
    m_FocalLengthMapper["550/10"] = "55";
    m_FocalLengthMapper["559/10"] = "55.9";
    m_FocalLengthMapper["570/10"] = "5.7";
    m_FocalLengthMapper["600/10"] = "60";
    m_FocalLengthMapper["608/10"] = "61";
    m_FocalLengthMapper["630/10"] = "63";
    m_FocalLengthMapper["684/10"] = "68.4";
    m_FocalLengthMapper["693/10"] = "69.3";
    m_FocalLengthMapper["700/10"] = "70";
    m_FocalLengthMapper["736/10"] = "73.6";
    m_FocalLengthMapper["850/10"] = "85";
    m_FocalLengthMapper["870/10"] = "87";
    m_FocalLengthMapper["1000/10"] = "100";
    m_FocalLengthMapper["1040/10"] = "104";
    m_FocalLengthMapper["1050/10"] = "105";
    m_FocalLengthMapper["1100/10"] = "110";
    m_FocalLengthMapper["1400/10"] = "140";
    m_FocalLengthMapper["1500/10"] = "150";
    m_FocalLengthMapper["1580/10"] = "158";
    m_FocalLengthMapper["1600/10"] = "160";
    m_FocalLengthMapper["1650/10"] = "165";
    m_FocalLengthMapper["1750/10"] = "175";
    m_FocalLengthMapper["1800/10"] = "180";
    m_FocalLengthMapper["1850/10"] = "185";
    m_FocalLengthMapper["2000/10"] = "200";
    m_FocalLengthMapper["2100/10"] = "210";
    m_FocalLengthMapper["2200/10"] = "220";
    m_FocalLengthMapper["2300/10"] = "230";
    m_FocalLengthMapper["2400/10"] = "240";
    m_FocalLengthMapper["2800/10"] = "280";
    m_FocalLengthMapper["2900/10"] = "290";
    m_FocalLengthMapper["3000/10"] = "300";
    m_FocalLengthMapper["3100/10"] = "310";
    m_FocalLengthMapper["3300/10"] = "330";
    m_FocalLengthMapper["3400/10"] = "340";
    m_FocalLengthMapper["3600/10"] = "360";
    m_FocalLengthMapper["4000/10"] = "400";
    m_FocalLengthMapper["4600/10"] = "460";
    m_FocalLengthMapper["4900/10"] = "490";
    m_FocalLengthMapper["5000/10"] = "500";
    m_FocalLengthMapper["5500/10"] = "550";
    m_FocalLengthMapper["5600/10"] = "560";
    m_FocalLengthMapper["6000/10"] = "600";
    m_FocalLengthMapper["8500/10"] = "850";

    m_FocalLengthMapper["125/16"] = "7.8";

    m_FocalLengthMapper["31/20"] = "1.6";
    m_FocalLengthMapper["53/20"] = "2.7";
    m_FocalLengthMapper["77/20"] = "3.9";
    m_FocalLengthMapper["83/20"] = "4.2";
    m_FocalLengthMapper["2259/20"] = "113";

    m_FocalLengthMapper["103/25"] = "4.1";
    m_FocalLengthMapper["107/25"] = "4.3";

    m_FocalLengthMapper["173/32"] = "5.4";
    m_FocalLengthMapper["186/32"] = "5.8";
    m_FocalLengthMapper["189/32"] = "5.9";
    m_FocalLengthMapper["224/32"] = "7";
    m_FocalLengthMapper["227/32"] = "7.1";
    m_FocalLengthMapper["250/32"] = "7.8";
    m_FocalLengthMapper["301/32"] = "9.4";
    m_FocalLengthMapper["314/32"] = "9.8";
    m_FocalLengthMapper["342/32"] = "10.7";
    m_FocalLengthMapper["362/32"] = "11.3";
    m_FocalLengthMapper["400/32"] = "12.5";
    m_FocalLengthMapper["461/32"] = "14.4";
    m_FocalLengthMapper["682/32"] = "21.3";

    m_FocalLengthMapper["0/100"] = "";
    m_FocalLengthMapper["220/100"] = "2.2";
    m_FocalLengthMapper["271/100"] = "2.7";
    m_FocalLengthMapper["279/100"] = "2.8";
    m_FocalLengthMapper["331/100"] = "3.3";
    m_FocalLengthMapper["350/100"] = "3.5";
    m_FocalLengthMapper["354/100"] = "3.5";
    m_FocalLengthMapper["360/100"] = "3.6";
    m_FocalLengthMapper["369/100"] = "3.7";
    m_FocalLengthMapper["370/100"] = "3.7";
    m_FocalLengthMapper["382/100"] = "3.8";
    m_FocalLengthMapper["399/100"] = "4";
    m_FocalLengthMapper["403/100"] = "4";
    m_FocalLengthMapper["405/100"] = "4.1";
    m_FocalLengthMapper["410/100"] = "4.1";
    m_FocalLengthMapper["413/100"] = "4.1";
    m_FocalLengthMapper["420/100"] = "4.2";
    m_FocalLengthMapper["425/100"] = "4.3";
    m_FocalLengthMapper["430/100"] = "4.3";
    m_FocalLengthMapper["431/100"] = "4.3";
    m_FocalLengthMapper["442/100"] = "4.4";
    m_FocalLengthMapper["460/100"] = "4.6";
    m_FocalLengthMapper["467/100"] = "4.7";
    m_FocalLengthMapper["480/100"] = "4.8";
    m_FocalLengthMapper["490/100"] = "4.9";
    m_FocalLengthMapper["500/100"] = "5";
    m_FocalLengthMapper["514/100"] = "5.1";
    m_FocalLengthMapper["523/100"] = "5.2";
    m_FocalLengthMapper["535/100"] = "5.4";
    m_FocalLengthMapper["543/100"] = "5.4";
    m_FocalLengthMapper["570/100"] = "5.7";
    m_FocalLengthMapper["580/100"] = "5.8";
    m_FocalLengthMapper["585/100"] = "5.9";
    m_FocalLengthMapper["587/100"] = "5.9";
    m_FocalLengthMapper["590/100"] = "5.9";
    m_FocalLengthMapper["591/100"] = "5.9";
    m_FocalLengthMapper["600/100"] = "6";
    m_FocalLengthMapper["610/100"] = "6.1";
    m_FocalLengthMapper["620/100"] = "6.2";
    m_FocalLengthMapper["630/100"] = "6.3";
    m_FocalLengthMapper["633/100"] = "6.3";
    m_FocalLengthMapper["650/100"] = "6.5";
    m_FocalLengthMapper["660/100"] = "6.6";
    m_FocalLengthMapper["663/100"] = "6.6";
    m_FocalLengthMapper["670/100"] = "6.7";
    m_FocalLengthMapper["750/100"] = "7.5";
    m_FocalLengthMapper["780/100"] = "7.8";
    m_FocalLengthMapper["790/100"] = "7.9";
    m_FocalLengthMapper["800/100"] = "8";
    m_FocalLengthMapper["820/100"] = "8.2";
    m_FocalLengthMapper["840/100"] = "8.4";
    m_FocalLengthMapper["882/100"] = "8.8";
    m_FocalLengthMapper["1100/100"] = "11";
    m_FocalLengthMapper["1270/100"] = "12.7";
    m_FocalLengthMapper["1510/100"] = "15.1";
    m_FocalLengthMapper["1700/100"] = "17";
    m_FocalLengthMapper["1712/100"] = "17.1";
    m_FocalLengthMapper["1820/100"] = "18.2";
    m_FocalLengthMapper["1860/100"] = "18.6";
    m_FocalLengthMapper["2300/100"] = "23";
    m_FocalLengthMapper["2510/100"] = "25.1";
    m_FocalLengthMapper["3400/100"] = "34";
    m_FocalLengthMapper["3500/100"] = "35";
    m_FocalLengthMapper["4500/100"] = "45";
    m_FocalLengthMapper["4750/100"] = "47.5";
    m_FocalLengthMapper["5000/100"] = "50";
    m_FocalLengthMapper["5300/100"] = "53";
    m_FocalLengthMapper["5500/100"] = "55";
    m_FocalLengthMapper["5600/100"] = "56";
    m_FocalLengthMapper["6330/100"] = "63.3";
    m_FocalLengthMapper["15000/100"] = "150";

    m_FocalLengthMapper["755/128"] = "5.9";
    m_FocalLengthMapper["3971/256"] = "15.5";

    m_FocalLengthMapper["2940/1000"] = "2.9";
    m_FocalLengthMapper["3097/1000"] = "3.1";
    m_FocalLengthMapper["3170/1000"] = "3.2";
    m_FocalLengthMapper["3200/1000"] = "3.2";
    m_FocalLengthMapper["3620/1000"] = "3.6";
    m_FocalLengthMapper["3820/1000"] = "3.8";
    m_FocalLengthMapper["3830/1000"] = "3.8";
    m_FocalLengthMapper["4000/1000"] = "4";
    m_FocalLengthMapper["4300/1000"] = "4.3";
    m_FocalLengthMapper["4442/1000"] = "4.4";
    m_FocalLengthMapper["4499/1000"] = "4.5";
    m_FocalLengthMapper["4500/1000"] = "4.5";
    m_FocalLengthMapper["4600/1000"] = "4.6";
    m_FocalLengthMapper["4710/1000"] = "4.7";
    m_FocalLengthMapper["4740/1000"] = "4.7";
    m_FocalLengthMapper["5000/1000"] = "5";
    m_FocalLengthMapper["5400/1000"] = "5.4";
    m_FocalLengthMapper["5583/1000"] = "5.6";
    m_FocalLengthMapper["5700/1000"] = "5.7";
    m_FocalLengthMapper["5800/1000"] = "5.8";
    m_FocalLengthMapper["5854/1000"] = "5.9";
    m_FocalLengthMapper["5900/1000"] = "5.9";
    m_FocalLengthMapper["5989/1000"] = "6";
    m_FocalLengthMapper["6000/1000"] = "6";
    m_FocalLengthMapper["6100/1000"] = "6.1";
    m_FocalLengthMapper["6190/1000"] = "6.2";
    m_FocalLengthMapper["6200/1000"] = "6.2";
    m_FocalLengthMapper["6300/1000"] = "6.3";
    m_FocalLengthMapper["6447/1000"] = "6.4";
    m_FocalLengthMapper["6600/1000"] = "6.6";
    m_FocalLengthMapper["6769/1000"] = "6.8";
    m_FocalLengthMapper["6810/1000"] = "6.8";
    m_FocalLengthMapper["7300/1000"] = "7.3";
    m_FocalLengthMapper["7400/1000"] = "7.4";
    m_FocalLengthMapper["7700/1000"] = "7.7";
    m_FocalLengthMapper["7947/1000"] = "7.9";
    m_FocalLengthMapper["8205/1000"] = "8.2";
    m_FocalLengthMapper["9954/1000"] = "10";
    m_FocalLengthMapper["12074/1000"] = "12.1";
    m_FocalLengthMapper["12669/1000"] = "12.7";
    m_FocalLengthMapper["12845/1000"] = "12.8";
    m_FocalLengthMapper["13300/1000"] = "13.3";
    m_FocalLengthMapper["13600/1000"] = "13.6";
    m_FocalLengthMapper["14783/1000"] = "14.8";
    m_FocalLengthMapper["14900/1000"] = "14.9";
    m_FocalLengthMapper["14926/1000"] = "14.9";
    m_FocalLengthMapper["15673/1000"] = "15.7";
    m_FocalLengthMapper["20000/1000"] = "20";
    m_FocalLengthMapper["20100/1000"] = "20.1";
    m_FocalLengthMapper["21556/1000"] = "21.6";
    m_FocalLengthMapper["23280/1000"] = "23.3";
    m_FocalLengthMapper["34900/1000"] = "34.9";
    m_FocalLengthMapper["44400/1000"] = "44.4";
    m_FocalLengthMapper["50000/1000"] = "50";
    m_FocalLengthMapper["72000/1000"] = "72";

    m_FocalLengthMapper["251773/37217"] = "6.7";

    m_FocalLengthMapper["2497280/65536"] = "38.1";

    m_FocalLengthMapper["469865/174671"] = "2.7";

    m_FocalLengthMapper["3302983/524283"] = "6.3";

    m_FocalLengthMapper["880803840/8388608"] = "105";

    m_FocalLengthMapper["6300000/1000000"] = "6.3";
    m_FocalLengthMapper["150000000/1000000"] = "150";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Exposure time mapper
void ExifInfo::Init_ExposureTimeMapper()
{
    CALL_IN("");

    // For values from Photo.ExposureTime

    m_ExposureTimeMapper["0/1"] = "";

    // Note that exposure times of the format "1/n" are handled separately.

    m_ExposureTimeMapper["2/39"] = "1/20";
    m_ExposureTimeMapper["3/10"] = "0.3";
    m_ExposureTimeMapper["4/10"] = "0.4";
    m_ExposureTimeMapper["5/1"] = "5";
    m_ExposureTimeMapper["5/2"] = "2.5";
    m_ExposureTimeMapper["5/10"] = "0.5";
    m_ExposureTimeMapper["5/300"] = "1/60";
    m_ExposureTimeMapper["6/1"] = "6";
    m_ExposureTimeMapper["6/10"] = "0.6";
    m_ExposureTimeMapper["8/10"] = "0.8";

    m_ExposureTimeMapper["10/1"] = "10";
    m_ExposureTimeMapper["10/10"] = "1";
    m_ExposureTimeMapper["10/50"] = "1/5";
    m_ExposureTimeMapper["10/57"] = "1/6";
    m_ExposureTimeMapper["10/60"] = "1/6";
    m_ExposureTimeMapper["10/70"] = "1/7";
    m_ExposureTimeMapper["10/80"] = "1/8";
    m_ExposureTimeMapper["10/100"] = "1/10";
    m_ExposureTimeMapper["10/160"] = "1/16";
    m_ExposureTimeMapper["10/200"] = "1/20";
    m_ExposureTimeMapper["10/250"] = "1/25";
    m_ExposureTimeMapper["10/300"] = "1/30";
    m_ExposureTimeMapper["10/320"] = "1/32";
    m_ExposureTimeMapper["10/340"] = "1/34";
    m_ExposureTimeMapper["10/376"] = "1/38";
    m_ExposureTimeMapper["10/400"] = "1/40";
    m_ExposureTimeMapper["10/450"] = "1/45";
    m_ExposureTimeMapper["10/500"] = "1/50";
    m_ExposureTimeMapper["10/600"] = "1/60";
    m_ExposureTimeMapper["10/601"] = "1/60";
    m_ExposureTimeMapper["10/603"] = "1/60";
    m_ExposureTimeMapper["10/700"] = "1/70";
    m_ExposureTimeMapper["10/750"] = "1/75";
    m_ExposureTimeMapper["10/800"] = "1/80";
    m_ExposureTimeMapper["10/833"] = "1/83";
    m_ExposureTimeMapper["10/1000"] = "1/100";
    m_ExposureTimeMapper["10/1050"] = "1/105";
    m_ExposureTimeMapper["10/1250"] = "1/125";
    m_ExposureTimeMapper["10/1265"] = "1/127";
    m_ExposureTimeMapper["10/1600"] = "1/160";
    m_ExposureTimeMapper["10/2000"] = "1/200";
    m_ExposureTimeMapper["10/2500"] = "1/250";
    m_ExposureTimeMapper["10/3200"] = "1/320";
    m_ExposureTimeMapper["10/3500"] = "1/350";
    m_ExposureTimeMapper["10/4000"] = "1/400";
    m_ExposureTimeMapper["10/5000"] = "1/500";
    m_ExposureTimeMapper["10/6400"] = "1/640";
    m_ExposureTimeMapper["10/8000"] = "1/800";
    m_ExposureTimeMapper["10/10000"] = "1/1000";
    m_ExposureTimeMapper["10/12500"] = "1/1250";
    m_ExposureTimeMapper["10/16000"] = "1/1600";
    m_ExposureTimeMapper["10/20000"] = "1/2000";

    m_ExposureTimeMapper["13/1"] = "13";
    m_ExposureTimeMapper["13/10"] = "1.3";
    m_ExposureTimeMapper["15/1"] = "15";
    m_ExposureTimeMapper["16/10"] = "1.6";
    m_ExposureTimeMapper["20/1"] = "20";
    m_ExposureTimeMapper["20/10"] = "2";
    m_ExposureTimeMapper["25/10"] = "2.5";
    m_ExposureTimeMapper["30/1"] = "30";
    m_ExposureTimeMapper["32/10"] = "3.2";
    m_ExposureTimeMapper["36/100000"] = "1/2700";
    m_ExposureTimeMapper["38/10"] = "3.8";
    m_ExposureTimeMapper["89/1"] = "89";

    m_ExposureTimeMapper["100/599"] = "1/6";
    m_ExposureTimeMapper["120/1"] = "120";
    m_ExposureTimeMapper["285/10000"] = "1/35";
    m_ExposureTimeMapper["360/9450"] = "1/26";
    m_ExposureTimeMapper["400/10000"] = "1/25";
    m_ExposureTimeMapper["403/10"] = "40";
    m_ExposureTimeMapper["416/10000"] = "1/24";
    m_ExposureTimeMapper["833/100000"] = "1/120";
    m_ExposureTimeMapper["866/100000"] = "1/115";

    m_ExposureTimeMapper["1008/1000000"] = "1/1000";
    m_ExposureTimeMapper["1250/10000"] = "1/8";
    m_ExposureTimeMapper["1666/100000"] = "1/60";
    m_ExposureTimeMapper["2499/100000"] = "1/40";
    m_ExposureTimeMapper["3125/1000000"] = "1/300";
    m_ExposureTimeMapper["3261/100000"] = "1/31";
    m_ExposureTimeMapper["4000/1000000"] = "1/250";
    m_ExposureTimeMapper["5000/1000000"] = "1/200";
    m_ExposureTimeMapper["8000/1000000"] = "1/125";
    m_ExposureTimeMapper["8335/1000000"] = "1/12";
    m_ExposureTimeMapper["8400/1000000"] = "1/12";
    m_ExposureTimeMapper["8904/1000000"] = "1/11";
    m_ExposureTimeMapper["9997/1000000"] = "1/100";

    m_ExposureTimeMapper["10000/1000000"] = "1/100";
    m_ExposureTimeMapper["10000/3367003"] = "1/337";
    m_ExposureTimeMapper["15625/1000000"] = "1/64";
    m_ExposureTimeMapper["16667/1000000"] = "1/60";
    m_ExposureTimeMapper["20000/1000000"] = "1/50";
    m_ExposureTimeMapper["20001/1000000"] = "1/50";
    m_ExposureTimeMapper["20166/1000000"] = "1/50";
    m_ExposureTimeMapper["20339/1000000"] = "1/50";
    m_ExposureTimeMapper["25000/1000000"] = "1/40";
    m_ExposureTimeMapper["29000/1000000"] = "1/34";
    m_ExposureTimeMapper["32062/1000000"] = "1/31";
    m_ExposureTimeMapper["33000/1000000"] = "1/30";
    m_ExposureTimeMapper["33333/1000000"] = "1/30";
    m_ExposureTimeMapper["39926/1000000"] = "1/25";
    m_ExposureTimeMapper["40000/1000000"] = "1/25";
    m_ExposureTimeMapper["63151/1000000"] = "1/16";
    m_ExposureTimeMapper["69951/1000000"] = "1/14";
    m_ExposureTimeMapper["84857/1000000"] = "1/12";
    m_ExposureTimeMapper["90000/1000000"] = "1/11";

    m_ExposureTimeMapper["100000/1000000"] = "1/10";

    m_ExposureTimeMapper["1666667/100000000"] = "1/60";
    m_ExposureTimeMapper["639132/19173959"] = "1/30";

    m_ExposureTimeMapper["8947849/536870912"] = "1/60";

    m_ExposureTimeMapper["16666667/1000000000"] = "1/60";
    m_ExposureTimeMapper["134217728/536870912"] = "1/4";

    m_ExposureTimeMapper["6604300/1000000000"] = "1/151";
    m_ExposureTimeMapper["7845866/1000000000"] = "1/127";
    m_ExposureTimeMapper["8315366/1000000000"] = "1/120";
    m_ExposureTimeMapper["29997000/1000000000"] = "1/33";
    m_ExposureTimeMapper["40004000/1000000000"] = "1/25";

    m_ExposureTimeMapper["3435973/4294967295"] = "1/1250";
    m_ExposureTimeMapper["10737417/4294967295"] = "1/400";
    m_ExposureTimeMapper["11184811/67108864"] = "1/6";
    m_ExposureTimeMapper["17179869/4294967295"] = "1/250";

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Mappers
QHash < QString, QString > ExifInfo::m_CameraMakerMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_CameraModelMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_LensMakerMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_LensModelMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_FStopMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_FocalLengthMapper =
    QHash < QString, QString >();
QHash < QString, QString > ExifInfo::m_ExposureTimeMapper =
    QHash < QString, QString >();



// ====================================================================== Debug



///////////////////////////////////////////////////////////////////////////////
// Dump full info
void ExifInfo::Dump() const
{
    CALL_IN("");

    qDebug().noquote() << "==== Exif Info";

    // Determine field widths
    int width_group = 0;
    int width_tag = 0;
    int width_type = 0;
    QStringList sorted_groups(m_ExifData.keyBegin(), m_ExifData.keyEnd());
    std::sort(sorted_groups.begin(), sorted_groups.end());
    for (auto group_iterator = sorted_groups.begin();
         group_iterator != sorted_groups.end();
         group_iterator++)
    {
        const QString group = *group_iterator;
        width_group = qMax(width_group, group.size());
        QStringList sorted_tags(m_ExifData[group].keyBegin(),
            m_ExifData[group].keyEnd());
        std::sort(sorted_tags.begin(), sorted_tags.end());
        for (auto tag_iterator = sorted_tags.begin();
             tag_iterator != sorted_tags.end();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            width_tag = qMax(width_tag, tag.size());
            width_type = qMax(width_type, m_ExifTypes[group][tag].size());
        }
    }

    // Show contents
    for (auto group_iterator = sorted_groups.begin();
         group_iterator != sorted_groups.end();
         group_iterator++)
    {
        const QString group = *group_iterator;
        QStringList sorted_tags(m_ExifData[group].keyBegin(),
            m_ExifData[group].keyEnd());
        std::sort(sorted_tags.begin(), sorted_tags.end());
        for (auto tag_iterator = sorted_tags.begin();
             tag_iterator != sorted_tags.end();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            const QString type = m_ExifTypes[group][tag];
            const QString value = m_ExifData[group][tag];
            qDebug().noquote()
                << group +
                    QString(" ").repeated(width_group + 1 - group.size())
                << tag + QString(" ").repeated(width_tag + 1 - tag.size())
                << type + QString(" ").repeated(width_type + 1 - type.size())
                << value;
        }
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Compile data
void ExifInfo::RegisterData()
{
    CALL_IN("");

    for (auto group_iterator = m_ExifData.keyBegin();
         group_iterator != m_ExifData.keyEnd();
         group_iterator++)
    {
        const QString group = *group_iterator;
        for (auto tag_iterator = m_ExifData[group].keyBegin();
             tag_iterator != m_ExifData[group].keyEnd();
             tag_iterator++)
        {
            const QString tag = *tag_iterator;
            const QString key = group + "." + tag;
            const QString value = m_ExifData[group][tag].trimmed();
            m_TagUsage[key][m_Filename] = value;

            if (key == "Image.Make")
            {
                if(!m_CameraMakerMapper.contains(value))
                {
                    m_NewMapperValues["CameraMaker"] += value;
                }
            }
            if (key == "Image.Model")
            {
                const QString model = GetCameraMaker() + "." + value;
                if(!m_CameraModelMapper.contains(model))
                {
                    m_NewMapperValues["CameraModel"] += model;
                }
            }
            if (key == "Photo.LensMake")
            {
                if(!m_LensMakerMapper.contains(value))
                {
                    m_NewMapperValues["LensMaker"] += value;
                }
            }
            if (key == "Photo.LensModel")
            {
                const QString model = GetLensMaker() + "." + value;
                if(!m_LensModelMapper.contains(model))
                {
                    m_NewMapperValues["LensModel"] += model;
                }
            }
            if (key == "Photo.FNumber")
            {
                if (!m_FStopMapper.contains(value))
                {
                    m_NewMapperValues["FStop"] += value;
                }
            }
            if (key == "Photo.FocalLength")
            {
                if (!m_FocalLengthMapper.contains(value))
                {
                    m_NewMapperValues["FocalLength"] += value;
                }
            }
            if (key == "Photo.ExposureTime")
            {
                // Exposure times of the format "1/n" are handled separately
                if (!value.startsWith("1/") &&
                    !m_ExposureTimeMapper.contains(value))
                {
                    m_NewMapperValues["ExposureTime"] += value;
                }
            }

        }
    }


    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
QHash < QString, QHash < QString, QString > > ExifInfo::m_TagUsage;
QHash < QString, QSet < QString > > ExifInfo::m_NewMapperValues;



///////////////////////////////////////////////////////////////////////////////
// Dump compiled statistics
void ExifInfo::Dump_CompliedStatistics()
{
    CALL_IN("");

     qDebug().noquote() << "==== Aggregate Exif Tags";

    // Determine field widths
    int width_key = 0;
    QStringList sorted_keys(m_TagUsage.keyBegin(), m_TagUsage.keyEnd());
    std::sort(sorted_keys.begin(), sorted_keys.end());
    for (auto key_iterator = sorted_keys.begin();
         key_iterator != sorted_keys.end();
         key_iterator++)
    {
        const QString key = *key_iterator;
        width_key = qMax(width_key, key.size());
    }

    // Show contents
    for (auto key_iterator = sorted_keys.begin();
         key_iterator != sorted_keys.end();
         key_iterator++)
    {
        const QString key = *key_iterator;
        qDebug().noquote()
            << key + QString(" ").repeated(width_key + 1 - key.size())
            << m_TagUsage[key].size();
        for (auto file_iterator = m_TagUsage[key].keyBegin();
             file_iterator != m_TagUsage[key].keyEnd();
             file_iterator++)
        {
            const QString file = *file_iterator;
            const QString value = m_TagUsage[key][file];
            qDebug().noquote() << "\t\t" << file << "\t\t" << value;
        }
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Dump new values for mappers
void ExifInfo::Dump_NewMapperValues()
{
    CALL_IN("");

    if (m_NewMapperValues.isEmpty())
    {
        // Nothing to do
        CALL_OUT("");
        return;
    }

    qDebug().noquote() << "==== New mapper values";
    for (auto mapper_iterator = m_NewMapperValues.keyBegin();
         mapper_iterator != m_NewMapperValues.keyEnd();
         mapper_iterator++)
    {
        const QString mapper = *mapper_iterator;
        qDebug().noquote() << mapper;
        QStringList values(m_NewMapperValues[mapper].begin(),
            m_NewMapperValues[mapper].end());
        std::sort(values.begin(), values.end());
        qDebug().noquote() << QString("\t%1\n").arg(values.join("\n\t"));
    }

    CALL_OUT("");
}
