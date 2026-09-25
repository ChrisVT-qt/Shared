// AbstractWorker.cpp
// Class implementation

// Project includes
#include "AbstractWorker.h"
#include "CallTracer.h"
#include "MessageLogger.h"

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

    setAutoDelete(false);
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
QAtomicInteger < int > AbstractWorker::m_NextWorkerID = 0;



// ============================================================ Everything else



///////////////////////////////////////////////////////////////////////////////
// Worker ID
int AbstractWorker::GetWorkerID() const
{
    return m_WorkerID;
}



///////////////////////////////////////////////////////////////////////////////
// bool DerivedClass::Configure(any parameters)
// {
//      - Needs to set all data and parameters required for the worker to run.
//      - Set m_IsConfigured to "true" upon success
// }



///////////////////////////////////////////////////////////////////////////////
// Check if worker has been configured
bool AbstractWorker::IsConfigured() const
{
    return m_IsConfigured;
}



///////////////////////////////////////////////////////////////////////////////
// Cancel work
void AbstractWorker::Cancel()
{
    m_IsCanceled = true;
}



///////////////////////////////////////////////////////////////////////////////
// Start work
void AbstractWorker::run()
{
    // Needs to be implemented by every derived class

    // Check if we're configured
    if (!m_IsConfigured)
    {
        const QString reason = tr("[Worker %1] No work configured.")
            .arg(QString::number(m_WorkerID));
        MessageLogger::Error(CALL_METHOD, reason);
        emit Finished(false);
        return;
    }

    // Not idle anymore, and not canceled
    m_IsCanceled = false;

    if (DEBUG_WORKER)
    {
        const QString now =
            QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss.zzz");
        qDebug() << tr("[Worker %1] Started work on %2")
            .arg(QString::number(m_WorkerID),
                 now);
    }

    QElapsedTimer timer;
    timer.start();

    const bool success = DoWork();

    if (DEBUG_WORKER)
    {
        const QString now =
            QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm:ss.zzz");
        qDebug() << tr("[Worker %1] %2 work on %3 (%4s)")
            .arg(QString::number(m_WorkerID),
                 success ? tr("Completed") : tr("Aborted"),
                 now,
                 QString::number(timer.elapsed() * 0.001));
    }

    // Let outside world know we're done for now.
    emit Finished(success);
}



#if 0
///////////////////////////////////////////////////////////////////////////////
// Do work
bool DerivedClass::DoWork() const
{
    // Implementers: on success, before returning true, store your result
    // into its permanent home yourself - the worker object is deleted
    // immediately after this function returns, so nothing survives beyond
    // DoWork(). Do this only on the success path; do not store partial results
    // on failure or cancellation. Storage must be thread-safe, since DoWork()
    // runs on a pool thread and multiple workers may store concurrently.

    Loop
    {
        // !!! Some work item

        // Check for cancelation
        if (m_IsCanceled)
        {
            return false;
        }
    }

    // If successful, store result

    // Object will be deleted after leaving this method
}
#endif


