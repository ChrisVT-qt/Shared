// AbstractWorker.cpp
// Class implementation

// Project includes
#include "AbstractWorker.h"

// Qt includes
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>


#define DEBUG_WORKER false



// We don't do call tracing here because our way of doing that is not thread
// safe.



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
AbstractWorker::AbstractWorker()
{
    // Remember worker ID
    m_WorkerID = m_NextWorkerID++;

    // Not configured to work
    m_IsConfigured = false;

    // Currently is idle
    m_IsIdle = true;

    // Not canceled
    m_IsCanceled = false;

    if (DEBUG_WORKER)
    {
        const QString now =
            QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss.zzz");
        qDebug() << tr("[Worker %1] Created on %2")
            .arg(QString::number(m_WorkerID),
                 now);
    }
}



///////////////////////////////////////////////////////////////////////////////
// Destructor
AbstractWorker::~AbstractWorker()
{
    if (DEBUG_WORKER)
    {
        const QString now =
            QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss.zzz");
        qDebug() << tr("[Worker %1] Destroyed on %2")
            .arg(QString::number(m_WorkerID),
                 now);
    }

    // Nothing else to do.
}



///////////////////////////////////////////////////////////////////////////////
// Global worker ID count
int AbstractWorker::m_NextWorkerID = 0;



// ============================================================ Everything else



///////////////////////////////////////////////////////////////////////////////
// Worker ID
int AbstractWorker::GetWorkerID() const
{
    return m_WorkerID;
}



///////////////////////////////////////////////////////////////////////////////
// Check if worker has been configured
bool AbstractWorker::IsConfigured() const
{
    return m_IsConfigured;
}



///////////////////////////////////////////////////////////////////////////////
// Check if worker is idle
bool AbstractWorker::IsIdle() const
{
    return m_IsIdle;
}



///////////////////////////////////////////////////////////////////////////////
// Cancel work
void AbstractWorker::Cancel()
{
    m_IsCanceled = true;
}
