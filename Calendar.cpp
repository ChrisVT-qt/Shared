// Calendar.cpp
// Class implementation

// Project includes
#include "Calendar.h"
#include "CallTracer.h"
#include "MessageLogger.h"

// Qt includes
#include <QRegularExpression>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
Calendar::Calendar()
{
    CALL_IN("");


    // == Initialize regular expressions
    const QString match_month(
        "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)");
    const QString match_day("(0[1-9]|[12][0-9]|3[01])");
    const QString match_weekday("(Mon|Tue|Wed|Thu|Fri|Sat|Sun)");
    const QString match_nth("(1st|2nd|3rd|4th|5th)");
    const QString match_year("([0-9]{4})");

    // Every May 01
    m_FormatEvery = QRegularExpression(QString("^Every %1 %2$")
        .arg(match_month,
             match_day));

    // Every 4th Thu in Nov
    m_FormatNth = QRegularExpression(QString("^Every %1 %2 in %3$")
        .arg(match_nth,
             match_weekday,
             match_month));

    // On 02 Sep 2024
    m_FormatDate = QRegularExpression(QString("^On %1 %2 %3$")
        .arg(match_day,
             match_month,
             match_year));

    // Initialize some holidays
    Init_Holidays();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Initialize some holidays
void Calendar::Init_Holidays()
{
    CALL_IN("");

    // ==== Add global holidays
    AddHoliday(tr("New Year"), "Every Jan 01");
    AddHoliday(tr("Christmas Day"), "Every Dec 25");


    // ==== Add holidays in Germany
    AddCountry("DE");

    // General
    AddHoliday("DE", tr("2. Weihnachtstag"), "Every Dec 26");
    AddHoliday("DE", tr("Tag der Deutschen Einheit"), "Every Oct 03");
    AddHoliday("DE", tr("Tag der Arbeit"), "Every May 01");

    // 2024
    AddHoliday("DE", tr("Karfreitag"), "On 29 Mar 2024");
    AddHoliday("DE", tr("Ostermontag"), "On 01 Apr 2024");
    AddHoliday("DE", tr("Christi Himmelfahrt"), "On 09 May 2024");
    AddHoliday("DE", tr("Pfingstmontag"), "On 20 May 2024");


    // ==== Add holidays in the US
    AddCountry("US");

    // General
    AddHoliday("US", tr("Thanksgiving"), "Every 4th Thu in Nov");

    // 2024
    AddHoliday("US", tr("Memorial Day"), "On 27 May 2024");
    AddHoliday("US", tr("Independence Day"), "On 04 Jul 2024");
    AddHoliday("US", tr("Labor Day"), "On 02 Sep 2024");

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
Calendar::~Calendar()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Instanciator
Calendar * Calendar::Instance()
{
    CALL_IN("");

    if (!m_Instance)
    {
        m_Instance = new Calendar();
    }

    CALL_OUT("");
    return m_Instance;
}



///////////////////////////////////////////////////////////////////////////////
// Instance
Calendar * Calendar::m_Instance = nullptr;



// ================================================================== Calendars



///////////////////////////////////////////////////////////////////////////////
// Add known country
bool Calendar::AddCountry(const QString & mcrCountry)
{
    CALL_IN(QString("mcrCountry=%1")
        .arg(CALL_SHOW(mcrCountry)));

    // Country cannot be empty
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Country provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country already exists
    if (m_Countries.contains(mcrCountry))
    {
        CALL_OUT("");
        return true;
    }

    // Otherwise, add it.
    m_Countries += mcrCountry;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Remove known country
bool Calendar::DeleteCountry(const QString & mcrCountry)
{
    CALL_IN(QString("mcrCountry=%1")
        .arg(CALL_SHOW(mcrCountry)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist.")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Remove country
    m_Countries -= mcrCountry;
    m_CountryToRegions.remove(mcrCountry);
    m_CountryToHolidays.remove(mcrCountry);
    m_CountryRegionToHolidays.remove(mcrCountry);
    for (auto person_iterator = m_PersonInfo.keyBegin();
         person_iterator != m_PersonInfo.keyEnd();
         person_iterator++)
    {
        const QString person = *person_iterator;
        if (!m_PersonInfo[person].contains("country"))
        {
            continue;
        }
        m_PersonInfo[person].remove("country");
        m_PersonInfo[person].remove("region");
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Add known region
bool Calendar::AddRegion(const QString & mcrCountry, const QString & mcrRegion)
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion)));

    // Country cannot be empty
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Country provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist yet.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Region cannot be empty
    if (mcrRegion.isEmpty())
    {
        const QString reason = tr("Region provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region alsready exists
    if (m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        CALL_OUT("");
        return true;
    }

    // Otherwise, add it.
    m_CountryToRegions[mcrCountry] += mcrRegion;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Remove known region
bool Calendar::DeleteRegion(const QString & mcrCountry,
    const QString & mcrRegion)
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion)));

    // Country cannot be empty
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Country provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist yet.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Region cannot be empty
    if (mcrRegion.isEmpty())
    {
        const QString reason = tr("Region provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Region \"%1\" does not exist in \"%2\".")
            .arg(mcrRegion,
                 mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Delete region
    m_CountryToRegions[mcrCountry] -= mcrRegion;
    if (m_CountryToRegions[mcrCountry].isEmpty())
    {
        m_CountryToRegions.remove(mcrCountry);
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Get all known countries
QSet < QString > Calendar::GetAvailableCountries() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_Countries;
}



///////////////////////////////////////////////////////////////////////////////
// Get all known regions for a country
QSet < QString > Calendar::GetAvailableRegions(
    const QString & mcrCountry) const
{
    CALL_IN(QString("mcrCountry=%1")
        .arg(CALL_SHOW(mcrCountry)));

    // Country cannot be empty
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Country provided is empty.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QSet < QString >();
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QSet < QString >();
    }

    if (m_CountryToRegions.contains(mcrCountry))
    {
        CALL_OUT("");
        return m_CountryToRegions[mcrCountry];
    } else
    {
        CALL_OUT("");
        return QSet < QString >();
    }

    // We never get here
}



// =================================================================== Holidays



///////////////////////////////////////////////////////////////////////////////
// Add general holiday
bool Calendar::AddHoliday(const QString & mcrName, const QString & mcrRule)
{
    CALL_IN(QString("mcrName=%1, mcrRule=%2")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcrRule)));

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for duplicates
    for (int index = 0;
         index < m_GlobalHolidays.size();
         index++)
    {
        if (m_GlobalHolidays[index]["name"] == mcrName)
        {
            const QString reason =
                tr("A global holiday \"%1\" has already been defined "
                    "(existing rule: \"%2\", new rule: \"%3\").")
                    .arg(mcrName,
                         m_GlobalHolidays[index]["rule"],
                         mcrRule);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }

    // Check if rule has been provided
    if (mcrRule.isEmpty())
    {
        const QString reason = tr("No rule has been provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if rule has a valid format
    while (true)
    {
        QRegularExpressionMatch match = m_FormatEvery.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatNth.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatDate.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }

        // Not in proper format
        const QString reason = tr("Invalid rule \"%1\"")
            .arg(mcrRule);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Store rule
    QHash < QString, QString > new_holiday;
    new_holiday["name"] = mcrName;
    new_holiday["rule"] = mcrRule;
    m_GlobalHolidays << new_holiday;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Add country-specific holiday
bool Calendar::AddHoliday(const QString & mcrCountry, const QString & mcrName,
    const QString & mcrRule)
{
    CALL_IN(QString("mcrCountry=%1, mcrName=%2, mcrRule=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrName),
             CALL_SHOW(mcrRule)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for duplicates
    for (int index = 0;
         index < m_CountryToHolidays[mcrCountry].size();
         index++)
    {
        if (m_CountryToHolidays[mcrCountry][index]["name"] == mcrName)
        {
            const QString reason =
                tr("A holiday \"%1\" for country \"%2\" has already been "
                    "defined (existing rule: \"%3\", new rule: \"%4\").")
                    .arg(mcrName,
                         mcrCountry,
                         m_CountryToHolidays[mcrCountry][index]["rule"],
                         mcrRule);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }

    // Check if rule has been provided
    if (mcrRule.isEmpty())
    {
        const QString reason = tr("No rule has been provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if rule has a valid format
    while (true)
    {
        QRegularExpressionMatch match = m_FormatEvery.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatNth.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatDate.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }

        // Not in proper format
        const QString reason = tr("Invalid rule \"%1\"")
            .arg(mcrRule);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Store rule
    QHash < QString, QString > new_holiday;
    new_holiday["name"] = mcrName;
    new_holiday["rule"] = mcrRule;
    m_CountryToHolidays[mcrCountry] << new_holiday;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Add regional holiday
bool Calendar::AddHoliday(const QString & mcrCountry,
    const QString & mcrRegion, const QString & mcrName,
    const QString & mcrRule)
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcrName=%3, mcrRule=%4")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcrName),
             CALL_SHOW(mcrRule)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for duplicates
    for (int index = 0;
         index < m_CountryRegionToHolidays[mcrCountry][mcrRegion].size();
         index++)
    {
        if (m_CountryRegionToHolidays[mcrCountry][mcrRegion][index]["name"] ==
            mcrName)
        {
            const QString reason =
                tr("A holiday \"%1\" for country \"%2\" has already been "
                    "defined (existing rule: \"%3\", new rule: \"%4\").")
                    .arg(mcrName,
                         mcrCountry,
                         m_CountryRegionToHolidays[mcrCountry][mcrRegion]
                            [index]["rule"],
                         mcrRule);
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }

    // Check if rule has been provided
    if (mcrRule.isEmpty())
    {
        const QString reason = tr("No rule has been provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if rule has a valid format
    while (true)
    {
        QRegularExpressionMatch match = m_FormatEvery.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatNth.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }
        match = m_FormatDate.match(mcrRule);
        if (match.hasMatch())
        {
            break;
        }

        // Not in proper format
        const QString reason = tr("Invalid rule \"%1\"")
            .arg(mcrRule);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Store rule
    QHash < QString, QString > new_holiday;
    new_holiday["name"] = mcrName;
    new_holiday["rule"] = mcrRule;
    m_CountryRegionToHolidays[mcrCountry][mcrRegion] << new_holiday;

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Delete general holiday
bool Calendar::DeleteHoliday(const QString & mcrName)
{
    CALL_IN(QString("mcrName=%1")
        .arg(CALL_SHOW(mcrName)));

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name exists
    for (int index = 0;
         index < m_GlobalHolidays.size();
         index++)
    {
        if (m_GlobalHolidays[index]["name"] == mcrName)
        {
            m_GlobalHolidays.removeAt(index);
            CALL_OUT("");
            return true;
        }
    }

    // Not found
    const QString reason = tr("Global holiday \"%1\" could not be found.")
        .arg(mcrName);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Delete country-specific holiday
bool Calendar::DeleteHoliday(const QString & mcrCountry,
    const QString & mcrName)
{
    CALL_IN(QString("mcrCountry=%1, mcrName=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrName)));

    // Check if country has been provided
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Empty country name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist.")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name exists
    for (int index = 0;
         index < m_CountryToHolidays[mcrCountry].size();
         index++)
    {
        if (m_CountryToHolidays[mcrCountry][index]["name"] == mcrName)
        {
            m_CountryToHolidays[mcrCountry].removeAt(index);
            CALL_OUT("");
            return true;
        }
    }

    // Not found
    const QString reason =
        tr("Holiday \"%1\" for country \"%2\" could not be found.")
            .arg(mcrName,
                 mcrCountry);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Delete regional holiday
bool Calendar::DeleteHoliday(const QString & mcrCountry,
    const QString & mcrRegion, const QString & mcrName)
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcrName=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcrName)));

    // Check if country has been provided
    if (mcrCountry.isEmpty())
    {
        const QString reason = tr("Empty country name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Country \"%1\" does not exist.")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region has been provided
    if (mcrRegion.isEmpty())
    {
        const QString reason = tr("Empty region name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason =
            tr("Region \"%1\" does not exist for country \"%2\".")
                .arg(mcrRegion,
                     mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name has been provided
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty holiday name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if name exists
    for (int index = 0;
         index < m_CountryRegionToHolidays[mcrCountry][mcrRegion].size();
         index++)
    {
        if (m_CountryRegionToHolidays[mcrCountry][mcrRegion][index]["name"]
            == mcrName)
        {
            m_CountryToHolidays[mcrCountry].removeAt(index);
            CALL_OUT("");
            return true;
        }
    }

    // Not found
    const QString reason =
        tr("Holiday \"%1\" for country \"%2\" could not be found.")
            .arg(mcrName,
                 mcrCountry);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Get global holidays for a particular date
QStringList Calendar::GetHolidays(const QDate mcDate) const
{
    CALL_IN(QString("mcDate=%1")
        .arg(CALL_SHOW(mcDate)));

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Get matching global holidays
    QStringList holidays;
    for (int index = 0;
         index < m_GlobalHolidays.size();
         index++)
    {
        const bool holiday_matches =
            DoesDateMatchRule(mcDate, m_GlobalHolidays[index]["rule"]);
        if (holiday_matches)
        {
            holidays << m_GlobalHolidays[index]["name"];
        }
    }

    CALL_OUT("");
    return holidays;
}



///////////////////////////////////////////////////////////////////////////////
// Get holidays for a specific country
QStringList Calendar::GetHolidays(const QString & mcrCountry,
    const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Get matching country holidays
    QStringList holidays;
    for (int index = 0;
         index < m_CountryToHolidays[mcrCountry].size();
         index++)
    {
        const bool holiday_matches = DoesDateMatchRule(mcDate,
            m_CountryToHolidays[mcrCountry][index]["rule"]);
        if (holiday_matches)
        {
            holidays << m_CountryToHolidays[mcrCountry][index]["name"];
        }
    }

    CALL_OUT("");
    return holidays;
}



///////////////////////////////////////////////////////////////////////////////
// Get regional holidays
QStringList Calendar::GetHolidays(const QString & mcrCountry,
    const QString & mcrRegion, const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcDate=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QStringList();
    }

    // Get matching regional holidays
    QStringList holidays;
    for (int index = 0;
         index < m_CountryRegionToHolidays[mcrCountry][mcrRegion].size();
         index++)
    {
        const bool holiday_matches = DoesDateMatchRule(mcDate,
            m_CountryRegionToHolidays[mcrCountry][mcrRegion][index]["rule"]);
        if (holiday_matches)
        {
            holidays << m_CountryRegionToHolidays[mcrCountry][mcrRegion]
                [index]["name"];
        }
    }

    CALL_OUT("");
    return holidays;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a holiday
bool Calendar::IsHoliday(const QDate mcDate) const
{
    CALL_IN(QString("mcDate=%1")
        .arg(CALL_SHOW(mcDate)));

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for matching global holidays
    for (int index = 0;
         index < m_GlobalHolidays.size();
         index++)
    {
        const bool holiday_matches =
            DoesDateMatchRule(mcDate, m_GlobalHolidays[index]["rule"]);
        if (holiday_matches)
        {
            CALL_OUT("");
            return true;
        }
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a holiday in a particular country
bool Calendar::IsHoliday(const QString & mcrCountry, const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check global level
    if (IsHoliday(mcDate))
    {
        CALL_OUT("");
        return true;
    }

    // Check for matching country holidays
    for (int index = 0;
         index < m_CountryToHolidays[mcrCountry].size();
         index++)
    {
        const bool holiday_matches = DoesDateMatchRule(mcDate,
            m_CountryToHolidays[mcrCountry][index]["rule"]);
        if (holiday_matches)
        {
            CALL_OUT("");
            return true;
        }
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a holiday in a particular region
bool Calendar::IsHoliday(const QString & mcrCountry, const QString & mcrRegion,
    const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcDate=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check global and country level
    if (IsHoliday(mcDate) ||
        IsHoliday(mcrCountry, mcDate))
    {
        CALL_OUT("");
        return true;
    }

    // Check for matching regional holidays
    for (int index = 0;
         index < m_CountryRegionToHolidays[mcrCountry][mcrRegion].size();
         index++)
    {
        const bool holiday_matches = DoesDateMatchRule(mcDate,
            m_CountryRegionToHolidays[mcrCountry][mcrRegion][index]["rule"]);
        if (holiday_matches)
        {
            CALL_OUT("");
            return true;
        }
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a work day
bool Calendar::IsWorkDay(const QDate mcDate) const
{
    CALL_IN(QString("mcDate=%1")
        .arg(CALL_SHOW(mcDate)));

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for workday
    const bool is_workday =
        (mcDate.dayOfWeek() != 6) &&
        (mcDate.dayOfWeek() != 7) &&
        !IsHoliday(mcDate);

    CALL_OUT("");
    return is_workday;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a workd day in a particular country
bool Calendar::IsWorkDay(const QString & mcrCountry, const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for workday
    const bool is_workday =
        (mcDate.dayOfWeek() != 6) &&
        (mcDate.dayOfWeek() != 7) &&
        !IsHoliday(mcrCountry, mcDate);

    CALL_OUT("");
    return is_workday;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a date is a work day in a particular region
bool Calendar::IsWorkDay(const QString & mcrCountry, const QString & mcrRegion,
    const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcDate=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check for workday
    const bool is_workday =
        (mcDate.dayOfWeek() != 6) &&
        (mcDate.dayOfWeek() != 7) &&
        !IsHoliday(mcrCountry, mcrRegion, mcDate);

    CALL_OUT("");
    return is_workday;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a particular date matches a rule
bool Calendar::DoesDateMatchRule(const QDate mcDate,
    const QString & mcrRule) const
{
    CALL_IN(QString("mcDate=%1, mcrRule=%2")
        .arg(CALL_SHOW(mcDate),
             CALL_SHOW(mcrRule)));

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check rule
    QRegularExpressionMatch match = m_FormatEvery.match(mcrRule);
    if (match.hasMatch())
    {
        const QString day = match.captured(1);
        const QString month = match.captured(2);
        const QString match_date = day + " " + month;
        const bool status = (mcDate.toString("MMM dd") == match_date);
        CALL_OUT("");
        return status;
    }

    match = m_FormatNth.match(mcrRule);
    if (match.hasMatch())
    {
        const int n = match.captured(1).toInt();
        const QString weekday = match.captured(2);
        const QString month = match.captured(3);
        const bool status = (mcDate.toString("MMM") == month) &&
            (mcDate.toString("ddd") == weekday) &&
            (mcDate.day() >= (n-1)*7+1);
        CALL_OUT("");
        return status;
    }

    match = m_FormatDate.match(mcrRule);
    if (match.hasMatch())
    {
        const QString day = match.captured(1);
        const QString month = match.captured(2);
        const QString year = match.captured(3);
        const QString match_date = day + " " + month + " " + year;
        const bool status = (mcDate.toString("dd MMM yyyy") == match_date);
        CALL_OUT("");
        return status;
    }

    // Invalid rule
    const QString reason = tr("Invalid rule \"%1\".")
        .arg(mcrRule);
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



// =================================================================== Vacation



///////////////////////////////////////////////////////////////////////////////
// Add person
bool Calendar::AddPerson(const QString & mcrName, const QString & mcrCountry,
    const QString & mcrRegion)
{
    CALL_IN(QString("mcrName=%1, mcrCountry=%2, mcrRegion=%3")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion)));

    // Check if name is empty
    if (mcrName.isEmpty())
    {
        const QString reason = tr("Empty name provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if person already exists
    if (m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" already exists.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Set up person
    QHash < QString, QString > new_person;
    new_person["name"] = mcrName;
    if (!mcrCountry.isEmpty())
    {
        new_person["country"] = mcrCountry;
        if (!mcrRegion.isEmpty())
        {
            new_person["region"] = mcrRegion;
        }
    }
    m_PersonInfo[mcrName] = new_person;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Remove person
bool Calendar::DeletePerson(const QString & mcrName)
{
    CALL_IN(QString("mcrName=%1")
        .arg(CALL_SHOW(mcrName)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Remove person
    m_PersonInfo.remove(mcrName);
    m_PersonToVacations.remove(mcrName);

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// All registered persons
QStringList Calendar::GetAllPersons() const
{
    CALL_IN("");

    QStringList persons(m_PersonInfo.keys());
    std::sort(persons.begin(), persons.end());

    CALL_OUT("");
    return persons;
}



///////////////////////////////////////////////////////////////////////////////
// Details of a particular person
QHash < QString, QString > Calendar::GetPersonDetails(
    const QString & mcrName) const
{
    CALL_IN(QString("mcrName=%1")
        .arg(CALL_SHOW(mcrName)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QHash < QString, QString >();
    }

    CALL_OUT("");
    return m_PersonInfo[mcrName];
}



///////////////////////////////////////////////////////////////////////////////
// Add vacation
bool Calendar::AddVacation(const QString & mcrName, const QDate mcFirstDay,
    const QDate mcLastDay)
{
    CALL_IN(QString("mcrName=%1, mcFirstDay=%2, mcLastDay=%3")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcFirstDay),
             CALL_SHOW(mcLastDay)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if dates are valid
    if (!mcFirstDay.isValid())
    {
        const QString reason = tr("First day provided is not valid.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }
    QDate last_day = mcLastDay;
    if (!mcLastDay.isValid())
    {
        last_day = mcFirstDay;
    }

    // Check if new vacation intersects with existing vacation
    for (int index = 0;
         index < m_PersonToVacations[mcrName].size();
         index++)
    {
        const QDate vacation_first_day =
            m_PersonToVacations[mcrName][index].first;
        const QDate vacation_last_day =
            m_PersonToVacations[mcrName][index].second;
        if ((mcFirstDay >= vacation_first_day &&
             mcFirstDay <= vacation_last_day) ||
            (last_day >= vacation_first_day &&
             last_day <= vacation_last_day))
        {
            const QString reason = tr("New vacation intersects with an "
                "existing one from %1 to %2.")
                .arg(vacation_first_day.toString("dd MMM yyyy"),
                     vacation_last_day.toString("dd MMM yyyy"));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }

    // Add new vacation
    QPair < QDate, QDate > new_vacation;
    new_vacation.first = mcFirstDay;
    new_vacation.second = last_day;
    m_PersonToVacations[mcrName] << new_vacation;

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Remove vacation
bool Calendar::DeleteVacation(const QString & mcrName, const QDate mcFirstDay)
{
    CALL_IN(QString("mcrName=%1, mcFirstDay=%2, mcLastDay=%3")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcFirstDay)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcFirstDay.isValid())
    {
        const QString reason = tr("Invalid date");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Delete vacation that starts on thay day
    for (int index = 0;
         index < m_PersonToVacations[mcrName].size();
         index++)
    {
        if (m_PersonToVacations[mcrName][index].first == mcFirstDay)
        {
            // Delete this vacation
            m_PersonToVacations[mcrName].removeAt(index);
            CALL_OUT("");
            return true;
        }
    }

    // Not found
    const QString reason = tr("No vacation for \"%1\" starting on \"%2\".")
        .arg(mcrName,
             mcFirstDay.toString("dd MMM yyyy"));
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a person is on vacation
bool Calendar::HasVacation(const QString & mcrName, const QDate mcDate) const
{
    CALL_IN(QString("mcrName=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcDate)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if person is on vacation
    for (int index = 0;
         index < m_PersonToVacations[mcrName].size();
         index++)
    {
        const QDate start_date = m_PersonToVacations[mcrName][index].first;
        const QDate end_date = m_PersonToVacations[mcrName][index].second;
        if (mcDate >= start_date &&
            mcDate <= end_date)
        {
            // Person is on vacation
            CALL_OUT("");
            return true;
        }
    }

    // Person is not on vacation
    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a person has a holiday
bool Calendar::HasHoliday(const QString & mcrName, const QDate mcDate) const
{
    CALL_IN(QString("mcrName=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcDate)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if person is on holiday
    QString country;
    QString region;
    if (m_PersonInfo[mcrName].contains("country"))
    {
        country = m_PersonInfo[mcrName]["country"];
        if (m_PersonInfo[mcrName].contains("region"))
        {
            region = m_PersonInfo[mcrName]["region"];
        }
    }
    const bool has_holiday = IsHoliday(country, region, mcDate);

    CALL_OUT("");
    return has_holiday;
}



///////////////////////////////////////////////////////////////////////////////
// Check if a person is away (vacation or Holiday)
bool Calendar::IsAway(const QString & mcrName, const QDate mcDate) const
{
    CALL_IN(QString("mcrName=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcDate)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    const bool is_away = HasVacation(mcrName, mcDate) ||
        HasHoliday(mcrName, mcDate);

    CALL_OUT("");
    return is_away;
}



// ============================================================ Date Management



///////////////////////////////////////////////////////////////////////////////
// Next working day
QDate Calendar::NextWorkingDay(const QDate mcDate) const
{
    CALL_IN(QString("mcDate=%1")
        .arg(CALL_SHOW(mcDate)));

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    QDate next_working_day = mcDate;
    int tries = 0;
    while (tries++ < 10000)
    {
        // Try next day
        next_working_day = next_working_day.addDays(1);

        // Not Sat/Sun
        if (next_working_day.dayOfWeek() == 6 ||
            next_working_day.dayOfWeek() == 7)
        {
            continue;
        }

        // Not a global holiday
        if (IsHoliday(next_working_day))
        {
            continue;
        }

        // This is the next working day
        CALL_OUT("");
        return next_working_day;
    }

    // No next working day.
    const QString reason = tr("No next working day found.");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return QDate();
}



///////////////////////////////////////////////////////////////////////////////
// Next working day in a particular country
QDate Calendar::NextWorkingDay(const QString & mcrCountry,
    const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcDate=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    QDate next_working_day = mcDate;
    int tries = 0;
    while (tries++ < 10000)
    {
        // Try next day
        next_working_day = next_working_day.addDays(1);

        // Not Sat/Sun
        if (next_working_day.dayOfWeek() == 6 ||
            next_working_day.dayOfWeek() == 7)
        {
            continue;
        }

        // Not a global holiday
        if (IsHoliday(next_working_day))
        {
            continue;
        }

        // Not a country holiday
        if (IsHoliday(mcrCountry, next_working_day))
        {
            continue;
        }

        // This is the next working day
        CALL_OUT("");
        return next_working_day;
    }

    // No next working day.
    const QString reason = tr("No next working day found.");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return QDate();
}



///////////////////////////////////////////////////////////////////////////////
// Next working day in a particular region
QDate Calendar::NextWorkingDay(const QString & mcrCountry,
    const QString & mcrRegion, const QDate mcDate) const
{
    CALL_IN(QString("mcrCountry=%1, mcrRegion=%2, mcDate=%3")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion),
             CALL_SHOW(mcDate)));

    // Check if country exists
    if (!m_Countries.contains(mcrCountry))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    // Check if region exists
    if (!m_CountryToRegions[mcrCountry].contains(mcrRegion))
    {
        const QString reason = tr("Unknown country \"%1\".")
            .arg(mcrCountry);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    QDate next_working_day = mcDate;
    int tries = 0;
    while (tries++ < 10000)
    {
        // Try next day
        next_working_day = next_working_day.addDays(1);

        // Not Sat/Sun
        if (next_working_day.dayOfWeek() == 6 ||
            next_working_day.dayOfWeek() == 7)
        {
            continue;
        }

        // Not a global holiday
        if (IsHoliday(next_working_day))
        {
            continue;
        }

        // Not a country holiday
        if (IsHoliday(mcrCountry, next_working_day))
        {
            continue;
        }

        // Not a regional holiday
        if (IsHoliday(mcrCountry, mcrRegion, next_working_day))
        {
            continue;
        }

        // This is the next working day
        CALL_OUT("");
        return next_working_day;
    }

    // No next working day.
    const QString reason = tr("No next working day found.");
    MessageLogger::Error(CALL_METHOD, reason);
    CALL_OUT(reason);
    return QDate();
}



///////////////////////////////////////////////////////////////////////////////
// Next working day for a particular person
QDate Calendar::NextWorkingDayForPerson(const QString & mcrName,
    const QDate mcDate) const
{
    CALL_IN(QString("mcrName=%1, mcData=%2")
        .arg(CALL_SHOW(mcrName),
             CALL_SHOW(mcDate)));

    // Check if person exists
    if (!m_PersonInfo.contains(mcrName))
    {
        const QString reason = tr("Person \"%1\" does not exist.")
            .arg(mcrName);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    // Check if date is valid
    if (!mcDate.isValid())
    {
        const QString reason = tr("Invalid date");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    QDate next_day = mcDate.addDays(1);
    int tries = 0;
    while (IsAway(mcrName, next_day) &&
           tries++ < 10000)
    {
        next_day = mcDate.addDays(1);
    }
    if (IsAway(mcrName, next_day))
    {
        const QString reason =
            tr("Could not find a day when \"%1\" wasn't away.")
                .arg(mcrName);;
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return QDate();
    }

    CALL_OUT("");
    return next_day;
}



// ====================================================================== Debug



///////////////////////////////////////////////////////////////////////////////
// Show everything
void Calendar::Dump() const
{
    CALL_IN("");

    qDebug().noquote() << tr("===== Countries and regions");
    QStringList countries(m_Countries.begin(), m_Countries.end());
    std::sort(countries.begin(), countries.end());
    for (const QString & country : countries)
    {
        QStringList regions(m_CountryToRegions[country].begin(),
            m_CountryToRegions[country].end());
        std::sort(regions.begin(), regions.end());
        qDebug().noquote() << QString("%1: %2")
            .arg(country,
                 regions.join(", "));
    }

    qDebug().noquote() << tr("===== Holidays");
    qDebug().noquote() << tr("=== Global holidays");
    for (int index = 0;
         index < m_GlobalHolidays.size();
         index++)
    {
        Dump_Holiday(m_GlobalHolidays[index]);
    }
    qDebug().noquote() << tr("=== Country-specific holidays");
    for (const QString & country : countries)
    {
        if (!m_CountryToHolidays.contains(country))
        {
            continue;
        }
        qDebug().noquote() << tr("= Holidays in %1")
            .arg(country);
        for (int index = 0;
             index < m_CountryToHolidays[country].size();
             index++)
        {
            Dump_Holiday(m_CountryToHolidays[country][index]);
        }

        QStringList regions(m_CountryRegionToHolidays[country].keyBegin(),
            m_CountryRegionToHolidays[country].keyEnd());
        std::sort(regions.begin(), regions.end());
        for (const QString & region : regions)
        {
            qDebug().noquote() << tr("Holidays in region %1")
                .arg(region);
            for (int index = 0;
                 index < m_CountryRegionToHolidays[country][region].size();
                 index++)
            {
                Dump_Holiday(
                    m_CountryRegionToHolidays[country][region][index]);
            }
        }
    }

    qDebug().noquote() << tr("===== People");
    QStringList all_persons(m_PersonInfo.keyBegin(), m_PersonInfo.keyEnd());
    std::sort(all_persons.begin(), all_persons.end());
    for (const QString & person : all_persons)
    {
        qDebug().noquote() << tr("=== %1")
            .arg(person);
        if (m_PersonInfo[person].contains("country"))
        {
            qDebug().noquote() << tr("Country: %1")
                .arg(m_PersonInfo[person]["country"]);
            if (m_PersonInfo[person].contains("region"))
            {
                qDebug().noquote() << tr("Region: %1")
                    .arg(m_PersonInfo[person]["region"]);
            }
        }

        if (!m_PersonToVacations.contains(person))
        {
            continue;
        }
        qDebug().noquote() << tr("= Planned vacations");
        for (int index = 0;
             index < m_PersonToVacations[person].size();
             index++)
        {
            const QPair < QDate, QDate > & range =
                m_PersonToVacations[person][index];
            qDebug().noquote() << tr("From %1 to %2")
                .arg(range.first.toString("dd MMM yyyy"),
                     range.second.toString("dd MMM yyyy"));
        }
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Show holiday
void Calendar::Dump_Holiday(
    const QHash < QString, QString > & mcrHolidayInfo) const
{
    CALL_IN(QString("mcrHolidayInfo=%1")
        .arg(CALL_SHOW(mcrHolidayInfo)));

    // !!!
    qDebug().noquote() << CALL_SHOW(mcrHolidayInfo);

    CALL_OUT("");
}
