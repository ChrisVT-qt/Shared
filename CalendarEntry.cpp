// TaskList - a software organizing everyday tasks
// Copyright (C) 2023 Chris von Toerne
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Contact the author by email: christian.vontoerne@gmail.com

// CalendarEntry.cpp
// Class implementation

// Project includes
#include "CalendarEntry.h"
#include "CallTracer.h"
#include "MessageLogger.h"

// Qt includes
#include <QFile>
#include <QRegularExpression>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
CalendarEntry::CalendarEntry()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Create instace from file
CalendarEntry * CalendarEntry::NewCalendarEntryFromFile(
    const QString mcFilename)
{
    CALL_IN(QString("mcFilename=%1")
        .arg(CALL_SHOW(mcFilename)));

    QFile input_file(mcFilename);
    if (!input_file.open(QIODevice::ReadOnly))
    {
        const QString reason = tr("File \"%1\" could not be opened.")
            .arg(mcFilename);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return nullptr;
    }

    // Read the whole thing
    const QString content = input_file.readAll();
    input_file.close();

    // Create object
    CalendarEntry * entry = NewCalendarEntry(content);

    CALL_OUT("");
    return entry;
}



///////////////////////////////////////////////////////////////////////////////
// Create instace from attachment data
CalendarEntry * CalendarEntry::NewCalendarEntry(const QString mcCalendarData)
{
    CALL_IN(QString("mcCalendarData=%1")
        .arg(CALL_SHOW(mcCalendarData)));

    // Create object
    CalendarEntry * entry = new CalendarEntry();
    QStringList split = mcCalendarData.trimmed().split("\n");

    // Take care of attributes spread over multiple lines
    int row = 0;
    while (row < split.size())
    {
        QString line = split[row];
        line.replace("\r", "");
        if (line.startsWith(" "))
        {
            if (entry -> m_Data.isEmpty())
            {
                // This shouldn't happen
                const QString reason(
                    tr("Calendar entry started with line continuation: \"%1\"")
                        .arg(line));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return nullptr;
            }
            const int last_row = entry -> m_Data.size() - 1;
            entry -> m_Data[last_row] += line.mid(1);
        } else
        {
            entry -> m_Data << line;
        }
        row++;
    }
    entry -> Parse_VCalendar(0);

    // Convert all dates/times to UTC
    entry -> CovertDateTimesToUTC();

    CALL_OUT("");
    return entry;
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
CalendarEntry::~CalendarEntry()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Parse calendar entry
int CalendarEntry::Parse_VCalendar(int mStartLine)
{
    CALL_IN(QString("mStartLine=%1")
        .arg(CALL_SHOW(mStartLine)));

    // BEGIN:VCALENDAR
    // METHOD:REQUEST
    // PRODID:Microsoft Exchange Server 2010
    // VERSION:2.0
    // CALSCALE:GREGORIAN
    // BEGIN:VTIMEZONE
    // ...
    // END:VTIMEZONE
    // BEGIN:VEVENT
    // ...
    // END:VEVENT
    // END:VCALENDAR

    if (m_Data[mStartLine] != "BEGIN:VCALENDAR")
    {
        const QString reason(tr("Unexpected content line: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    static const QRegularExpression format("^([A-Z\\-]+):(.*)$");
    while (mStartLine < m_Data.size())
    {
        mStartLine++;

        // Ignore empty lines
        if (m_Data[mStartLine].isEmpty())
        {
            continue;
        }

        // Split command and parameters
        const QRegularExpressionMatch match = format.match(m_Data[mStartLine]);
        if (!match.hasMatch())
        {
            const QString reason(tr("Content line could not be split: \"%1\"")
                .arg(m_Data[mStartLine]));
            MessageLogger::Message(CALL_METHOD, reason);
            continue;
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // END:VCALENDAR
        if (command == "END" &&
            parameters == "VCALENDAR")
        {
            // Done here.
            break;
        }

        // METHOD:REQUEST
        if (command == "METHOD")
        {
            m_EntryDetails[EntryMethod] = parameters;
            continue;
        }

        // PRODID:Microsoft Exchange Server 2010
        if (command == "PRODID")
        {
            m_EntryDetails[EntryProduct] = parameters;
            continue;
        }

        // VERSION:2.0
        if (command == "VERSION")
        {
            m_EntryDetails[EntryVersion] = parameters;
            continue;
        }

        // CALSCALE:GREGORIAN (Google)
        if (command == "CALSCALE")
        {
            m_EntryDetails[EntryCalendarScale] = parameters;
            continue;
        }

        // BEGIN:VTIMEZONE
        if (command == "BEGIN" &&
            parameters == "VTIMEZONE")
        {
            mStartLine = Parse_VTimeZone(mStartLine);
            continue;
        }

        // BEGIN:VEVENT
        if (command == "BEGIN" &&
            parameters == "VEVENT")
        {
            mStartLine = Parse_VEvent(mStartLine);
            continue;
        }

        // Unknown command
        const QString reason(tr("Content line has unknown command: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return mStartLine;
}



///////////////////////////////////////////////////////////////////////////////
// Parse person details
QHash < CalendarEntry::PersonDetails, QString >
    CalendarEntry::Parse_PersonDetails(const QString mcDetails)
{
    CALL_IN(QString("mcDetails=%1")
        .arg(CALL_SHOW(mcDetails)));

    // CN=John Doe:mailto:john.doe@foo.com (Outlook)

    // ROLE=REQ-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE;CN=Jane
    //  Doe:mailto:jane.doe@bar.com (Outlook)

    // CUTYPE=INDIVIDUAL;ROLE=REQ-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE;
    //  CN=john.doe@foo.com;X-NUM-GUESTS=0:mailto:john.doe@foo.com (Google)

    // Details
    QHash < PersonDetails, QString > details;

    // Email address
    static const QRegularExpression format_mailto("^(.+):mailto:(.*)$");
    QRegularExpressionMatch match = format_mailto.match(mcDetails);
    if (!match.hasMatch())
    {
        const QString reason(
            tr("Expected :mailto: in person details, got this: \"%1\"")
                .arg(mcDetails));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < CalendarEntry::PersonDetails, QString >();
    }
    details[PersonEmailAddress] = match.captured(2);
    const QString rest = match.captured(1);

    // The rest
    const QStringList split = rest.split(";");
    static const QRegularExpression format("^([A-Z\\-]+)=(.*)$");
    for (const QString & part : split)
    {
        match = format.match(part);
        if (!match.hasMatch())
        {
            const QString reason(tr("Invalid part encountered in "
                "person details: \"%1\" (\"%2\")")
                    .arg(part,
                         mcDetails));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QHash < PersonDetails, QString >();
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // CN=Jane Doe
        if (command == "CN")
        {
            details[PersonName] = parameters;
            continue;
        }

        // CUTYPE=INDIVIDUAL
        if (command == "CUTYPE")
        {
            details[PersonType] = parameters;
            continue;
        }

        // PARTSTAT=NEEDS-ACTION
        if (command == "PARTSTAT")
        {
            details[PersonParticipationStatus] = parameters;
            continue;
        }

        // ROLE=REQ-PARTICIPANT
        if (command == "ROLE")
        {
            details[PersonRole] = parameters;
            continue;
        }

        // RSVP=TRUE
        if (command == "RSVP")
        {
            details[PersonRSVP] = parameters;
            continue;
        }

        // X-NUM-GUESTS=0
        if (command == "X-NUM-GUESTS")
        {
            details[PersonXNumberOfGuests] = parameters;
            continue;
        }

        // Unknown command
        const QString reason(
            tr("Unknown parameter name \"%1\" encountered in person details: "
               "\"%2\"")
                .arg(command,
                     mcDetails));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return details;
}



///////////////////////////////////////////////////////////////////////////////
// Parse date/time details
QHash < CalendarEntry::DateTimeDetails, QString >
    CalendarEntry::Parse_DateTimeDetails(const QString mcDetails)
{
    CALL_IN(QString("mcDetails=%1")
        .arg(CALL_SHOW(mcDetails)));

    // TZID=Pacific Standard Time:20290303T060000
    // 20230306T090000Z

    // Return value
    QHash < CalendarEntry::DateTimeDetails, QString > details;

    // Timezone
    static const QRegularExpression format_timezone(
        "^TZID=([^:]+):([0-9]+T[0-9]+)Z?$");
    QRegularExpressionMatch match = format_timezone.match(mcDetails);
    if (match.hasMatch())
    {
        details[DateTimeOriginalTimezoneName] = match.captured(1);
        const QDateTime date_time =
            QDateTime::fromString(match.captured(2), "yyyyMMddThhmmss");
        if (!date_time.isValid())
        {
            const QString reason(tr("Malformed date/time: \"%1\"")
                .arg(match.captured(2)));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QHash < CalendarEntry::DateTimeDetails, QString >();
        }
        details[DateTimeOriginalDateTime] =
            date_time.toString("yyyy-MM-dd hh:mm:ss");
        CALL_OUT("");
        return details;
    }

    // No timezone
    static const QRegularExpression format_notimezone(
        "^([0-9]+T[0-9]+)Z?$");
    match = format_notimezone.match(mcDetails);
    if (match.hasMatch())
    {
        QDateTime date_time =
            QDateTime::fromString(match.captured(1), "yyyyMMddThhmmss");
        if (date_time.isValid())
        {
            details[DateTimeOriginalDateTime] =
                date_time.toString("yyyy-MM-dd hh:mm:ss");
            CALL_OUT("");
            return details;
        }
        date_time = QDateTime::fromString(match.captured(1), "yyyyMMddThhmm");
        if (date_time.isValid())
        {
            details[DateTimeOriginalDateTime] =
                date_time.toString("yyyy-MM-dd hh:mm:ss");
            CALL_OUT("");
            return details;
        }

        // Error
        const QString reason(tr("Malformed date/time: \"%1\"")
            .arg(match.captured(2)));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < CalendarEntry::DateTimeDetails, QString >();
    }

    // Error
    const QString reason(tr("Malformed time details: \"%1\"")
        .arg(mcDetails));
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return QHash < CalendarEntry::DateTimeDetails, QString >();
}



///////////////////////////////////////////////////////////////////////////////
// Parse text details
QPair < QString, QString > CalendarEntry::Parse_TextDetails(
    const QString mcDetails)
{
    CALL_IN(QString("mcDetails=%1")
        .arg(CALL_SHOW(mcDetails)));

    // LANGUAGE=en-US:lorem ipsum... (Outlook)
    // lorem ipsim... (Google)

    // Return value
    QPair < QString, QString > details;

    // Language (if available)
    static const QRegularExpression format_language(
        "^LANGUAGE=([a-zA-Z\\-]+):(.*)$");
    const QRegularExpressionMatch match = format_language.match(mcDetails);
    if (match.hasMatch())
    {
        details.first = match.captured(1);
        details.second = match.captured(2);
    } else
    {
        // No language provided
        details.first = "";
        details.second = mcDetails;
    }
    details.second.replace("\\n", "\n");
    details.second.replace("\\,", ",");

    CALL_OUT("");
    return details;
}



///////////////////////////////////////////////////////////////////////////////
// Parse calendar entry
int CalendarEntry::Parse_VTimeZone(int mStartLine)
{
    CALL_IN(QString("mStartLine=%1")
        .arg(CALL_SHOW(mStartLine)));

    // BEGIN:VTIMEZONE
    // TZID:Pacific Standard Time
    // X-LIC-LOCATION:America/Los_Angeles (Google)
    // BEGIN:STANDARD
    // ...
    // END:STANDARD
    // BEGIN:DAYLIGHT
    // ...
    // END:DAYLIGHT
    // END:VTIMEZONE

    if (m_Data[mStartLine] != "BEGIN:VTIMEZONE")
    {
        const QString reason(tr("Unexpected content line: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    static const QRegularExpression format("^([A-Z\\-]+):(.*)$");
    while (mStartLine < m_Data.size())
    {
        mStartLine++;
        const QRegularExpressionMatch match = format.match(m_Data[mStartLine]);
        if (!match.hasMatch())
        {
            const QString reason(tr("Content line could not be split: \"%1\"")
                .arg(m_Data[mStartLine]));
            MessageLogger::Message(CALL_METHOD, reason);
            continue;
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // TZID:Pacific Stadandard Time
        if (command == "TZID")
        {
            m_TimezoneDetails[TimezoneName] = parameters;
            continue;
        }

        // X-LIC-LOCATION:America/Los_Angeles
        if (command == "X-LIC-LOCATION")
        {
            m_TimezoneDetails[TimezoneLocation] = parameters;
            continue;
        }

        // BEGIN:STANDARD
        if (command == "BEGIN" &&
            (parameters == "STANDARD" || parameters == "DAYLIGHT"))
        {
            mStartLine = Parse_VTimeZoneDetails(mStartLine);
            continue;
        }

        // END:VTIMEZONE
        if (command == "END" &&
            parameters == "VTIMEZONE")
        {
            // Done here.
            break;
        }

        // Unknown command
        const QString reason(tr("Content line has unknown command: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return mStartLine;
}



///////////////////////////////////////////////////////////////////////////////
// Parse calendar entry
int CalendarEntry::Parse_VTimeZoneDetails(int mStartLine)
{
    CALL_IN(QString("mStartLine=%1")
        .arg(CALL_SHOW(mStartLine)));

    // BEGIN:STANDARD
    // DTSTART:16010101T020000
    // TZOFFSETFROM:-0700
    // TZOFFSETTO:-0800
    // TZNAME:PST (Google)
    // RRULE:FREQ=YEARLY;INTERVAL=1;BYDAY=1SU;BYMONTH=11
    // END:STANDARD

    // BEGIN:DAYLIGHT
    // DTSTART:16010101T020000
    // TZOFFSETFROM:-0800
    // TZOFFSETTO:-0700
    // TZNAME:PDT (Google)
    // RRULE:FREQ=YEARLY;INTERVAL=1;BYDAY=2SU;BYMONTH=3
    // END:DAYLIGHT

    if (m_Data[mStartLine] != "BEGIN:STANDARD" &&
        m_Data[mStartLine] != "BEGIN:DAYLIGHT")
    {
        const QString reason(tr("Unexpected content line: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    // Map some attributes
    TimezoneDetails attribute_name;
    TimezoneDetails attribute_startdatetime;
    TimezoneDetails attribute_offsetfrom_min;
    TimezoneDetails attribute_offsetto_min;
    TimezoneDetails attribute_repeatrule;
    if (m_Data[mStartLine] == "BEGIN:STANDARD")
    {
        attribute_name = TimezoneStandardTime_Name;
        attribute_startdatetime = TimezoneStandardTime_StartDateTime;
        attribute_offsetfrom_min = TimezoneStandardTime_OffsetFrom_Min;
        attribute_offsetto_min = TimezoneStandardTime_OffsetTo_Min;
        attribute_repeatrule = TimezoneStandardTime_RepeatRule;
    } else if (m_Data[mStartLine] == "BEGIN:DAYLIGHT")
    {
        attribute_name = TimezoneDaylightSavingTime_Name;
        attribute_startdatetime = TimezoneDaylightSavingTime_StartDateTime;
        attribute_offsetfrom_min = TimezoneDaylightSavingTime_OffsetFrom_Min;
        attribute_offsetto_min = TimezoneDaylightSavingTime_OffsetTo_Min;
        attribute_repeatrule = TimezoneDaylightSavingTime_RepeatRule;
    }

    static const QRegularExpression format("^([A-Z]+):(.*)$");
    static const QRegularExpression format_offset(
        "^([\\+\\-]?[0-9]+)([0-5][0-9])$");
    while (mStartLine < m_Data.size())
    {
        mStartLine++;
        const QRegularExpressionMatch match = format.match(m_Data[mStartLine]);
        if (!match.hasMatch())
        {
            const QString reason(tr("Content line could not be split: \"%1\"")
                .arg(m_Data[mStartLine]));
            MessageLogger::Message(CALL_METHOD, reason);
            continue;
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // TZNAME:PST
        if (command == "TZNAME")
        {
            m_TimezoneDetails[attribute_name] = parameters;
            continue;
        }

        // DTSTART:16010101T020000
        if (command == "DTSTART")
        {
            const QDateTime start =
                QDateTime::fromString(parameters, "yyyyMMddThhmmss");
            if (!start.isValid())
            {
                const QString reason(
                    tr("Invalid timezone start date/time format: \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            m_TimezoneDetails[attribute_startdatetime] =
                start.toString("yyyy-MM-dd hh:mm:ss");
            continue;
        }

        // TZOFFSETFROM:-0700
        if (command == "TZOFFSETFROM")
        {
            const QRegularExpressionMatch match_offset =
                format_offset.match(parameters);
            if (!match_offset.hasMatch())
            {
                const QString reason(
                    tr("Invalid timezone offset format (from): \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            const int hours = match_offset.captured(1).toInt();
            const int minutes = match_offset.captured(2).toInt();
            m_TimezoneDetails[attribute_offsetfrom_min] =
                QString::number(hours*60 + (hours < 0 ? -minutes : minutes));
            continue;
        }

        // TZOFFSETTO:-0800
        if (command == "TZOFFSETTO")
        {
            const QRegularExpressionMatch match_offset =
                format_offset.match(parameters);
            if (!match_offset.hasMatch())
            {
                const QString reason(
                    tr("Invalid timezone offset format (to): \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            const int hours = match_offset.captured(1).toInt();
            const int minutes = match_offset.captured(2).toInt();
            m_TimezoneDetails[attribute_offsetto_min] =
                QString::number(hours*60 + (hours < 0 ? -minutes : minutes));
            continue;
        }

        // RRULE:FREQ=YEARLY;INTERVAL=1;BYDAY=1SU;BYMONTH=11
        if (command == "RRULE")
        {
            m_TimezoneDetails[attribute_repeatrule] = parameters;
            continue;
        }

        // END:STANDARD
        // END:DAYLIGHT
        if (command == "END" &&
            (parameters == "STANDARD" || parameters == "DAYLIGHT"))
        {
            // Done here.
            break;
        }

        // Unknown command
        const QString reason(tr("Content line has unknown command: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return mStartLine;
}



///////////////////////////////////////////////////////////////////////////////
// Parse calendar entry
int CalendarEntry::Parse_VEvent(int mStartLine)
{
    CALL_IN(QString("mStartLine=%1")
        .arg(CALL_SHOW(mStartLine)));

    // BEGIN:VEVENT
    // ORGANIZER;CN=John Doe:mailto:john.doe@foo.com
    // ATTENDEE;ROLE=REQ-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE;CN=Jane
    //  Doe:mailto:jane.doe@bar.com
    // DESCRIPTION;LANGUAGE=en-US:lorem ipsum...
    // UID:040000008200E00074C5B7102A82E00800500000AA8ED371434DD901000000000000000
    //  0100000000138782FDE983B448BEBD2415630E30F
    // SUMMARY;LANGUAGE=en-US:eAArly DETECT data
    // DTSTART;TZID=Pacific Standard Time:20290303T060000
    // DTEND;TZID=Pacific Standard Time:20290303T070000
    // CLASS:PUBLIC
    // PRIORITY:5
    // DTSTAMP:20290302T201139Z
    // TRANSP:OPAQUE
    // STATUS:CONFIRMED
    // RECURRENCE-ID;TZID=America/Los_Angeles:20230325T120000 (Google)
    // SEQUENCE:0
    // LOCATION;LANGUAGE=en-US:https://some.url/k/85e4612517106?from=addon
    // CREATED:20210227T193507Z (Google)
    // LAST-MODIFIED:20230304T205056Z (Google)
    // X-GOOGLE-CONFERENCE:https://meet.google.com/kyj-sict-zbe (Google)
    // X-MICROSOFT-CDO-APPT-SEQUENCE:0 (Outlook)
    // X-MICROSOFT-CDO-OWNERAPPTID:2121470890 (Outlook)
    // X-MICROSOFT-CDO-BUSYSTATUS:TENTATIVE (Outlook)
    // X-MICROSOFT-CDO-INTENDEDSTATUS:BUSY (Outlook)
    // X-MICROSOFT-CDO-ALLDAYEVENT:FALSE (Outlook)
    // X-MICROSOFT-CDO-IMPORTANCE:1 (Outlook)
    // X-MICROSOFT-CDO-INSTTYPE:0 (Outlook)
    // X-MICROSOFT-LATITUDE:0
    // X-MICROSOFT-LOCATIONURI:Meetingroom@foo.com
    // X-MICROSOFT-LONGITUDE:0
    // X-MICROSOFT-ONLINEMEETINGEXTERNALLINK: (Outlook)
    // X-MICROSOFT-ONLINEMEETINGCONFLINK: (Outlook)
    // X-MICROSOFT-DONOTFORWARDMEETING:FALSE (Outlook)
    // X-MICROSOFT-DISALLOW-COUNTER:FALSE (Outlook)
    // X-MICROSOFT-ISRESPONSEREQUESTED:TRUE (Outlook)
    // X-MICROSOFT-LOCATIONDISPLAYNAME:https://some.url/k/85e4612517106?from=
    //  addon (Outlook)
    // X-MICROSOFT-LOCATIONSOURCE:None (Outlook)
    // X-MICROSOFT-LOCATIONS:[{"DisplayName":"https://some.url/k/85e461251710
    //  6?from=addon"\,"LocationAnnotation":""\,"LocationUri":""\,"LocationStr
    //  eet":""\,"LocationCity":""\,"LocationState":""\,"LocationCountry":""\,
    //  "LocationPostalCode":""\,"LocationFullAddress":""}] (Outlook)
    // X-MICROSOFT-ONLINEMEETINGINFORMATION:{"OnlineMeetingChannelId":null\,"
    //  OnlineMeetingProvider":3}
    // X-MICROSOFT-ONLINEMEETINGTOLLNUMBER:+49 69 710414474
    // X-MICROSOFT-SCHEDULINGSERVICEUPDATEURL:https://api.scheduler.teams.micr
    //  osoft.com/teams/67babc5f-d3b3-47d2-8bcd-40d59d4c2cb1/0ac82ff2-ffb1-470
    //  4-8fbb-6b30fe49c5ea/19_meeting_NzI5OTRlMmEtODJlZC00ZjA2LTkyMzEtZWVkNGE
    //  5YWMzN2M1@thread.v2/0
    // X-MICROSOFT-SKYPETEAMSMEETINGURL:https://teams.microsoft.com/l/meetup-j
    //  oin/19%3ameeting_Y2I0ODFkNGItNGIzMC00N2FkLWI2ZTQtNDc1NTUyMzM4ZmEx%40th
    //  read.v2/0?context=%7b%22Tid%22%3a%2267babc5f-d3b3-47d2-8bcd-40d59d4c2c
    //  b1%22%2c%22Oid%22%3a%2202af95eb-0bfc-4767-b99a-dc89b62fc224%22%7d
    // X-MICROSOFT-SKYPETEAMSPROPERTIES:{"cid":"19:meeting_Y2I0ODFkNGItNGIzMC0
    //  0N2FkLWI2ZTQtNDc1NTUyMzM4ZmEx@thread.v2"\,"private":true\,"type":0\,"m
    //  id":0\,"rid":0\,"uid":null}
    // BEGIN:VALARM
    // ...
    // END:VALARM
    // END:VEVENT

    if (m_Data[mStartLine] != "BEGIN:VEVENT")
    {
        const QString reason(tr("Unexpected content line: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    static const QRegularExpression format_colon("^([A-Z\\-]+):(.*)$");
    static const QRegularExpression format_semicolon("^([A-Z\\-]+);(.*)$");
    while (mStartLine < m_Data.size())
    {
        mStartLine++;

        // Ignore empty lines
        if (m_Data[mStartLine].isEmpty())
        {
            continue;
        }

        // Separate command and its parameters
        QRegularExpressionMatch match = format_colon.match(m_Data[mStartLine]);
        if (!match.hasMatch())
        {
            // Not sure why the separator changes from ":" everywhere else to
            // ";" for some items in this section; at least it does for
            // Outlook. Awfully looks like a bug.
            match = format_semicolon.match(m_Data[mStartLine]);
        }
        if (!match.hasMatch())
        {
            const QString reason(tr("Content line could not be split: \"%1\"")
                .arg(m_Data[mStartLine]));
            MessageLogger::Message(CALL_METHOD, reason);
            continue;
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // ATTENDEE;ROLE=REQ-PARTICIPANT;PARTSTAT=NEEDS-ACTION;RSVP=TRUE;CN=Jane
        //  Doe:mailto:jane.doe@bar.com
        if (command == "ATTENDEE")
        {
            m_ParticipantsDetails << Parse_PersonDetails(parameters);
            continue;
        }

        // BEGIN:VALARM
        if (command == "BEGIN" &&
            parameters == "VALARM")
        {
            mStartLine = Parse_VAlarm(mStartLine);
            continue;
        }

        if (command == "CATEGORIES")
        {
            m_EntryDetails[EntryCategories] = parameters;
            continue;
        }

        // CLASS:PUBLIC
        if (command == "CLASS")
        {
            m_EntryDetails[EntryClass] = parameters;
            continue;
        }

        // CREATED:20210227T193507Z (Google)
        if (command == "CREATED")
        {
            const QDateTime created =
                QDateTime::fromString(parameters, "yyyyMMddThhmmssZ");
            if (!created.isValid())
            {
                const QString reason(
                    tr("Invalid date/time created format: \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            m_EntryDetails[EntryCreatedDateTimeOriginalTimezone] =
                created.toString("yyyy-MM-dd hh:mm:ss");
            continue;
        }

        // DESCRIPTION;LANGUAGE=en-US:lorem ipsum...
        if (command == "DESCRIPTION")
        {
            const QPair < QString, QString > details =
                Parse_TextDetails(parameters);
            m_EntryDetails[EntryDescriptionLanguage] = details.first;
            m_EntryDetails[EntryDescription] = details.second;
            continue;
        }

        // DTEND;TZID=Pacific Standard Time:20290303T070000
        if (command == "DTEND")
        {
            m_EndDetails = Parse_DateTimeDetails(parameters);
            continue;
        }

        // DTSTAMP:20290302T201139Z
        if (command == "DTSTAMP")
        {
            QDateTime sent =
                QDateTime::fromString(parameters, "yyyyMMddThhmmssZ");
            if (!sent.isValid())
            {
                sent = QDateTime::fromString(parameters, "yyyyMMddThhmmZ");
            }
            if (!sent.isValid())
            {
                const QString reason(
                    tr("Invalid date/time sent format: \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            m_EntryDetails[EntryDateTimeSentOriginalTimezone] =
                sent.toString("yyyy-MM-dd hh:mm:ss");
            continue;
        }

        // DTSTART;TZID=Pacific Standard Time:20290303T060000
        if (command == "DTSTART")
        {
            m_StartDetails = Parse_DateTimeDetails(parameters);
            continue;
        }

        // END:VEVENT
        if (command == "END" &&
            parameters == "VEVENT")
        {
            // Done here.
            break;
        }

        // LAST-MODIFIED:20230304T205056Z (Google)
        if (command == "LAST-MODIFIED")
        {
            const QDateTime last_modified =
                QDateTime::fromString(parameters, "yyyyMMddThhmmssZ");
            if (!last_modified.isValid())
            {
                const QString reason(
                    tr("Invalid date/time last modified format: \"%1\"")
                        .arg(m_Data[mStartLine]));
                MessageLogger::Error(CALL_METHOD, reason);
                CALL_OUT(reason);
                return -1;
            }
            m_EntryDetails[EntryLastModifiedDateTimeOriginalTimezone] =
                last_modified.toString("yyyy-MM-dd hh:mm:ss");
            continue;
        }

        // LOCATION;LANGUAGE=en-US:https://some.url/k/85e4612517106?from=addon
        if (command == "LOCATION")
        {
            const QPair < QString, QString > details =
                Parse_TextDetails(parameters);
            m_EntryDetails[EntryLocationLanguage] = details.first;
            m_EntryDetails[EntryLocation] = details.second;
            continue;
        }

        // ORGANIZER;CN=John Doe:mailto:john.doe@foo.com
        if (command == "ORGANIZER")
        {
            QHash < PersonDetails, QString > person =
                Parse_PersonDetails(parameters);
            person[PersonRole] = "organizer";
            m_ParticipantsDetails << person;
            continue;
        }

        // PRIORITY:5
        if (command == "PRIORITY")
        {
            m_EntryDetails[EntryPriority] = parameters;
            continue;
        }

        // RECURRENCE-ID;TZID=America/Los_Angeles:20230325T120000 (Google)
        // (This is something like the original date for a recurring event
        // if this instance has been moved to an out-of-sequence date)
        if (command == "RECURRENCE-ID")
        {
            m_EntryDetails[EntryRecurrenceID] = parameters;
            continue;
        }

        // SEQUENCE:0
        if (command == "SEQUENCE")
        {
            m_EntryDetails[EntrySequence] = parameters;
            continue;
        }

        // STATUS:CONFIRMED
        if (command == "STATUS")
        {
            m_EntryDetails[EntryStatus] = parameters;
            continue;
        }

        // SUMMARY;LANGUAGE=en-US:eAArly DETECT data
        if (command == "SUMMARY")
        {
            const QPair < QString, QString > details =
                Parse_TextDetails(parameters);
            m_EntryDetails[EntrySummaryLanguage] = details.first;
            m_EntryDetails[EntrySummary] = details.second;
            continue;
        }

        // TRANSP:OPAQUE
        if (command == "TRANSP")
        {
            m_EntryDetails[EntryTransparency] = parameters;
            continue;
        }

        // UID:040000008200E00074C5B7102A82E00800500000AA8ED371434DD901000000000000000
        //  0100000000138782FDE983B448BEBD2415630E30F
        if (command == "UID")
        {
            m_EntryDetails[EntryUID] = parameters;
            continue;
        }

        // X-ALT-DESC;FMTTYPE=text/html:<html>...</html>
        if (command == "X-ALT-DESC")
        {
            m_EntryDetails[EntryDescription] = parameters;
            continue;
        }

        // X-GOOGLE-CONFERENCE:https://meet.google.com/kyj-sict-zbe (Google)
        if (command == "X-GOOGLE-CONFERENCE")
        {
            m_EntryDetails[XGoogleConference] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-ALLDAYEVENT:FALSE
        if (command == "X-MICROSOFT-CDO-ALLDAYEVENT")
        {
            m_EntryDetails[XAllDayEvent] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-APPT-SEQUENCE:0
        if (command == "X-MICROSOFT-CDO-APPT-SEQUENCE")
        {
            m_EntryDetails[XAppointmentSequence] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-BUSYSTATUS:TENTATIVE
        if (command == "X-MICROSOFT-CDO-BUSYSTATUS")
        {
            m_EntryDetails[XBusyStatus] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-IMPORTANCE:1
        if (command == "X-MICROSOFT-CDO-IMPORTANCE")
        {
            m_EntryDetails[XImportance] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-INSTTYPE:0
        if (command == "X-MICROSOFT-CDO-INSTTYPE")
        {
            m_EntryDetails[XInstType] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-INTENDEDSTATUS:BUSY
        if (command == "X-MICROSOFT-CDO-INTENDEDSTATUS")
        {
            m_EntryDetails[XIntendedStatus] = parameters;
            continue;
        }

        // X-MICROSOFT-CDO-OWNERAPPTID:2121470890
        if (command == "X-MICROSOFT-CDO-OWNERAPPTID")
        {
            m_EntryDetails[XOwnerAppointmentID] = parameters;
            continue;
        }

        // X-MICROSOFT-DISALLOW-COUNTER:FALSE
        if (command == "X-MICROSOFT-DISALLOW-COUNTER")
        {
            m_EntryDetails[XDisallowCounterpropose] = parameters;
            continue;
        }

        // X-MICROSOFT-DONOTFORWARDMEETING:FALSE
        if (command == "X-MICROSOFT-DONOTFORWARDMEETING")
        {
            m_EntryDetails[XDoNotForwardMeeting] = parameters;
            continue;
        }

        // X-MICROSOFT-ISRESPONSEREQUESTED:TRUE
        if (command == "X-MICROSOFT-ISRESPONSEREQUESTED")
        {
            m_EntryDetails[XIsResponseRequested] = parameters;
            continue;
        }

        // X-MICROSOFT-LATITUDE:0
        if (command == "X-MICROSOFT-LATITUDE")
        {
            m_EntryDetails[XLatitude] = parameters;
            continue;
        }

        // X-MICROSOFT-LOCATIONDISPLAYNAME:https://some.url/k/85e4612517106?
        //  from=addon
        if (command == "X-MICROSOFT-LOCATIONDISPLAYNAME")
        {
            m_EntryDetails[XLocationDisplayName] = parameters;
            continue;
        }

        // X-MICROSOFT-LOCATIONS:[{"DisplayName":"https://some.url/k/85e461251
        //  7106?from=addon"\,"LocationAnnotation":""\,"LocationUri":""\,"Loca
        //  tionStreet":""\,"LocationCity":""\,"LocationState":""\,"LocationCo
        //  untry":""\,"LocationPostalCode":""\,"LocationFullAddress":""}]
        if (command == "X-MICROSOFT-LOCATIONS")
        {
            m_EntryDetails[XLocations] = parameters;
            continue;
        }

        // X-MICROSOFT-LOCATIONSOURCE:None
        if (command == "X-MICROSOFT-LOCATIONSOURCE")
        {
            m_EntryDetails[XLocationSource] = parameters;
            continue;
        }

        // X-MICROSOFT-LOCATIONURI:Meetingroom@foo.com
        if (command == "X-MICROSOFT-LOCATIONURI")
        {
            m_EntryDetails[XLocationURI] = parameters;
            continue;
        }

        // X-MICROSOFT-LONGITUDE:0
        if (command == "X-MICROSOFT-LONGITUDE")
        {
            m_EntryDetails[XLongitude] = parameters;
            continue;
        }

        // X-MICROSOFT-ONLINEMEETINGCONFERENCEID:896679561
        if (command == "X-MICROSOFT-ONLINEMEETINGCONFERENCEID")
        {
            QString value = parameters;
            value.replace("\\,", ",");
            m_EntryDetails[XOnlineMeetingConferenceID] = parameters;
            continue;
        }

        // X-MICROSOFT-ONLINEMEETINGCONFLINK:
        if (command == "X-MICROSOFT-ONLINEMEETINGCONFLINK")
        {
            m_EntryDetails[XOnlineMeetingConferenceLink] = parameters;
            continue;
        }

        // X-MICROSOFT-ONLINEMEETINGEXTERNALLINK:
        if (command == "X-MICROSOFT-ONLINEMEETINGEXTERNALLINK")
        {
            m_EntryDetails[XOnlineMeetingExternalLink] = parameters;
            continue;
        }

        // "X-MICROSOFT-ONLINEMEETINGINFORMATION:{"OnlineMeetingChannelId":nul
        //  l\,"OnlineMeetingProvider":3}
        if (command == "X-MICROSOFT-ONLINEMEETINGINFORMATION")
        {
            QString value = parameters;
            value.replace("\\,", ",");
            m_EntryDetails[XOnlineMeetingInformation] = parameters;
            continue;
        }

        // X-MICROSOFT-ONLINEMEETINGTOLLNUMBER:+49 69 710414474
        if (command == "X-MICROSOFT-ONLINEMEETINGTOLLNUMBER")
        {
            QString value = parameters;
            value.replace("\\,", ",");
            m_EntryDetails[XOnlineMeetingTollNumber] = parameters;
            continue;
        }

        // X-MICROSOFT-SCHEDULINGSERVICEUPDATEURL:https://api.scheduler.teams.
        //  microsoft.com/teams/67babc5f-d3b3-47d2-8bcd-40d59d4c2cb1/0ac82ff2-
        //  ffb1-4704-8fbb-6b30fe49c5ea/19_meeting_NzI5OTRlMmEtODJlZC00ZjA2LTk
        //  yMzEtZWVkNGE5YWMzN2M1@thread.v2/0
        if (command == "X-MICROSOFT-SCHEDULINGSERVICEUPDATEURL")
        {
            m_EntryDetails[XSchedulingServiceUpdateURL] = parameters;
            continue;
        }

        // X-MICROSOFT-SKYPETEAMSMEETINGURL:https://teams.microsoft.com/l/meet
        //  up-join/19%3ameeting_Y2I0ODFkNGItNGIzMC00N2FkLWI2ZTQtNDc1NTUyMzM4Z
        //  mEx%40thread.v2/0?context=%7b%22Tid%22%3a%2267babc5f-d3b3-47d2-8bc
        //  d-40d59d4c2cb1%22%2c%22Oid%22%3a%2202af95eb-0bfc-4767-b99a-dc89b62
        //  fc224%22%7d
        if (command == "X-MICROSOFT-SKYPETEAMSMEETINGURL")
        {
            m_EntryDetails[XSkypeTeamsMeetingURL] = parameters;
            continue;
        }

        // X-MICROSOFT-SKYPETEAMSPROPERTIES:{"cid":"19:meeting_Y2I0ODFkNGItNGI
        // zMC00N2FkLWI2ZTQtNDc1NTUyMzM4ZmEx@thread.v2"\,"private":true\,"type
        // ":0\,"mid":0\,"rid":0\,"uid":null}
        if (command == "X-MICROSOFT-SKYPETEAMSPROPERTIES")
        {
            QString value = parameters;
            value.replace("\\,", ",");
            m_EntryDetails[XSkypeTeamsProperties] = parameters;
            continue;
        }









        // Unknown command
        const QString reason(tr("Content line has unknown command: "
            "command: \"%1\", parameters: \"%2\"")
            .arg(command,
                 parameters));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return mStartLine;
}



///////////////////////////////////////////////////////////////////////////////
// Parse calendar entry
int CalendarEntry::Parse_VAlarm(int mStartLine)
{
    CALL_IN(QString("mStartLine=%1")
        .arg(CALL_SHOW(mStartLine)));

    // BEGIN:VALARM
    // DESCRIPTION:REMINDER
    // TRIGGER;RELATED=START:-PT15M
    // ACTION:DISPLAY
    // END:VALARM

    if (m_Data[mStartLine] != "BEGIN:VALARM")
    {
        const QString reason(tr("Unexpected content line: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return -1;
    }

    static const QRegularExpression format_colon("^([A-Z\\-]+):(.*)$");
    static const QRegularExpression format_semicolon("^([A-Z\\-]+);(.*)$");
    while (mStartLine < m_Data.size())
    {
        mStartLine++;
        QRegularExpressionMatch match = format_colon.match(m_Data[mStartLine]);
        if (!match.hasMatch())
        {
            // Not sure why the separator changes from ":" everywhere else to
            // ";" for TRIGGER. Awfully looks like a bug.
            match = format_semicolon.match(m_Data[mStartLine]);
        }
        if (!match.hasMatch())
        {
            const QString reason(tr("Content line could not be split: \"%1\"")
                .arg(m_Data[mStartLine]));
            MessageLogger::Message(CALL_METHOD, reason);
            continue;
        }
        const QString command = match.captured(1);
        const QString parameters = match.captured(2);

        // DESCRIPTION:REMINDER
        if (command == "DESCRIPTION")
        {
            m_AlarmDetails[AlarmDescription] = parameters;
            continue;
        }

        // TRIGGER;RELATED=START:-PT15M
        if (command == "TRIGGER")
        {
            Parse_AlarmDetails(parameters);
            continue;
        }

        // ACTION:DISPLAY
        if (command == "ACTION")
        {
            m_AlarmDetails[AlarmAction] = parameters;
            continue;
        }

        // END:VALARM
        if (command == "END" &&
            parameters == "VALARM")
        {
            // Done here.
            break;
        }

        // Unknown command
        const QString reason(tr("Content line has unknown command: \"%1\"")
            .arg(m_Data[mStartLine]));
        MessageLogger::Message(CALL_METHOD, reason);
    }

    CALL_OUT("");
    return mStartLine;
}



///////////////////////////////////////////////////////////////////////////////
// Parse alarm details
void CalendarEntry::Parse_AlarmDetails(const QString mcDetails)
{
    CALL_IN(QString("mcDetails=%1")
        .arg(CALL_SHOW(mcDetails)));

    // RELATED=START:-PT15M
    static const QRegularExpression format("^RELATED=([A-Z]+):-PT(.+)$");
    QRegularExpressionMatch match = format.match(mcDetails);
    if (!match.hasMatch())
    {
        const QString reason(tr("Invalid alarm details format: \"%1\"")
            .arg(mcDetails));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    m_AlarmDetails[AlarmTriggerRelated] = match.captured(1);
    m_AlarmDetails[AlarmTriggerOffset] = match.captured(2);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Convert dates/times to UTC
void CalendarEntry::CovertDateTimesToUTC()
{
    CALL_IN("");

    // Entry date/time created
    if (m_EntryDetails.contains(EntryCreatedDateTimeOriginalTimezone))
    {
        const QDateTime date_time = QDateTime::fromString(
            m_EntryDetails[EntryCreatedDateTimeOriginalTimezone],
            "yyyy-MM-dd hh:mm:ss");
        m_EntryDetails[EntryCreatedDateTimeUTC] =
            ConvertToUTC(date_time, m_TimezoneDetails[TimezoneName])
                .toString("yyyy-MM-dd hh:mm:ss");
    }

    // Entry date/time last modified
    if (m_EntryDetails.contains(EntryLastModifiedDateTimeOriginalTimezone))
    {
        const QDateTime date_time = QDateTime::fromString(
            m_EntryDetails[EntryLastModifiedDateTimeOriginalTimezone],
            "yyyy-MM-dd hh:mm:ss");
        m_EntryDetails[EntryLastModifiedDateTimeUTC] =
            ConvertToUTC(date_time, m_TimezoneDetails[TimezoneName])
                .toString("yyyy-MM-dd hh:mm:ss");
    }

    // Entry date/time sent
    if (m_EntryDetails.contains(EntryDateTimeSentOriginalTimezone))
    {
        const QDateTime date_time = QDateTime::fromString(
            m_EntryDetails[EntryDateTimeSentOriginalTimezone],
            "yyyy-MM-dd hh:mm:ss");
        m_EntryDetails[EntryDateTimeSentUTC] =
            ConvertToUTC(date_time, m_TimezoneDetails[TimezoneName])
                .toString("yyyy-MM-dd hh:mm:ss");
    }

    // Start date/time
    if (m_StartDetails.contains(DateTimeOriginalDateTime))
    {
        const QDateTime date_time = QDateTime::fromString(
            m_StartDetails[DateTimeOriginalDateTime], "yyyy-MM-dd hh:mm:ss");
        m_StartDetails[DateTimeUTCDateTime] =
            ConvertToUTC(date_time,
                m_StartDetails[DateTimeOriginalTimezoneName])
                    .toString("yyyy-MM-dd hh:mm:ss");
    }

    // End date/time
    if (m_EndDetails.contains(DateTimeOriginalDateTime))
    {
        const QDateTime date_time = QDateTime::fromString(
            m_EndDetails[DateTimeOriginalDateTime], "yyyy-MM-dd hh:mm:ss");
        m_EndDetails[DateTimeUTCDateTime] =
            ConvertToUTC(date_time, m_EndDetails[DateTimeOriginalTimezoneName])
                .toString("yyyy-MM-dd hh:mm:ss");
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Convert date/time in a timezone to UTC
QDateTime CalendarEntry::ConvertToUTC(const QDateTime mcOriginalDateTime,
    const QString mcTimezone)
{
    CALL_IN(QString("mcOriginalDateTime=%1, mcTimezone=%2")
        .arg(CALL_SHOW(mcOriginalDateTime),
             CALL_SHOW(mcTimezone)));

    // Check if we have a repeat rule
    if (mcTimezone.isEmpty() ||
        !m_TimezoneDetails.contains(TimezoneDaylightSavingTime_RepeatRule))
    {
        // No time zone or not repeat rule; this behaves like UTC.
        CALL_OUT("");
        return mcOriginalDateTime;
    }

    // Check if we have information about the timezone (should be!)
    if (mcTimezone != m_TimezoneDetails[TimezoneName])
    {
        // We don't. Oops.
        const QString reason(
            tr("Unknown timezone \"%1\"; invite time zone is \"%2\".")
                .arg(mcTimezone,
                     m_TimezoneDetails[TimezoneName]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }

    // Format: FREQ=YEARLY;INTERVAL=1;BYDAY=1SU;BYMONTH=11
    // We're assuming this means we're switching on the first Sunday of
    // November

    // Daylight saving time start and end
    const QDateTime dst_start = GetTimeChange(
        m_TimezoneDetails[TimezoneDaylightSavingTime_RepeatRule],
        mcOriginalDateTime);
    const QDateTime dst_end = GetTimeChange(
        m_TimezoneDetails[TimezoneStandardTime_RepeatRule],
        mcOriginalDateTime);


    // General idea:
    // Jan 01 is standard time.
    // Daylight saving time starts when indicated
    // Back to standard time when indicated
    int offset_min = 0;
    if (mcOriginalDateTime < dst_start ||
        mcOriginalDateTime > dst_end)
    {
        // Standard time
        offset_min =
            m_TimezoneDetails[TimezoneStandardTime_OffsetTo_Min].toInt();
    } else
    {
        // Daylight saving time
        offset_min =
            m_TimezoneDetails[TimezoneDaylightSavingTime_OffsetTo_Min].toInt();
    }
    QDateTime utc = mcOriginalDateTime.addSecs(-offset_min * 60);

    CALL_OUT("");
    return utc;
}



///////////////////////////////////////////////////////////////////////////////
// Get time change
QDateTime CalendarEntry::GetTimeChange(const QString mcParameters,
    const QDateTime mcOriginalDateTime) const
{
    CALL_IN(QString("mcParameters=%1, mcOriginalDateTime=%2")
        .arg(CALL_SHOW(mcParameters),
             CALL_SHOW(mcOriginalDateTime)));

    // FREQ=YEARLY;BYMONTH=3;BYDAY=2SU
    // FREQ=YEARLY;INTERVAL=1;BYDAY=-1SU;BYMONTH=10

    // Parse parameters
    QHash < QString, QString > parameters;
    QStringList split = mcParameters.split(";");
    for (const QString & part : split)
    {
        QStringList this_split = part.split("=");
        if (this_split.size() != 2)
        {
            // Error
            const QString reason(tr("Parameter cannot be split in \"%1\"")
                .arg(mcParameters));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return QDateTime();
        }

        // Otherwise, remember parameter/value pair
        parameters[this_split[0]] = this_split[1];
    }

    // Check for required information
    if (!parameters.contains("FREQ") ||
        !parameters.contains("BYMONTH") ||
        !parameters.contains("BYDAY"))
    {
        // Error
        const QString reason(
            tr("Need FREQ, BYMONTH, and BYDAY parameters: \"%1\"")
                .arg(mcParameters));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }

    // Gather data
    QString frequency = parameters["FREQ"];
    static const QRegularExpression format_byday("^(-?[0-9]+)([A-Z]+)$");
    const QRegularExpressionMatch match_byday =
        format_byday.match(parameters["BYDAY"]);
    if (!match_byday.hasMatch())
    {
        const QString reason(tr("Invalid BYDAY parameter: \"%1\"")
            .arg(mcParameters));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }
    int nth = match_byday.captured(1).toInt();
    QString weekday = match_byday.captured(2);
    int month = parameters["BYMONTH"].toInt();
    if (frequency != "YEARLY")
    {
        const QString reason(
            tr("Repeat frequency \"%1\" is not yet implemented.")
                .arg(frequency));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }
    if (parameters.contains("INTERVAL") &&
        parameters["INTERVAL"] != "1")
    {
        const QString reason(
            tr("Interval \"%1\" is not yet implemented.")
                .arg(parameters["INTERVAL"]));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }
    if (weekday != "SU")
    {
        const QString reason(
            tr("Start weekday \"%1\" is not yet implemented.")
                .arg(weekday));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDateTime();
    }
    const int year = mcOriginalDateTime.date().year();
    QDate tmp_date;
    if (nth > 0)
    {
        // Relative to start of month
        tmp_date = QDate(year, month, 1);
        tmp_date = tmp_date.addDays(7 - tmp_date.dayOfWeek());
        tmp_date = tmp_date.addDays(7 * (nth - 1));
    } else
    {
        // Relative to end of month
        tmp_date = QDate(year, month, 1);
        tmp_date = QDate(year, month, tmp_date.daysInMonth());
        if (tmp_date.dayOfWeek() != 7)
        {
            tmp_date = tmp_date.addDays(-tmp_date.dayOfWeek());
        }
        tmp_date = tmp_date.addDays(-7 * (nth - 1));
    }
    QDateTime epoch_start = QDateTime::fromString(
        m_TimezoneDetails[TimezoneDaylightSavingTime_StartDateTime],
        "yyyy-MM-dd hh:mm:ss");
    const QDateTime start(tmp_date, epoch_start.time());

    CALL_OUT("");
    return start;
}



// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Calendar entry
QHash < CalendarEntry::CalendarEntryDetails, QString >
    CalendarEntry::GetEntryDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_EntryDetails;
}



///////////////////////////////////////////////////////////////////////////////
// Details about organizer
QHash < CalendarEntry::PersonDetails, QString >
    CalendarEntry::GetOrganizerDetails() const
{
    CALL_IN("");

    // Get organizer
    QHash < CalendarEntry::PersonDetails, QString > organizer;
    for (int index = 0;
         index < m_ParticipantsDetails.size();
         index++)
    {
        if (m_ParticipantsDetails[index][PersonRole] == "organizer")
        {
            organizer = m_ParticipantsDetails[index];
            break;
        }
    }

    // Also ok if no organizer was found. Unusual, but ok.

    CALL_OUT("");
    return organizer;
}



///////////////////////////////////////////////////////////////////////////////
// Details about participants (including organizer)
QList < QHash < CalendarEntry::PersonDetails, QString > >
    CalendarEntry::GetParticipantsDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_ParticipantsDetails;
}



///////////////////////////////////////////////////////////////////////////////
// Timezone
QHash < CalendarEntry::TimezoneDetails, QString >
    CalendarEntry::GetTimezoneDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_TimezoneDetails;
}



///////////////////////////////////////////////////////////////////////////////
// Start
QHash < CalendarEntry::DateTimeDetails, QString >
    CalendarEntry::GetStartDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_StartDetails;
}



///////////////////////////////////////////////////////////////////////////////
// Finish
QHash < CalendarEntry::DateTimeDetails, QString >
    CalendarEntry::GetEndDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_EndDetails;
}



///////////////////////////////////////////////////////////////////////////////
// Alarm details
QHash < CalendarEntry::AlarmDetails, QString >
    CalendarEntry::GetAlarmDetails() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_AlarmDetails;
}



// ====================================================================== Debug



///////////////////////////////////////////////////////////////////////////////
// Show everything
void CalendarEntry::Dump() const
{
    CALL_IN("");

    QString text;

    text += QString("===== Entry Details\n");
    if (m_EntryDetails.contains(EntryMethod))
    {
        text += QString("    EntryMethod: %1\n")
            .arg(m_EntryDetails[EntryMethod]);
    }
    if (m_EntryDetails.contains(EntryProduct))
    {
        text += QString("    EntryProduct: %1\n")
            .arg(m_EntryDetails[EntryProduct]);
    }
    if (m_EntryDetails.contains(EntryVersion))
    {
        text += QString("    EntryVersion: %1\n")
            .arg(m_EntryDetails[EntryVersion]);
    }
    if (m_EntryDetails.contains(EntryCalendarScale))
    {
        text += QString("    EntryCalendarScale: %1\n")
            .arg(m_EntryDetails[EntryCalendarScale]);
    }
    if (m_EntryDetails.contains(EntryDescriptionLanguage))
    {
        text += QString("    EntryDescriptionLanguage: %1\n")
            .arg(m_EntryDetails[EntryDescriptionLanguage]);
    }
    if (m_EntryDetails.contains(EntryDescription))
    {
        text += QString("    EntryDescription: %1\n")
            .arg(CALL_SHOW(m_EntryDetails[EntryDescription]));
    }
    if (m_EntryDetails.contains(EntryCategories))
    {
        text += QString("    EntryCategories: %1\n")
            .arg(m_EntryDetails[EntryCategories]);
    }
    if (m_EntryDetails.contains(EntryUID))
    {
        text += QString("    EntryUID: %1\n")
            .arg(m_EntryDetails[EntryUID]);
    }
    if (m_EntryDetails.contains(EntrySummaryLanguage))
    {
        text += QString("    EntrySummaryLanguage: %1\n")
            .arg(m_EntryDetails[EntrySummaryLanguage]);
    }
    if (m_EntryDetails.contains(EntrySummary))
    {
        text += QString("    EntrySummary: %1\n")
            .arg(m_EntryDetails[EntrySummary]);
    }
    if (m_EntryDetails.contains(EntryClass))
    {
        text += QString("    EntryClass: %1\n")
            .arg(m_EntryDetails[EntryClass]);
    }
    if (m_EntryDetails.contains(EntryPriority))
    {
        text += QString("    EntryPriority: %1\n")
            .arg(m_EntryDetails[EntryPriority]);
    }
    if (m_EntryDetails.contains(EntryDateTimeSentOriginalTimezone))
    {
        text += QString("    EntryDateTimeSentOriginalTimezone: %1\n")
            .arg(m_EntryDetails[EntryDateTimeSentOriginalTimezone]);
    }
    if (m_EntryDetails.contains(EntryDateTimeSentUTC))
    {
        text += QString("    EntryDateTimeSentUTC: %1\n")
            .arg(m_EntryDetails[EntryDateTimeSentUTC]);
    }
    if (m_EntryDetails.contains(EntryCreatedDateTimeOriginalTimezone))
    {
        text += QString("    EntryCreatedDateTimeOriginalTimezone: %1\n")
            .arg(m_EntryDetails[EntryCreatedDateTimeOriginalTimezone]);
    }
    if (m_EntryDetails.contains(EntryCreatedDateTimeUTC))
    {
        text += QString("    EntryCreatedDateTimeUTC: %1\n")
            .arg(m_EntryDetails[EntryCreatedDateTimeUTC]);
    }
    if (m_EntryDetails.contains(EntryLastModifiedDateTimeOriginalTimezone))
    {
        text += QString("    EntryLastModifiedDateTimeOriginalTimezone: %1\n")
            .arg(m_EntryDetails[EntryLastModifiedDateTimeOriginalTimezone]);
    }
    if (m_EntryDetails.contains(EntryLastModifiedDateTimeUTC))
    {
        text += QString("    EntryLastModifiedDateTimeUTC: %1\n")
            .arg(m_EntryDetails[EntryLastModifiedDateTimeUTC]);
    }
    if (m_EntryDetails.contains(EntryTransparency))
    {
        text += QString("    EntryTransparency: %1\n")
            .arg(m_EntryDetails[EntryTransparency]);
    }
    if (m_EntryDetails.contains(EntryStatus))
    {
        text += QString("    EntryStatus: %1\n")
            .arg(m_EntryDetails[EntryStatus]);
    }
    if (m_EntryDetails.contains(EntryRecurrenceID))
    {
        text += QString("    EntryRecurrenceID: %1\n")
            .arg(m_EntryDetails[EntryRecurrenceID]);
    }
    if (m_EntryDetails.contains(EntrySequence))
    {
        text += QString("    EntrySequence: %1\n")
            .arg(m_EntryDetails[EntrySequence]);
    }
    if (m_EntryDetails.contains(EntryLocationLanguage))
    {
        text += QString("    EntryLocationLanguage: %1\n")
            .arg(m_EntryDetails[EntryLocationLanguage]);
    }
    if (m_EntryDetails.contains(EntryLocation))
    {
        text += QString("    EntryLocation: %1\n")
            .arg(m_EntryDetails[EntryLocation]);
    }
    if (m_EntryDetails.contains(XAppointmentSequence))
    {
        text += QString("    XAppointmentSequence: %1\n")
            .arg(m_EntryDetails[XAppointmentSequence]);
    }
    if (m_EntryDetails.contains(XOwnerAppointmentID))
    {
        text += QString("    XOwnerAppointmentID: %1\n")
            .arg(m_EntryDetails[XOwnerAppointmentID]);
    }
    if (m_EntryDetails.contains(XBusyStatus))
    {
        text += QString("    XBusyStatus: %1\n")
            .arg(m_EntryDetails[XBusyStatus]);
    }
    if (m_EntryDetails.contains(XIntendedStatus))
    {
        text += QString("    XIntendedStatus: %1\n")
            .arg(m_EntryDetails[XIntendedStatus]);
    }
    if (m_EntryDetails.contains(XAllDayEvent))
    {
        text += QString("    XAllDayEvent: %1\n")
            .arg(m_EntryDetails[XAllDayEvent]);
    }
    if (m_EntryDetails.contains(XImportance))
    {
        text += QString("    XImportance: %1\n")
            .arg(m_EntryDetails[XImportance]);
    }
    if (m_EntryDetails.contains(XInstType))
    {
        text += QString("    XInstType: %1\n")
            .arg(m_EntryDetails[XInstType]);
    }
    if (m_EntryDetails.contains(XLocationURI))
    {
        text += QString("    XLocationURI: %1\n")
            .arg(m_EntryDetails[XLocationURI]);
    }
    if (m_EntryDetails.contains(XLatitude))
    {
        text += QString("    XLatitude: %1\n")
            .arg(m_EntryDetails[XLatitude]);
    }
    if (m_EntryDetails.contains(XLongitude))
    {
        text += QString("    XLongitude: %1\n")
            .arg(m_EntryDetails[XLongitude]);
    }
    if (m_EntryDetails.contains(XOnlineMeetingExternalLink))
    {
        text += QString("    XOnlineMeetingExternalLink: %1\n")
            .arg(m_EntryDetails[XOnlineMeetingExternalLink]);
    }
    if (m_EntryDetails.contains(XOnlineMeetingConferenceLink))
    {
        text += QString("    XOnlineMeetingConferenceLink: %1\n")
                    .arg(m_EntryDetails[XOnlineMeetingConferenceLink]);
    }
    if (m_EntryDetails.contains(XOnlineMeetingConferenceID))
    {
        text += QString("    XOnlineMeetingConferenceID: %1\n")
                    .arg(m_EntryDetails[XOnlineMeetingConferenceID]);
    }
    if (m_EntryDetails.contains(XOnlineMeetingInformation))
    {
        text += QString("    XOnlineMeetingInformation: %1\n")
                    .arg(m_EntryDetails[XOnlineMeetingInformation]);
    }
    if (m_EntryDetails.contains(XOnlineMeetingTollNumber))
    {
        text += QString("    XOnlineMeetingTollNumber: %1\n")
                    .arg(m_EntryDetails[XOnlineMeetingTollNumber]);
    }
    if (m_EntryDetails.contains(XDoNotForwardMeeting))
    {
        text += QString("    XDoNotForwardMeeting: %1\n")
            .arg(m_EntryDetails[XDoNotForwardMeeting]);
    }
    if (m_EntryDetails.contains(XDisallowCounterpropose))
    {
        text += QString("    XDisallowCounterpropose: %1\n")
            .arg(m_EntryDetails[XDisallowCounterpropose]);
    }
    if (m_EntryDetails.contains(XGoogleConference))
    {
        text += QString("    XGoogleConference: %1\n")
            .arg(m_EntryDetails[XGoogleConference]);
    }
    if (m_EntryDetails.contains(XLocationDisplayName))
    {
        text += QString("    XLocationDisplayName: %1\n")
            .arg(m_EntryDetails[XLocationDisplayName]);
    }
    if (m_EntryDetails.contains(XLocationSource))
    {
        text += QString("    XLocationSource: %1\n")
            .arg(m_EntryDetails[XLocationSource]);
    }
    if (m_EntryDetails.contains(XLocations))
    {
        text += QString("    XLocations: %1\n")
            .arg(m_EntryDetails[XLocations]);
    }
    if (m_EntryDetails.contains(XSkypeTeamsMeetingURL))
    {
        text += QString("    XSkypeTeamsMeetingURL: %1\n")
            .arg(m_EntryDetails[XSkypeTeamsMeetingURL]);
    }
    if (m_EntryDetails.contains(XSkypeTeamsProperties))
    {
        text += QString("    XSkypeTeamsProperties: %1\n")
            .arg(m_EntryDetails[XSkypeTeamsProperties]);
    }

    text += QString("===== Participants Details\n");
    for (int index = 0;
         index < m_ParticipantsDetails.size();
         index++)
    {
        text += QString("  === Participant %1\n")
            .arg(QString::number(index));
        if (m_ParticipantsDetails[index].contains(PersonName))
        {
            text += QString("    PersonName: %1\n")
                .arg(m_ParticipantsDetails[index][PersonName]);
        }
        if (m_ParticipantsDetails[index].contains(PersonEmailAddress))
        {
            text += QString("    PersonEmailAddress: %1\n")
                .arg(m_ParticipantsDetails[index][PersonEmailAddress]);
        }
        if (m_ParticipantsDetails[index].contains(PersonRole))
        {
            text += QString("    PersonRole: %1\n")
                .arg(m_ParticipantsDetails[index][PersonRole]);
        }
        if (m_ParticipantsDetails[index].contains(PersonType))
        {
            text += QString("    PersonType: %1\n")
                .arg(m_ParticipantsDetails[index][PersonType]);
        }
        if (m_ParticipantsDetails[index].contains(PersonParticipationStatus))
        {
            text += QString("    PersonParticipationStatus: %1\n")
                .arg(m_ParticipantsDetails[index][PersonParticipationStatus]);
        }
        if (m_ParticipantsDetails[index].contains(PersonRSVP))
        {
            text += QString("    PersonRSVP: %1\n")
                .arg(m_ParticipantsDetails[index][PersonRSVP]);
        }
        if (m_ParticipantsDetails[index].contains(PersonXNumberOfGuests))
        {
            text += QString("    PersonXNumberOfGuests: %1\n")
                .arg(m_ParticipantsDetails[index][PersonXNumberOfGuests]);
        }
    }

    text += QString("===== Timezone Details\n");
    if (m_TimezoneDetails.contains(TimezoneName))
    {
        text += QString("    TimezoneName: %1\n")
            .arg(m_TimezoneDetails[TimezoneName]);
    }
    if (m_TimezoneDetails.contains(TimezoneLocation))
    {
        text += QString("    TimezoneLocation: %1\n")
            .arg(m_TimezoneDetails[TimezoneLocation]);
    }
    if (m_TimezoneDetails.contains(TimezoneStandardTime_Name))
    {
        text += QString("    TimezoneStandardTime_Name: %1\n")
            .arg(m_TimezoneDetails[TimezoneStandardTime_Name]);
    }
    if (m_TimezoneDetails.contains(TimezoneStandardTime_StartDateTime))
    {
        text += QString("    TimezoneStandardTime_StartDateTime: %1\n")
            .arg(m_TimezoneDetails[TimezoneStandardTime_StartDateTime]);
    }
    if (m_TimezoneDetails.contains(TimezoneStandardTime_OffsetFrom_Min))
    {
        text += QString("    TimezoneStandardTime_OffsetFrom_Min: %1\n")
            .arg(m_TimezoneDetails[TimezoneStandardTime_OffsetFrom_Min]);
    }
    if (m_TimezoneDetails.contains(TimezoneStandardTime_OffsetTo_Min))
    {
        text += QString("    TimezoneStandardTime_OffsetTo_Min: %1\n")
            .arg(m_TimezoneDetails[TimezoneStandardTime_OffsetTo_Min]);
    }
    if (m_TimezoneDetails.contains(TimezoneStandardTime_RepeatRule))
    {
        text += QString("    TimezoneStandardTime_RepeatRule: %1\n")
            .arg(m_TimezoneDetails[TimezoneStandardTime_RepeatRule]);
    }
    if (m_TimezoneDetails.contains(TimezoneDaylightSavingTime_Name))
    {
        text += QString("    TimezoneDaylightSavingTime_Name: %1\n")
            .arg(m_TimezoneDetails[TimezoneDaylightSavingTime_Name]);
    }
    if (m_TimezoneDetails.contains(TimezoneDaylightSavingTime_StartDateTime))
    {
        text += QString("    TimezoneDaylightSavingTime_StartDateTime: %1\n")
            .arg(m_TimezoneDetails[TimezoneDaylightSavingTime_StartDateTime]);
    }
    if (m_TimezoneDetails.contains(TimezoneDaylightSavingTime_OffsetFrom_Min))
    {
        text += QString("    TimezoneDaylightSavingTime_OffsetFrom_Min: %1\n")
            .arg(m_TimezoneDetails[TimezoneDaylightSavingTime_OffsetFrom_Min]);
    }
    if (m_TimezoneDetails.contains(TimezoneDaylightSavingTime_OffsetTo_Min))
    {
        text += QString("    TimezoneDaylightSavingTime_OffsetTo_Min: %1\n")
            .arg(m_TimezoneDetails[TimezoneDaylightSavingTime_OffsetTo_Min]);
    }
    if (m_TimezoneDetails.contains(TimezoneDaylightSavingTime_RepeatRule))
    {
        text += QString("    TimezoneDaylightSavingTime_RepeatRule: %1\n")
            .arg(m_TimezoneDetails[TimezoneDaylightSavingTime_RepeatRule]);
    }

    text += QString("===== Start Details\n");
    if (m_StartDetails.contains(DateTimeOriginalTimezoneName))
    {
        text += QString("    DateTimeOriginalTimezoneName: %1\n")
            .arg(m_StartDetails[DateTimeOriginalTimezoneName]);
    }
    if (m_StartDetails.contains(DateTimeOriginalDateTime))
    {
        text += QString("    DateTimeOriginalDateTime: %1\n")
            .arg(m_StartDetails[DateTimeOriginalDateTime]);
    }
    if (m_StartDetails.contains(DateTimeUTCDateTime))
    {
        text += QString("    DateTimeUTCDateTime: %1\n")
            .arg(m_StartDetails[DateTimeUTCDateTime]);
    }

    text += QString("===== End Details\n");
    if (m_EndDetails.contains(DateTimeOriginalTimezoneName))
    {
        text += QString("    DateTimeOriginalTimezoneName: %1\n")
            .arg(m_EndDetails[DateTimeOriginalTimezoneName]);
    }
    if (m_EndDetails.contains(DateTimeOriginalDateTime))
    {
        text += QString("    DateTimeOriginalDateTime: %1\n")
            .arg(m_EndDetails[DateTimeOriginalDateTime]);
    }
    if (m_EndDetails.contains(DateTimeUTCDateTime))
    {
        text += QString("    DateTimeUTCDateTime: %1\n")
            .arg(m_EndDetails[DateTimeUTCDateTime]);
    }

    text += QString("===== Alarm Details\n");
    if (m_AlarmDetails.contains(AlarmDescription))
    {
        text += QString("    AlarmDescription: %1\n")
            .arg(m_AlarmDetails[AlarmDescription]);
    }
    if (m_AlarmDetails.contains(AlarmTriggerRelated))
    {
        text += QString("    AlarmTriggerRelated: %1\n")
            .arg(m_AlarmDetails[AlarmTriggerRelated]);
    }
    if (m_AlarmDetails.contains(AlarmTriggerOffset))
    {
        text += QString("    AlarmTriggerOffset: %1\n")
            .arg(m_AlarmDetails[AlarmTriggerOffset]);
    }
    if (m_AlarmDetails.contains(AlarmAction))
    {
        text += QString("    AlarmAction: %1\n")
            .arg(m_AlarmDetails[AlarmAction]);
    }

    qDebug().noquote() << text;

    CALL_OUT("");
}
