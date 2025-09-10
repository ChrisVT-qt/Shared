// PickDateDialog.h
// Class definition

/** \file
  * \todo Add Doxygen information
  */

#ifndef PICKDATEDIALOG_H
#define PICKDATEDIALOG_H

// Qt includes
#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

// Forward declaration
class ClickableWidget;

// Class definition
class PickDateDialog
    : public QDialog
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Constructor
    PickDateDialog();

public:
    // Pick a date
    static PickDateDialog * NewPickSingleDate(
        const QDate mcAnchorDate = QDate());

    // Edit date range
    static PickDateDialog * NewPickDateRange(const QDate mcStartDate = QDate(),
        const QDate mcEndDate = QDate());

    // Destructor
    virtual ~PickDateDialog();

private:
    QDate m_SelectedDate;

    QDate m_RangeStartDate;
    QDate m_RangeEndDate;

    bool m_IsPickingRange;

    QHash < int, QString > m_MonthToShort;
    QHash < int, QString > m_WeekdayToShort;



    // ==================================================================== GUI
private:
    // Initialize Widgets
    void InitWidgets();

    // Month and date for the calendar
    int m_AnchorYear;
    int m_AnchorMonth;

    QLabel * m_CurrentMonthLabel;
    QLabel * m_CurrentYearLabel;
    QComboBox * m_CurrentMonthComboBox;
    QComboBox * m_CurrentYearComboBox;

    QList < QLabel * > m_Weekdays;

    QList < QLabel * > m_Weeks;
    QList < QDate > m_WeeksDates;
    QList < ClickableWidget * > m_WeeksContainer;

    QList < QLabel * > m_Days;
    QList < QDate > m_DaysDates;
    QList < ClickableWidget * > m_DaysContainer;

    QLabel * m_Details;

    void Refresh();

private slots:
    void PreviousMonth();
    void NextMonth();
    void YearSelected(const int mcIndex);
    void MonthSelected(const int mcIndex);
    void Today();
    void DaySingleClicked(const int mcIndex);
    void DayDoubleClicked(const int mcIndex);
    void WeekSingleClicked(const int mcIndex);
    void WeekDoubleClicked(const int mcIndex);

private:
    bool m_IsRangeSelectionStartSelected;

    // ================================================================= Access
public:
    // Set first day of the week
    void SetFirstDayOfWeek(const QString mcFirstDayOfWeek);
private:
    bool m_IsMondayFirstDayOfWeek;

public:
    // Show or hide week number
    void SetShowCalendarWeek(const bool mcNewState);
private:
    bool m_ShowCalendarWeek;

public:
    // Calendar information
    void SetCalendar(const QString & mcrCountry,
        const QString & mcrRegion = "");
private:
    QString m_Calendar_Country;
    QString m_Calendar_Region;

public:
    // Combobox for month, year
    void SetUseComboBoxForMonthAndYear(const bool mcNewState);
private:
    bool m_UseComboBoxForMonthAndYear;

public:
    // Show days outside of current month
    void SetShowDaysOutsideOfCurrentMonth(const bool mcNewState);
private:
    bool m_ShowDaysOutsideOfCurrentMonth;

public:
    // Show date details
    void SetShowDateDetails(const bool mcNewState);
private:
    bool m_ShowDateDetails;

public:
    // Get selected date
    QDate GetSelectedDate() const;

    // Get selected date range
    QPair < QDate, QDate > GetSelectedDateRange() const;
};

#endif
