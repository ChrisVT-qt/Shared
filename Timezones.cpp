// Timezones.cpp
// Class implementation file

// Project includes
#include "CallTracer.h"
#include "Timezones.h"

// Qt includes
#include <QDebug>
#include <QRegularExpression>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Default constructor (never to be called from outside)
Timezones::Timezones()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Initialize known timezones
    Initialize();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Instanciator
Timezones * Timezones::Instance()
{
    CALL_IN("");

    if (!m_Instance)
    {
        m_Instance = new Timezones;
    }
    CALL_OUT("");
    return m_Instance;
}



///////////////////////////////////////////////////////////////////////////////
// Instance
Timezones * Timezones::m_Instance = nullptr;



///////////////////////////////////////////////////////////////////////////////
// Destructor
Timezones::~Timezones()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Nothing to do, either.

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Initialize timezones
void Timezones::Initialize()
{
    CALL_IN("");

    m_OffsetToTimezone["-10:00"] = tr("Hawaii Standard Time");
    m_OffsetToTimezone["-08:00"] = tr("Pacific Standard Time");
    m_OffsetToTimezone["-07:00"] = tr("Pacific Daylight Saving Time");
    m_OffsetToTimezone["-06:00"] = tr("Central Standard Time");
    m_OffsetToTimezone["-05:00"] = tr("Central Daylight Saving Time");
    m_OffsetToTimezone["-05:00"] = tr("Eastern Standard Time");
    m_OffsetToTimezone["-04:00"] = tr("Eastern Daylight Saving Time");
    m_OffsetToTimezone[ "00:00"] = tr("Greenich Mean Time");
    m_OffsetToTimezone["+01:00"] = tr("Central European Summer Time");
    m_OffsetToTimezone["+02:00"] = tr("Central European Time");
    m_OffsetToTimezone["+03:00"] = tr("UNKNOWN");
    m_OffsetToTimezone["+08:00"] = tr("Hong Kong Time");
    m_OffsetToTimezone["+09:00"] = tr("Japan Time");
    m_OffsetToTimezone["+10:00"] = tr("Australia/Brisbane Time");
    m_OffsetToTimezone["+12:00"] = tr("New Zealand Time");

    for (auto offset_iterator = m_OffsetToTimezone.keyBegin();
         offset_iterator != m_OffsetToTimezone.keyEnd();
         offset_iterator++)
    {
        const QString offset = *offset_iterator;
        const QString timezone = m_OffsetToTimezone[offset];
        m_TimezoneToOffset[timezone] = offset;
    }

    CALL_OUT("");
}



// ================================================================= Everything



