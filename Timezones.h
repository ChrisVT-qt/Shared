// Timezones.h
// Class definition

#ifndef TIMEZONES_H
#define TIMEZONES_H

// Qt includes
#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPair>
#include <QString>

// Define class
class Timezones:
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Constructor
    Timezones();

public:
    // Instanciator
    static Timezones * Instance();
private:
    static Timezones * m_Instance;

    // Destructor
    virtual ~Timezones();

private:
    void Initialize();

    // Timezones
    QHash < QString, QString > m_TimezoneToOffset;
    QHash < QString, QString > m_OffsetToTimezone;
    QHash < QString, int > m_OffsetToSeconds;



    // ============================================================= Everything
public:
    // Known timezones
    QList < QPair < QString, QString > > KnownTimezones();

    // Check if a timezone is known
    bool IsKnownTimezone(const QString mcTimezoneName);

    // Convert timezone name to offset to GMT
    QString GetTimezoneOffsetToGMT(const QString mcTimezoneName);

    // Convert date/time from a timzone to GMT
    QPair < QString, QString > ConvertTimezoneToGMT(
        const QString mcDateTimezone, const QString mcTimeTimezone,
        const QString mcOffsetToGMT);
    QDateTime ConvertTimezoneToGMT(const QDateTime mcDateTimeTimezone,
        QString mcOffsetToGMT);

    // Convert date/time from GMT to a new timezone
    QPair < QString, QString > ConvertGMTToTimezone(
        const QString mcDateGMT, const QString mcTimeGMT,
        const QString mcOffsetToGMT);
    QDateTime ConvertGMTToTimezone(const QDateTime mcDateTimeGMT,
        QString mcOffsetToGMT);
};

#endif

