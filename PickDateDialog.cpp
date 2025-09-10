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

// PickDate.cpp
// Class implementation

// Project includes
#include "Calendar.h"
#include "CallTracer.h"
#include "ClickableWidget.h"
#include "ColorHelper.h"
#include "MessageLogger.h"
#include "PickDateDialog.h"

// Qt includes
#include <QGridLayout>
#include <QVBoxLayout>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
PickDateDialog::PickDateDialog()
{
    CALL_IN("");

    // Initialize some things...
    m_SelectedDate = QDate();
    m_RangeStartDate = QDate();
    m_RangeEndDate = QDate();
    m_IsPickingRange = false;

    m_AnchorYear = QDate::currentDate().year();
    m_AnchorMonth = QDate::currentDate().month();

    m_IsMondayFirstDayOfWeek = true;
    m_ShowCalendarWeek = true;
    m_UseComboBoxForMonthAndYear = true;
    m_ShowDaysOutsideOfCurrentMonth = true;
    m_ShowDateDetails = true;

    m_MonthToShort[1] = tr("Jan");
    m_MonthToShort[2] = tr("Feb");
    m_MonthToShort[3] = tr("Mar");
    m_MonthToShort[4] = tr("Apr");
    m_MonthToShort[5] = tr("May");
    m_MonthToShort[6] = tr("Jun");
    m_MonthToShort[7] = tr("Jul");
    m_MonthToShort[8] = tr("Aug");
    m_MonthToShort[9] = tr("Sep");
    m_MonthToShort[10] = tr("Oct");
    m_MonthToShort[11] = tr("Nov");
    m_MonthToShort[12] = tr("Dec");

    m_WeekdayToShort[0] = tr("Mon");
    m_WeekdayToShort[1] = tr("Tue");
    m_WeekdayToShort[2] = tr("Wed");
    m_WeekdayToShort[3] = tr("Thu");
    m_WeekdayToShort[4] = tr("Fri");
    m_WeekdayToShort[5] = tr("Sat");
    m_WeekdayToShort[6] = tr("Sun");

    m_IsRangeSelectionStartSelected = false;

    // GUI
    InitWidgets();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Pick a date
PickDateDialog * PickDateDialog::NewPickSingleDate(const QDate mcAnchorDate)
{
    CALL_IN(QString("mcAnchorDate=%1")
        .arg(CALL_SHOW(mcAnchorDate)));

    // New instance
    PickDateDialog * ret = new PickDateDialog();

    // Set anchor date
    if (mcAnchorDate.isValid())
    {
        // Given date
        ret -> m_AnchorYear = mcAnchorDate.year();
        ret -> m_AnchorMonth = mcAnchorDate.month();
        ret -> m_SelectedDate = mcAnchorDate;
    } else
    {
        // Has been initialized as "today" already in the constructor
    }

    ret -> m_IsPickingRange = false;

    ret -> Refresh();

    ret -> setFixedSize(220, 290);

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Edit date range
PickDateDialog * PickDateDialog::NewPickDateRange(const QDate mcStartDate,
    const QDate mcEndDate)
{
    CALL_IN(QString("mcStartDate=%1, mcEndDate=%2")
        .arg(CALL_SHOW(mcStartDate),
             CALL_SHOW(mcEndDate)));

    // New instance
    PickDateDialog * ret = new PickDateDialog();

    // Set anchor date
    if (mcStartDate.isValid())
    {
        // Given date
        ret -> m_AnchorYear = mcStartDate.year();
        ret -> m_AnchorMonth = mcStartDate.month();
    } else
    {
        // Has been initialized as "today" already in the constructor
    }

    // Set range start and end
    ret -> m_RangeStartDate = mcStartDate;
    ret -> m_RangeEndDate = mcEndDate;
    ret -> m_IsPickingRange = true;

    ret -> Refresh();

    ret -> setFixedSize(220, 290);

    CALL_OUT("");
    return ret;
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
PickDateDialog::~PickDateDialog()
{
    CALL_IN("");

    // Nothing to do

    CALL_OUT("");
}



// ======================================================================== GUI



///////////////////////////////////////////////////////////////////////////////
// Initialize Widgets
void PickDateDialog::InitWidgets()
{
    CALL_IN("");

    setAttribute(Qt::WA_MacSmallSize);

    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    // == Top
    QHBoxLayout * top_layout = new QHBoxLayout();
    top_layout -> setSpacing(2);

    QPushButton * pb_previous = new QPushButton(QString("<"));
    pb_previous -> setAttribute(Qt::WA_MacSmallSize);
    pb_previous -> setFixedWidth(20);
    connect (pb_previous, SIGNAL(clicked()),
        this, SLOT(PreviousMonth()));
    top_layout -> addWidget(pb_previous);

    top_layout -> addStretch(1);

    m_CurrentMonthLabel = new QLabel();
    top_layout -> addWidget(m_CurrentMonthLabel);
    m_CurrentYearLabel = new QLabel();
    top_layout -> addWidget(m_CurrentYearLabel);
    m_CurrentMonthComboBox = new QComboBox();
    for (int month = 1;
         month <= 12;
         month++)
    {
        m_CurrentMonthComboBox  -> addItem(m_MonthToShort[month], month);
    }
    connect (m_CurrentMonthComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(MonthSelected(int)));
    top_layout -> addWidget(m_CurrentMonthComboBox);
    m_CurrentYearComboBox = new QComboBox();
    connect (m_CurrentYearComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(YearSelected(int)));
    top_layout -> addWidget(m_CurrentYearComboBox);

    top_layout -> addStretch(1);

    QPushButton * pb_next = new QPushButton(QString(">"));
    pb_next -> setAttribute(Qt::WA_MacSmallSize);
    pb_next -> setFixedWidth(20);
    connect (pb_next, SIGNAL(clicked()),
        this, SLOT(NextMonth()));
    top_layout -> addWidget(pb_next);

    layout -> addLayout(top_layout);

    // == Calendar
    QHBoxLayout * calendar_container_layout = new QHBoxLayout();
    calendar_container_layout -> addStretch(1);

    // Fonts
    QFont small_font;
    small_font.setPointSize(small_font.pointSize() * 0.8);
    QFont large_font;
    large_font.setPointSize(small_font.pointSize() * 1.4);

    QGridLayout * calendar_layout = new QGridLayout();
    calendar_layout -> setSpacing(1);
    int row = 0;

    // Weekdays
    for (int weekday = 0;
         weekday < 7;
         weekday++)
    {
        QLabel * weekday_text = new QLabel();
        weekday_text -> setAlignment(Qt::AlignCenter);
        weekday_text -> setFont(small_font);
        m_Weekdays << weekday_text;
        calendar_layout -> addWidget(weekday_text, row, weekday+1);
    }
    row++;

    // Weeks & days
    QPalette week_palette;
    week_palette.setColor(QPalette::WindowText, QColor(180, 180, 180));

    int index = 0;
    for (int week = 0;
         week < 6;
         week++)
    {
        // Calendar week
        ClickableWidget * clickable = new ClickableWidget();
        clickable -> setFixedSize(24,24);
        clickable -> setAutoFillBackground(true);
        clickable -> setContentsMargins(5,5,5,5);

        connect(clickable, &ClickableWidget::SingleClicked, this,
            [=](){WeekSingleClicked(week);});
        connect(clickable, &ClickableWidget::DoubleClicked, this,
            [=](){WeekDoubleClicked(week);});

        QVBoxLayout * clickable_layout = new QVBoxLayout();
        clickable_layout -> setSpacing(0);
        clickable_layout -> setContentsMargins(0,0,0,0);
        clickable -> setLayout(clickable_layout);

        QLabel * week_text = new QLabel();
        week_text -> setAlignment(Qt::AlignCenter);
        week_text -> setFont(small_font);
        week_text -> setPalette(week_palette);
        clickable_layout -> addWidget(week_text);

        m_WeeksContainer << clickable;

        m_Weeks << week_text;
        calendar_layout -> addWidget(clickable, row, 0);

        m_WeeksDates << QDate();

        // Days of the week
        for (int weekday = 0;
             weekday < 7;
             weekday++)
        {
            ClickableWidget * clickable = new ClickableWidget();
            clickable -> setFixedSize(24,24);
            clickable -> setAutoFillBackground(true);
            clickable -> setContentsMargins(2,2,2,2);

            connect(clickable, &ClickableWidget::SingleClicked, this,
                [=](){DaySingleClicked(index);});
            connect(clickable, &ClickableWidget::DoubleClicked, this,
                [=](){DayDoubleClicked(index);});

            QVBoxLayout * clickable_layout = new QVBoxLayout();
            clickable_layout -> setSpacing(0);
            clickable_layout -> setContentsMargins(0,0,0,0);
            clickable -> setLayout(clickable_layout);

            QLabel * day_text = new QLabel();
            day_text -> setFont(large_font);
            day_text -> setAlignment(Qt::AlignCenter);
            clickable_layout -> addWidget(day_text);

            m_DaysContainer << clickable;

            m_Days << day_text;
            calendar_layout -> addWidget(clickable, row, weekday+1);

            m_DaysDates << QDate();
            index++;
        }

        row++;
    }

    calendar_container_layout -> addLayout(calendar_layout);

    calendar_container_layout -> addStretch(1);
    layout -> addLayout(calendar_container_layout);

    // == Details
    QHBoxLayout * detail_layout = new QHBoxLayout();

    detail_layout -> addStretch(1);

    m_Details = new QLabel();
    m_Details -> setFont(small_font);
    m_Details -> setAlignment(Qt::AlignCenter);
    detail_layout -> addWidget(m_Details);

    detail_layout -> addStretch(1);

    layout -> addLayout(detail_layout);

    // == Buttons
    QHBoxLayout * button_layout = new QHBoxLayout();

    button_layout -> addStretch(1);

    QPushButton * pb_today = new QPushButton(tr("Today"));
    pb_today -> setFont(small_font);
    pb_today -> setFixedWidth(60);
    connect (pb_today, SIGNAL(clicked(bool)),
        this, SLOT(Today()));
    button_layout -> addWidget(pb_today);

    QPushButton * pb_cancel = new QPushButton(tr("Cancel"));
    pb_cancel -> setFont(small_font);
    pb_cancel -> setFixedWidth(60);
    connect (pb_cancel, SIGNAL(clicked(bool)),
        this, SLOT(reject()));
    button_layout -> addWidget(pb_cancel);

    QPushButton * pb_ok = new QPushButton(tr("Ok"));
    pb_ok -> setFont(small_font);
    pb_ok -> setFixedWidth(60);
    connect (pb_ok, SIGNAL(clicked(bool)),
        this, SLOT(accept()));
    button_layout -> addWidget(pb_ok);

    layout -> addLayout(button_layout);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Refresh
void PickDateDialog::Refresh()
{
    CALL_IN("");

    // We'll need this
    Calendar * cal = Calendar::Instance();

    // == Top row
    m_CurrentMonthLabel -> setText(m_MonthToShort[m_AnchorMonth]);
    m_CurrentYearLabel -> setText(QString::number(m_AnchorYear));

    int index = m_CurrentMonthComboBox -> findData(m_AnchorMonth);
    m_CurrentMonthComboBox -> blockSignals(true);
    m_CurrentMonthComboBox -> setCurrentIndex(index);
    m_CurrentMonthComboBox -> blockSignals(false);

    m_CurrentYearComboBox -> blockSignals(true);
    m_CurrentYearComboBox -> clear();
    for (int year = m_AnchorYear - 5;
         year <= m_AnchorYear + 5;
         year++)
    {
        m_CurrentYearComboBox -> addItem(QString::number(year), year);
    }
    index = m_CurrentYearComboBox -> findData(m_AnchorYear);
    m_CurrentYearComboBox -> setCurrentIndex(index);
    m_CurrentYearComboBox -> blockSignals(false);

    if (m_UseComboBoxForMonthAndYear)
    {
        m_CurrentMonthLabel -> hide();
        m_CurrentYearLabel -> hide();
        m_CurrentMonthComboBox -> show();
        m_CurrentYearComboBox -> show();
    } else
    {
        m_CurrentMonthLabel -> show();
        m_CurrentYearLabel -> show();
        m_CurrentMonthComboBox -> hide();
        m_CurrentYearComboBox -> hide();
    }

    // == Calendar

    // Weekdays
    int day_offset = 0;
    if (!m_IsMondayFirstDayOfWeek)
    {
        day_offset = 6;
    }
    for (int weekday = 0;
         weekday < 7;
         weekday++)
    {
        m_Weekdays[weekday] -> setText(
            m_WeekdayToShort[(weekday + day_offset) % 7]);
    }

    // Weeks and days
    QColor weekday_bg = QColor(220,220,220);
    QColor weekend_bg = QColor(180,180,180);
    QColor standard = QColor(0,0,0);
    QColor selected_day = QColor(0,0,255);
    QColor not_this_month = QColor(120,120,120);
    QColor today = QColor(128,128,255);

    QDate date(m_AnchorYear, m_AnchorMonth, 1);
    if (m_IsMondayFirstDayOfWeek)
    {
        date = date.addDays(-date.dayOfWeek() + 1);
    } else
    {
        date = date.addDays(7-date.dayOfWeek());
    }
    int day_index = 0;
    bool show_labels = true;
    for (int week = 0;
         week < 6;
         week++)
    {
        m_Weeks[week] -> setText(QString::number(date.weekNumber()));
        m_Weeks[week] -> setVisible(show_labels && m_ShowCalendarWeek);
        m_WeeksDates[week] = date;
        for (int day = 0;
             day < 7;
             day++)
        {
            QPalette container_palette;
            const QString day_twodigits = "0" + QString::number(date.day());
            m_Days[day_index] -> setText(day_twodigits.right(2));
            m_DaysDates[day_index] = date;
            if (!m_ShowDaysOutsideOfCurrentMonth &&
                date.month() != m_AnchorMonth)
            {
                m_DaysContainer[day_index] -> setVisible(false);
            } else
            {
                m_DaysContainer[day_index] -> setVisible(show_labels);
            }
            bool is_workday;
            if (m_Calendar_Country.isEmpty())
            {
                is_workday = cal -> IsWorkDay(date);
            } else
            {
                if (m_Calendar_Region.isEmpty())
                {
                    is_workday = cal -> IsWorkDay(m_Calendar_Country, date);
                } else
                {
                    is_workday = cal -> IsWorkDay(m_Calendar_Country,
                        m_Calendar_Region, date);
                }
            }
            if (is_workday)
            {
                // Weekday
                container_palette.setColor(QPalette::Window, weekday_bg);
            } else
            {
                // Weekend
                container_palette.setColor(QPalette::Window, weekend_bg);
            }
            if (date == QDate::currentDate())
            {
                // Mix colors
                container_palette.setColor(QPalette::Window,
                    ColorHelper::Blend(
                        container_palette.color(QPalette::Window),
                        today, 0.3));
            }
            m_DaysContainer[day_index] -> setPalette(container_palette);

            QPalette content_palette;
            content_palette.setColor(QPalette::WindowText, standard);
            if (date.month() != m_AnchorMonth)
            {
                // Mix colors
                const QColor bg = container_palette.color(QPalette::Window);
                content_palette.setColor(QPalette::WindowText,
                    ColorHelper::Blend(bg, not_this_month, 0.5));
            } else
            {
                if (m_IsPickingRange)
                {
                    if ((date >= m_RangeStartDate &&
                        date <= m_RangeEndDate) ||
                        date == m_RangeStartDate)
                    {
                        content_palette.setColor(QPalette::WindowText,
                            selected_day);
                    }
                } else
                {
                    if (date == m_SelectedDate)
                    {
                        content_palette.setColor(QPalette::WindowText,
                            selected_day);
                    }
                }
            }
            m_Days[day_index] -> setPalette(content_palette);

            date = date.addDays(1);
            day_index++;
        }
        if (date.month() != m_AnchorMonth)
        {
            show_labels = false;
        }
    }

    // == Details
    if (m_IsPickingRange)
    {
        if (!m_RangeStartDate.isValid())
        {
            m_Details -> setText(tr("No range selected."));
        } else
        {
            QString text = tr("Range from\n");
            if (m_RangeStartDate.year() == m_RangeEndDate.year())
            {
                if (m_RangeStartDate.month() == m_RangeEndDate.month())
                {
                    if (m_RangeStartDate.day() == m_RangeEndDate.day())
                    {
                        // 17 Mar 2022 (single day)
                        text += m_RangeStartDate.toString("dd MMM yyyy");
                    } else
                    {
                        // Mar 17-25, 2022
                        text += m_RangeStartDate.toString("MMM dd-") +
                            m_RangeEndDate.toString("dd, yyyy");
                    }
                } else
                {
                    // Mar 12-Apr 15, 2022
                    text += m_RangeStartDate.toString("MMM dd-") +
                        m_RangeEndDate.toString("MMM dd, yyyy");
                }
            } else if (m_RangeEndDate.isValid())
            {
                // 12 Mar 2022 to 05 Jan 2023
                text += tr("%1 to %2")
                    .arg(m_RangeStartDate.toString("dd MMM yyyy"),
                         m_RangeEndDate.toString("dd MMM yyyy"));
            } else
            {
                // 12 Mar 2022 to ..."
                text += tr("%1 to ...")
                    .arg(m_RangeStartDate.toString("dd MMM yyyy"));
            }
            m_Details -> setText(text);
        }
    } else
    {
        // Picking a single day
        m_Details -> setText(m_SelectedDate.toString("dddd, dd MMM yyyy"));
        m_Details -> setVisible(m_ShowDateDetails);
    }

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Go to previous month
void PickDateDialog::PreviousMonth()
{
    CALL_IN("");

    QDate current_date(m_AnchorYear, m_AnchorMonth, 1);
    current_date = current_date.addMonths(-1);
    m_AnchorYear = current_date.year();
    m_AnchorMonth = current_date.month();

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Go to next month
void PickDateDialog::NextMonth()
{
    CALL_IN("");

    QDate current_date(m_AnchorYear, m_AnchorMonth, 1);
    current_date = current_date.addMonths(1);
    m_AnchorYear = current_date.year();
    m_AnchorMonth = current_date.month();

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Select year
void PickDateDialog::YearSelected(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    m_AnchorYear = m_CurrentYearComboBox -> itemData(mcIndex).toInt();

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Select month
void PickDateDialog::MonthSelected(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    m_AnchorMonth = m_CurrentMonthComboBox -> itemData(mcIndex).toInt();

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Go to today
void PickDateDialog::Today()
{
    CALL_IN("");

    m_SelectedDate = QDate::currentDate();
    m_AnchorYear = m_SelectedDate.year();
    m_AnchorMonth = m_SelectedDate.month();

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Day single clicked
void PickDateDialog::DaySingleClicked(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // For single dates...
    m_SelectedDate = m_DaysDates[mcIndex];

    // For ranges
    if (m_IsPickingRange)
    {
        if (!m_IsRangeSelectionStartSelected)
        {
            // Select start
            m_RangeStartDate = m_DaysDates[mcIndex];
            m_RangeEndDate = QDate();
            m_IsRangeSelectionStartSelected = true;
        } else
        {
            // Select end
            m_RangeEndDate = m_DaysDates[mcIndex];
            m_IsRangeSelectionStartSelected = false;

            // Make sure start is before end
            if (m_RangeStartDate > m_RangeEndDate)
            {
                qSwap(m_RangeStartDate, m_RangeEndDate);
            }
        }
    }

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Day double clicked
void PickDateDialog::DayDoubleClicked(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // For single dates...
    m_SelectedDate = m_DaysDates[mcIndex];

    // For date range...
    m_RangeStartDate = m_DaysDates[mcIndex];
    m_RangeEndDate = m_DaysDates[mcIndex];

    // Done here.
    CALL_OUT("");
    accept();
}



///////////////////////////////////////////////////////////////////////////////
// Week single clicked
void PickDateDialog::WeekSingleClicked(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    qDebug() << m_WeeksDates[mcIndex];

    // Doesn't do anything for single date selection
    if (!m_IsPickingRange)
    {
        CALL_OUT("");
        return;
    }

    if (!m_IsRangeSelectionStartSelected)
    {
        // Select start
        m_RangeStartDate = m_WeeksDates[mcIndex];
        m_RangeEndDate = m_RangeStartDate.addDays(6);
        m_IsRangeSelectionStartSelected = true;
    } else
    {
        // Select end
        if (m_WeeksDates[mcIndex] < m_RangeStartDate)
        {
            m_RangeStartDate = m_RangeStartDate.addDays(6);
            m_RangeEndDate = m_WeeksDates[mcIndex];
        } else
        {
            m_RangeEndDate = m_WeeksDates[mcIndex].addDays(6);
        }
        m_IsRangeSelectionStartSelected = false;

        // Make sure start is before end
        if (m_RangeStartDate > m_RangeEndDate)
        {
            qSwap(m_RangeStartDate, m_RangeEndDate);
        }
    }

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Week double clicked
void PickDateDialog::WeekDoubleClicked(const int mcIndex)
{
    CALL_IN(QString("mcIndex=%1")
        .arg(CALL_SHOW(mcIndex)));

    // Doesn't do anything for single date selection
    if (!m_IsPickingRange)
    {
        CALL_OUT("");
        return;
    }

    // For date range...
    m_RangeStartDate = m_WeeksDates[mcIndex];
    m_RangeEndDate = m_RangeStartDate.addDays(6);

    // Done here.
    CALL_OUT("");
    accept();
}



// ===================================================================== Access



///////////////////////////////////////////////////////////////////////////////
// Set first day of the week
void PickDateDialog::SetFirstDayOfWeek(const QString mcFirstDayOfWeek)
{
    CALL_IN(QString("mcFirstDayOfWeek=%1")
        .arg(CALL_SHOW(mcFirstDayOfWeek)));

    // Check if day is valid
    if (mcFirstDayOfWeek != "Monday" &&
        mcFirstDayOfWeek != "Sunday")
    {
        const QString reason =
            tr("Invalid day \"%1\" provided as first day of the week.")
                .arg(mcFirstDayOfWeek);
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    m_IsMondayFirstDayOfWeek = (mcFirstDayOfWeek == "Monday");

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Show or hide week number
void PickDateDialog::SetShowCalendarWeek(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    m_ShowCalendarWeek = mcNewState;

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Calendar information
void PickDateDialog::SetCalendar(const QString & mcrCountry,
    const QString & mcrRegion)
{
    CALL_IN(QString("mcrCountry=%1, mcrReagion=%2")
        .arg(CALL_SHOW(mcrCountry),
             CALL_SHOW(mcrRegion)));

    // Remember country and region
    m_Calendar_Country = mcrCountry;
    m_Calendar_Region = mcrRegion;

    CALL_OUT("");
    return;
}



///////////////////////////////////////////////////////////////////////////////
// Combobox for month, year
void PickDateDialog::SetUseComboBoxForMonthAndYear(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    m_UseComboBoxForMonthAndYear = mcNewState;

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Show days outside of current month
void PickDateDialog::SetShowDaysOutsideOfCurrentMonth(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    m_ShowDaysOutsideOfCurrentMonth = mcNewState;

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Show date details
void PickDateDialog::SetShowDateDetails(const bool mcNewState)
{
    CALL_IN(QString("mcNewState=%1")
        .arg(CALL_SHOW(mcNewState)));

    m_ShowDateDetails = mcNewState;

    Refresh();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Get selected date
QDate PickDateDialog::GetSelectedDate() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_SelectedDate;
}



///////////////////////////////////////////////////////////////////////////////
// Get selected date range
QPair < QDate, QDate > PickDateDialog::GetSelectedDateRange() const
{
    CALL_IN("");

    CALL_OUT("");
    return QPair < QDate, QDate >(m_RangeStartDate, m_RangeEndDate);
}