///////////////////////////////////////////////////////////////////////////////
// Known timezones
QList < QPair < QString, QString > > Timezones::KnownTimezones()
{
    CALL_IN("");

    // Sorting them is a bit of an affair...
    QList < QString > offsets_minus;
    QList < QString > offsets_no_sign;
    QList < QString > offsets_plus;
    for (auto offset_iterator = m_OffsetToTimezone.keyBegin();
         offset_iterator != m_OffsetToTimezone.keyEnd();
         offset_iterator++)
    {
        const QString offset = *offset_iterator;
        if (offset.startsWith("-"))
        {
            offsets_minus << offset;
        } else if (offset.startsWith("+"))
        {
            offsets_plus << offset;
        } else
        {
            offsets_no_sign << offset;
        }
    }

    std::sort(offsets_minus.begin(), offsets_minus.end(),
        std::greater<QString>());
    std::sort(offsets_plus.begin(), offsets_plus.end());
    QList < QString > offsets_sorted;
    offsets_sorted << offsets_minus << offsets_no_sign << offsets_plus;

    // Build return value
    QList < QPair < QString, QString > > ret;
    for (auto offset_iterator = m_OffsetToTimezone.keyBegin();
         offset_iterator != m_OffsetToTimezone.keyEnd();
         offset_iterator++)
    {
        const QString offset = *offset_iterator;
        ret << QPair < QString, QString >(offset, m_OffsetToTimezone[offset]);
    }

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a timezone is known
bool Timezones::IsKnownTimezone(const QString mcTimezoneName)
{
    CALL_IN(QString("mcTimezoneName=%1")
        .arg(CALL_SHOW(mcTimezoneName)));

    CALL_OUT("");
    return m_TimezoneToOffset.contains(mcTimezoneName);
}



///////////////////////////////////////////////////////////////////////////////
// Convert timezone name to offset to GMT
QString Timezones::GetTimezoneOffsetToGMT(const QString mcTimezoneName)
{
    CALL_IN(QString("mcTimezoneName=%1")
        .arg(CALL_SHOW(mcTimezoneName)));

    if (m_TimezoneToOffset.contains(mcTimezoneName))
    {
        CALL_OUT("");
        return m_TimezoneToOffset[mcTimezoneName];
    } else
    {
        CALL_OUT("");
        return QString();
    }

    // We never get here
}



///////////////////////////////////////////////////////////////////////////////
// Convert date/time from a timzone to GMT
QPair < QString, QString > Timezones::ConvertTimezoneToGMT(
    const QString mcDateTimezone, const QString mcTimeTimezone,
    const QString mcOffsetToGMT)
{
    CALL_IN(QString("mcDateTimeZone=%1, mcTimeTimezone=%2, mcOffsetToGMT=%3")
        .arg(CALL_SHOW(mcDateTimezone),
             CALL_SHOW(mcTimeTimezone),
             CALL_SHOW(mcOffsetToGMT)));

    QDateTime date_time = QDateTime::fromString(
        mcDateTimezone + " " + mcTimeTimezone, "yyyy-MM-dd hh:mm:ss");
    date_time = ConvertTimezoneToGMT(date_time, mcOffsetToGMT);

    // Return new timezone date and time
    CALL_OUT("");
    return QPair < QString, QString >(
        date_time.toString("yyyy-MM-dd"), date_time.toString("hh:mm:ss"));
}



///////////////////////////////////////////////////////////////////////////////
// Convert date/time from a timezone to GMT
QDateTime Timezones::ConvertTimezoneToGMT(const QDateTime mcDateTimeTimezone,
    const QString mcOffsetToGMT)
{
    CALL_IN(QString("mcDateTimeTimezone=%1, mcOffsetToGMT=%2")
        .arg(CALL_SHOW(mcDateTimeTimezone.toString("yyyy-MM-dd hh:mm:ss")),
             CALL_SHOW(mcOffsetToGMT)));

    // Offset to convert date/times to GMT
    if (!m_OffsetToSeconds.contains(mcOffsetToGMT))
    {
        static const QRegularExpression format_offset(
            "(\\+|\\-)([01][0-9]):([0-5][0-9])");
        const QRegularExpressionMatch match_offset =
            format_offset.match(mcOffsetToGMT);
        m_OffsetToSeconds[mcOffsetToGMT] =
                (match_offset.captured(1) + "1").toInt() *
            (match_offset.captured(2).toInt() * 3600 +
            match_offset.captured(3).toInt() * 60);
    }

    // Return GMT date and time
    CALL_OUT("");
    return mcDateTimeTimezone.addSecs(-m_OffsetToSeconds[mcOffsetToGMT]);
}



///////////////////////////////////////////////////////////////////////////////
// Convert date/time from GMT to a new timezone
QPair < QString, QString > Timezones::ConvertGMTToTimezone(
    const QString mcDateGMT, const QString mcTimeGMT,
    const QString mcOffsetToGMT)
{
    CALL_IN(QString("mcDateGMT=%1, mcTimeGMT=%2, mcOffseToGMT=%3")
        .arg(CALL_SHOW(mcDateGMT),
             CALL_SHOW(mcTimeGMT),
             CALL_SHOW(mcOffsetToGMT)));

    QDateTime date_time = QDateTime::fromString(
        mcDateGMT + " " + mcTimeGMT, "yyyy-MM-dd hh:mm:ss");
    date_time = ConvertGMTToTimezone(date_time, mcOffsetToGMT);

    // Return new timezone date and time
    CALL_OUT("");
    return QPair < QString, QString >(
        date_time.toString("yyyy-MM-dd"), date_time.toString("hh:mm:ss"));
}



///////////////////////////////////////////////////////////////////////////////
// Convert date/time from GMT to a new timezone
QDateTime Timezones::ConvertGMTToTimezone(const QDateTime mcDateTimeGMT,
    const QString mcOffsetToGMT)
{
    CALL_IN(QString("mcDateTimeGMT=%1, mcOffsetToGMT=%2")
        .arg(CALL_SHOW(mcDateTimeGMT.toString("yyyy-MM-dd hh:mm:ss")),
             CALL_SHOW(mcOffsetToGMT)));

    // Offset to convert date/times to local timezone
    if (!m_OffsetToSeconds.contains(mcOffsetToGMT))
    {
        static const QRegularExpression format_offset(
            "(\\+|\\-)([01][0-9]):([0-5][0-9])");
        const QRegularExpressionMatch match_offset =
            format_offset.match(mcOffsetToGMT);
        m_OffsetToSeconds[mcOffsetToGMT] =
                (match_offset.captured(1) + "1").toInt() *
            (match_offset.captured(2).toInt() * 3600 +
            match_offset.captured(3).toInt() * 60);
    }

    // Return local timezone date and time
    CALL_OUT("");
    return mcDateTimeGMT.addSecs(m_OffsetToSeconds[mcOffsetToGMT]);
}
