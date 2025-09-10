// ExifInfo.h
// Class definition file

// Avoid duplicate includes
#ifndef EXIFINFO_H
#define EXIFINFO_H

// Qt includes
#include <QHash>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QString>



// Class definition
class ExifInfo:
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:

    // Constructor (never to be called)
    ExifInfo();

public:
    // Destructor
    virtual ~ExifInfo();

    // Factory
    static ExifInfo * CreateExifInfo(const QString & mcrFilename);

private:
    // File name
    QString m_Filename;

    // Exif data
    QHash < QString, QHash < QString, QString > > m_ExifTypes;
    QHash < QString, QHash < QString, QString > > m_ExifData;



    // ======================================================= EXIF Data Access
public:
    // Get MIME type
    QString GetMIMEType() const;
private:
    QString m_MIMEType;

public:
    // Check if a particular tag is defined
    bool HasTag(const QString & mcrGroup, const QString & mcrTag) const;

    // Get a particular tag value
    QString GetValue(const QString & mcrGroup, const QString & mcrTag,
        const QString & mcrDefault = QString()) const;



    // ======= Camera

    // Camera Maker
    QString GetCameraMaker() const;

    // Camera Model
    QString GetCameraModel() const;

    // Unknown camera model?
    bool IsCameraModelKnown() const;


    // ======= Lens

    // Lens Maker
    QString GetLensMaker() const;

    // Lens Model
    QString GetLensModel() const;

    // Lens Min Focal Length
    QString GetLensMinFocalLength() const;

    // Lens Max Focal Length
    QString GetLensMaxFocalLength() const;

    // Lens Min F Stop at minimum focal length
    QString GetLensMinFStopAtMinFocalLength() const;

    // Lens Min F Stop at maximum focal length
    QString GetLensMinFStopAtMaxFocalLength() const;


    // ======= Exposure

    // Exposure: Date & Time
    QString GetExposureDateTime() const;

    // Exposure: Focal Length
    QString GetExposureFocalLength() const;

    // Exposure: F Stop
    QString GetExposureFStop() const;

    // Exposure: Time
    QString GetExposureTime() const;

    // Exposure: Bias
    QString GetExposureBias() const;

    // Exposure: ISO Rating
    QString GetISORating() const;

    // Exposure: Subject Distance
    QString GetExposureSubjectDistance() const;

    // Flash Fired?
    QString GetFlashFired() const;

    // Flash Bias
    QString GetFlashBias() const;

    // Picture Number
    QString GetPictureNumber() const;


    // ======= Image

    // Width of real picture
    QString GetRealPictureWidth() const;

    // Height of real picture
    QString GetRealPictureHeight() const;

    // Orientation
    int GetOrientation() const;

    // Subject area
    QString GetSubjectArea() const;

    // Software
    QString GetSoftware() const;


    // ======= Owner/Copyright

    // Owner
    QString GetOwner() const;

    // Camera serial number
    QString GetCameraSerialNumber() const;


    // ======= GPS Position

    // Latitude
    double GetGPSLatitude() const;

    // Longitude
    double GetGPSLongitude() const;

    // Elevation
    double GetGPSElevation() const;

    // Direction
    double GetGPSDirection() const;


    // ======= Temperature
    double GetCameraTemperature() const;

private:
    // Convert rational to string
    QString ConvertRational(const QString & mcrValue) const;



    // ================================================================ Mappers
private:
    static bool InitializeMappers();
    static const bool m_MappersInitialized;

    static void Init_CameraMakerMapper();
    static void Init_CameraModelMapper();
    static void Init_LensMakerMapper();
    static void Init_LensModelMapper();
    static void Init_FStopMapper();
    static void Init_FocalLengthMapper();
    static void Init_ExposureTimeMapper();

    static QHash < QString, QString > m_CameraMakerMapper;
    static QHash < QString, QString > m_CameraModelMapper;
    static QHash < QString, QString > m_LensMakerMapper;
    static QHash < QString, QString > m_LensModelMapper;
    static QHash < QString, QString > m_FStopMapper;
    static QHash < QString, QString > m_FocalLengthMapper;
    static QHash < QString, QString > m_ExposureTimeMapper;



    // ================================================================== Debug
public:
    // Dump full info
    void Dump() const;

private:
    // Compile data
    void RegisterData();
    static QHash < QString, QHash < QString, QString > > m_TagUsage;
    static QHash < QString, QSet < QString > > m_NewMapperValues;

public:
    // Dump compiled statistics
    static void Dump_CompliedStatistics();

    // Dump new values for mappers
    static void Dump_NewMapperValues();
};

#endif

