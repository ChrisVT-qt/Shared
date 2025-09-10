// CalendarEntry.h
// Class definition

/** \file
  * \todo Add Doxygen information
  * \todo Implement creation of calendar entries
  * \todo Implement import of Apple iCal entries
  */

#ifndef CALENDARENTRY_H
#define CALENDARENTRY_H

// Qt includes
#include <QHash>
#include <QObject>

// Class definition
class CalendarEntry
    : public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Constructor
    CalendarEntry();

public:
    // Create instace from file
    static CalendarEntry * NewCalendarEntryFromFile(const QString mcFilename);

    // Create instace from attachment data
    static CalendarEntry * NewCalendarEntry(const QString mcCalendarData);

    // Destructor
    virtual ~CalendarEntry();

private:
    QStringList m_Data;

public:
    enum CalendarEntryDetails {
        EntryCalendarScale,
        EntryCategories,
        EntryClass,
        EntryCreatedDateTimeOriginalTimezone,
        EntryCreatedDateTimeUTC,
        EntryDateTimeSentOriginalTimezone,
        EntryDateTimeSentUTC,
        EntryDescription,
        EntryDescriptionLanguage,
        EntryLastModifiedDateTimeOriginalTimezone,
        EntryLastModifiedDateTimeUTC,
        EntryLocation,
        EntryLocationLanguage,
        EntryMethod,
        EntryPriority,
        EntryProduct,
        EntryRecurrenceID,
        EntrySequence,
        EntryStatus,
        EntrySummary,
        EntrySummaryLanguage,
        EntryTransparency,
        EntryUID,
        EntryVersion,
        XAllDayEvent,
        XAppointmentSequence,
        XBusyStatus,
        XDisallowCounterpropose,
        XDoNotForwardMeeting,
        XGoogleConference,
        XImportance,
        XInstType,
        XIntendedStatus,
        XIsResponseRequested,
        XLatitude,
        XLocationDisplayName,
        XLocations,
        XLocationURI,
        XLocationSource,
        XLongitude,
        XOnlineMeetingConferenceID,
        XOnlineMeetingConferenceLink,
        XOnlineMeetingExternalLink,
        XOnlineMeetingInformation,
        XOnlineMeetingTollNumber,
        XOwnerAppointmentID,
        XSchedulingServiceUpdateURL,
        XSkypeTeamsMeetingURL,
        XSkypeTeamsProperties
    };
    enum PersonDetails {
        PersonEmailAddress,
        PersonName,
        PersonParticipationStatus,
        PersonRole,
        PersonRSVP,
        PersonType,
        PersonXNumberOfGuests
    };
    enum DateTimeDetails {
        DateTimeOriginalDateTime,
        DateTimeOriginalTimezoneName,
        DateTimeUTCDateTime
    };
    enum TimezoneDetails {
        TimezoneDaylightSavingTime_Name,
        TimezoneDaylightSavingTime_OffsetFrom_Min,
        TimezoneDaylightSavingTime_OffsetTo_Min,
        TimezoneDaylightSavingTime_RepeatRule,
        TimezoneDaylightSavingTime_StartDateTime,
        TimezoneName,
        TimezoneLocation,
        TimezoneStandardTime_Name,
        TimezoneStandardTime_OffsetFrom_Min,
        TimezoneStandardTime_OffsetTo_Min,
        TimezoneStandardTime_RepeatRule,
        TimezoneStandardTime_StartDateTime
    };
    // Alarm details
    enum AlarmDetails {
        AlarmAction,
        AlarmDescription,
        AlarmTriggerOffset,
        AlarmTriggerRelated
    };



    // ================================================================== Parse
private:
    // == Parse calendar entry
    int Parse_VCalendar(int mStartLine);

    // Parse person details
    QHash < PersonDetails, QString > Parse_PersonDetails(
        const QString mcDetails);

    // Parse date/time details
    QHash < DateTimeDetails, QString > Parse_DateTimeDetails(
        const QString mcDetails);

    // Parse text details
    QPair < QString, QString > Parse_TextDetails(const QString mcDetails);

    // == Parse timezone
    int Parse_VTimeZone(int mStartLine);
    int Parse_VTimeZoneDetails(int mStartLine);

    // == Parse event
    int Parse_VEvent(int mStartLine);

    // == Parse alarm
    int Parse_VAlarm(int mStartLine);

    // Parse alarm details
    void Parse_AlarmDetails(const QString mcDetails);

    // == Convert dates/times to UTC
    void CovertDateTimesToUTC();

    // Convert date/time in a timezone to UTC
    QDateTime ConvertToUTC(const QDateTime mcOriginalDateTime,
        const QString mcTimezone);

    // Get time change
    QDateTime GetTimeChange(const QString mcParameters,
        const QDateTime mcOriginalDateTime) const;



    // ================================================================= Access
public:
    // Calendar entry
    QHash < CalendarEntryDetails, QString > GetEntryDetails() const;
private:
    QHash < CalendarEntryDetails, QString > m_EntryDetails;

public:
    // Details about a person (organizer, participant)
    QHash < PersonDetails, QString > GetOrganizerDetails() const;
    QList < QHash < PersonDetails, QString > > GetParticipantsDetails() const;
private:
    QList < QHash < PersonDetails, QString > > m_ParticipantsDetails;

public:
    // Timezone
    QHash < TimezoneDetails, QString > GetTimezoneDetails() const;
private:
    QHash < TimezoneDetails, QString > m_TimezoneDetails;

public:
    // Start and finish
    QHash < DateTimeDetails, QString > GetStartDetails() const;
    QHash < DateTimeDetails, QString > GetEndDetails() const;
private:
    QHash < DateTimeDetails, QString > m_StartDetails;
    QHash < DateTimeDetails, QString > m_EndDetails;

public:
    // Alarm details
    QHash < AlarmDetails, QString > GetAlarmDetails() const;
private:
    QHash < AlarmDetails, QString > m_AlarmDetails;



    // ================================================================== Debug
public:
    // Show everything
    void Dump() const;
};

#endif
