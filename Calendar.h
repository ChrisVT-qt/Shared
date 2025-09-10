// Calendar.h
// Class definition

/** \file
  * \todo Add Doxygen information
  */

#ifndef CALENDAR_H
#define CALENDAR_H

// Qt includes
#include <QDate>
#include <QObject>
#include <QPair>
#include <QRegularExpression>
#include <QSet>



// Class definition
class Calendar
    : public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Constructor
    Calendar();

    // Initialize some holidays
    void Init_Holidays();

public:
    // Destructor
    virtual ~Calendar();

public:
    // Instanciator
    static Calendar * Instance();

private:
    // Instance
    static Calendar * m_Instance;



    // ============================================================== Calendars
public:
    // Add or delete country
    bool AddCountry(const QString & mcrCountry);
    bool DeleteCountry(const QString & mcrCountry);

    // Add or delete region
    bool AddRegion(const QString & mcrCountry, const QString & mcrRegion);
    bool DeleteRegion(const QString & mcrCountry, const QString & mcrRegion);

    // Get all available countries/regions
    QSet < QString > GetAvailableCountries() const;
    QSet < QString > GetAvailableRegions(const QString & mcrCountry) const;

private:
    QSet < QString > m_Countries;
    QHash < QString, QSet < QString > > m_CountryToRegions;



    // =============================================================== Holidays
public:
    // Add holiday
    bool AddHoliday(const QString & mcrName, const QString & mcrRule);
    bool AddHoliday(const QString & mcrCountry, const QString & mcrName,
        const QString & mcrRule);
    bool AddHoliday(const QString & mcrCountry, const QString & mcrRegion,
        const QString & mcrName, const QString & mcrRule);

    // Delete holiday
    bool DeleteHoliday(const QString & mcrName);
    bool DeleteHoliday(const QString & mcrCountry, const QString & mcrName);
    bool DeleteHoliday(const QString & mcrCountry, const QString & mcrRegion,
        const QString & mcrName);

    // Get holidays
    QStringList GetHolidays(const QDate mcDate) const;
    QStringList GetHolidays(const QString & mcrCountry,
        const QDate mcDate) const;
    QStringList GetHolidays(const QString & mcrCountry,
        const QString & mcrRegion, const QDate mcDate) const;

    // Check if a date is a holiday
    bool IsHoliday(const QDate mcDate) const;
    bool IsHoliday(const QString & mcrCountry, const QDate mcDate) const;
    bool IsHoliday(const QString & mcrCountry, const QString & mcrRegion,
        const QDate mcDate) const;

    // Check if a date is a work day
    bool IsWorkDay(const QDate mcDate) const;
    bool IsWorkDay(const QString & mcrCountry, const QDate mcDate) const;
    bool IsWorkDay(const QString & mcrCountry, const QString & mcrRegion,
        const QDate mcDate) const;

private:
    QList < QHash < QString, QString > > m_GlobalHolidays;
    QHash < QString, QList < QHash < QString, QString > > >
        m_CountryToHolidays;
    QHash < QString, QHash < QString, QList < QHash < QString, QString > > > >
        m_CountryRegionToHolidays;

    // Check if a particular date matches a rule
    bool DoesDateMatchRule(const QDate mcDate, const QString & mcrRule) const;

    // Regular expressions
    QRegularExpression m_FormatEvery;
    QRegularExpression m_FormatNth;
    QRegularExpression m_FormatDate;



    // =============================================================== Vacation
public:
    // Add/remove person
    bool AddPerson(const QString & mcrName, const QString & mcrCountry = "",
        const QString & mcrRegion = "");
    bool DeletePerson(const QString & mcrName);

    // Details
    QStringList GetAllPersons() const;
    QHash < QString, QString > GetPersonDetails(const QString & mcrName) const;

    // Add/remove vacation
    bool AddVacation(const QString & mcrName, const QDate mcFirstDay,
        const QDate mcLastDay = QDate());
    bool DeleteVacation(const QString & mcrName, const QDate mcFirstDay);

    // Check if a person is away
    bool HasVacation(const QString & mcrName, const QDate mcDate) const;
    bool HasHoliday(const QString & mcrName, const QDate mcDate) const;
    bool IsAway(const QString & mcrName, const QDate mcDate) const;

private:
    QHash < QString, QHash < QString, QString > > m_PersonInfo;
    QHash < QString, QList < QPair < QDate, QDate > > > m_PersonToVacations;



    // ======================================================== Date Management
public:
    // Next working day
    QDate NextWorkingDay(const QDate mcDate) const;
    QDate NextWorkingDay(const QString & mcrCountry, const QDate mcDate) const;
    QDate NextWorkingDay(const QString & mcrCountry, const QString & mcrRegion,
        const QDate mcDate) const;
    QDate NextWorkingDayForPerson(const QString & mcrName,
        const QDate mcDate) const;



    // ================================================================== Debug
public:
    // Show everything
    void Dump() const;

private:
    void Dump_Holiday(const QHash < QString, QString > & mcrHolidayInfo) const;
};

#endif
