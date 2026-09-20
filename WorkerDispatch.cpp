// WorkerDispatch.cpp
// Class implementation

// Project includes
#include "CallTracer.h"
#include "AbstractWorker.h"
#include "MessageLogger.h"
#include "WorkerDispatch.h"

// Qt includes
#include <QCoreApplication>
#include <QThread>



// ================================================================== Lifecycle



///////////////////////////////////////////////////////////////////////////////
// Constructor
WorkerDispatch::WorkerDispatch()
{
    CALL_IN("");
    REGISTER_INSTANCE;

    // Configure thread pool
    m_ThreadPool.setMaxThreadCount(QThread::idealThreadCount());

    // Initially not running
    m_IsRunning = false;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Instance
WorkerDispatch * WorkerDispatch::Instance()
{
    CALL_IN("");

    if (!m_Instance)
    {
        m_Instance = new WorkerDispatch();
    }

    CALL_OUT("");
    return m_Instance;
}



///////////////////////////////////////////////////////////////////////////////
// Actual instance
WorkerDispatch * WorkerDispatch::m_Instance = nullptr;



///////////////////////////////////////////////////////////////////////////////
// Destructor
WorkerDispatch::~WorkerDispatch()
{
    CALL_IN("");
    UNREGISTER_INSTANCE;

    // Delete pending workers
    for (AbstractWorker * worker : m_PendingWorkers)
    {
        delete worker;
    }

    CALL_OUT("");
}



// ====================================================== Thread Management



///////////////////////////////////////////////////////////////////////////////
// Number of concurrent threads
int WorkerDispatch::GetNumberOfConcurrentThreads() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_ThreadPool.maxThreadCount();
}



///////////////////////////////////////////////////////////////////////////////
// Set number of concurrent threads
void WorkerDispatch::SetNumberOfConcurrentThreads(const int mcNumber)
{
    CALL_IN("");

    if (mcNumber <= 0)
    {
        const QString reason =
            tr("Invalid maximum number of concurrent threads: %1")
                .arg(QString::number(mcNumber));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return;
    }

    m_ThreadPool.setMaxThreadCount(mcNumber);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Start work
void WorkerDispatch::Start()
{
    CALL_IN("");

    m_IsRunning = true;
    for (AbstractWorker * worker : std::as_const(m_PendingWorkers))
    {
        SubmitWorker(worker);
    }
    m_PendingWorkers.clear();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Start work and return only when it's finished
void WorkerDispatch::Start_WaitForFinish()
{
    CALL_IN("");

    // Catch the case if there's nothing to do at all
    if (m_PendingWorkers.isEmpty() &&
        m_SubmittedWorkers.isEmpty())
    {
        CALL_OUT("");
        return;
    }

    QEventLoop loop;
    connect (this, &WorkerDispatch::AllFinished,
        &loop, &QEventLoop::quit);
    Start();
    loop.exec();

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Stop work
void WorkerDispatch::Stop()
{
    CALL_IN("");

    // Note that Stop() will allow the currently running workers to finish
    // but it will prevent any new ones starting their work.
    // If you want to stop all work, call Terminate(GetActiveWorkerIDs()).
    m_IsRunning = false;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Add task to queue
bool WorkerDispatch::AddWorker(AbstractWorker * mpWorker)
{
    CALL_IN(QString("mpWorker=%1")
        .arg(CALL_SHOW(mpWorker)));

    // Check if this worker exists
    if (!mpWorker)
    {
        const QString reason = tr("No worker object provided.");
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if worker is already in the queue
    const int worker_id = mpWorker -> GetWorkerID();
    if (m_PendingWorkers.contains(mpWorker) ||
        m_SubmittedWorkers.contains(worker_id))
    {
        const QString reason = tr("Worker %1 is already queued or running.")
            .arg(QString::number(worker_id));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if worker is configured
    if (!mpWorker -> IsConfigured())
    {
        const QString reason = tr("Worker %1 is not yet configured.")
            .arg(QString::number(worker_id));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    if (m_IsRunning)
    {
        SubmitWorker(mpWorker);
    } else
    {
        m_PendingWorkers << mpWorker;
    }

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Add several workers at once
bool WorkerDispatch::AddWorkers(const QList < AbstractWorker * > & mcrWorkers)
{
    CALL_IN(QString("mcrWorkers=%1")
        .arg("..."));

    bool all_valid = true;
    QString reason_invalid;
    for (AbstractWorker * worker : mcrWorkers)
    {
        // Check if this worker exists
        if (!worker)
        {
            reason_invalid = tr("No worker object provided.");
            all_valid = false;
            break;
        }

        // Check if worker is already in the queue
        const int worker_id = worker -> GetWorkerID();
        if (m_PendingWorkers.contains(worker) ||
            m_SubmittedWorkers.contains(worker_id))
        {
            reason_invalid = tr("Worker %1 is already queued or running.")
                .arg(QString::number(worker_id));
            all_valid = false;
            break;
        }

        // Check if worker is configured
        if (!worker -> IsConfigured())
        {
            reason_invalid = tr("Worker %1 is not yet configured.")
                .arg(QString::number(worker_id));
            all_valid = false;
            break;
        }
    }

    // Handle failure
    if (!all_valid)
    {
        MessageLogger::Error(CALL_METHOD, reason_invalid);
        CALL_OUT(reason_invalid);
        return false;
    }

    // All good, add all workerd
    if (m_IsRunning)
    {
        for (AbstractWorker * worker : mcrWorkers)
        {
            SubmitWorker(worker);
        }
    } else
    {
        for (AbstractWorker * worker : mcrWorkers)
        {
            m_PendingWorkers << worker;
        }
    }

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
void WorkerDispatch::SubmitWorker(AbstractWorker * mpWorker)
{
    CALL_IN(QString("mpWorker=%1")
        .arg(CALL_SHOW(mpWorker)));

    const int worker_id = mpWorker -> GetWorkerID();
    m_SubmittedWorkers[worker_id] = mpWorker;

    connect (mpWorker, &AbstractWorker::Finished,
        this, [ = ](const bool success)
        {
            WorkerFinished(worker_id, success);
        });

    m_ThreadPool.start(mpWorker);

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Active workers
QSet < int > WorkerDispatch::GetActiveWorkerIDs() const
{
    CALL_IN("");

    const QSet < int > active_ids(m_SubmittedWorkers.keyBegin(),
        m_SubmittedWorkers.keyEnd());

    CALL_OUT("");
    return active_ids;
}



///////////////////////////////////////////////////////////////////////////////
// Remove workers
bool WorkerDispatch::Terminate(const QSet < int > & mcrWorkerIDs)
{
    CALL_IN(QString("mcrWorkerIDs=%1")
        .arg(CALL_SHOW(mcrWorkerIDs)));

    bool all_removed_before_starting = true;

    // Workers that haven't even been submitted yet (Start() not called)
    int index = 0;
    while (index < m_PendingWorkers.size())
    {
        const int worker_id = m_PendingWorkers[index] -> GetWorkerID();
        if (mcrWorkerIDs.contains(worker_id))
        {
            delete m_PendingWorkers[index];
            m_PendingWorkers.removeAt(index);
        } else
        {
            index++;
        }
    }

    for (const int id : mcrWorkerIDs)
    {
        if (!m_SubmittedWorkers.contains(id))
        {
            continue;
        }

        AbstractWorker * worker = m_SubmittedWorkers[id];

        if (m_ThreadPool.tryTake(worker))
        {
            // Was still waiting in the pool's internal queue - run() was
            // never called, safe to drop immediately.
            disconnect(worker, &AbstractWorker::Finished, this, nullptr);
            m_SubmittedWorkers.remove(id);
            delete worker;
        } else
        {
            // Already running (or, racily, already finished) - can only
            // ask it to stop cooperatively. WorkerFinished() does the
            // real cleanup once StartWork() actually returns.
            worker -> Cancel();
            all_removed_before_starting = false;
        }
    }

    CALL_OUT("");
    return all_removed_before_starting;
}



///////////////////////////////////////////////////////////////////////////////
// Worker finished
void WorkerDispatch::WorkerFinished(const int mcWorkerID,
    const bool mcWasSuccessful)
{
    CALL_IN(QString("mcWorkerID=%1, mcWasSuccessful=%2")
        .arg(CALL_SHOW(mcWorkerID),
             CALL_SHOW(mcWasSuccessful)));

    m_SubmittedWorkers.remove(mcWorkerID);

    emit Finished(mcWorkerID, mcWasSuccessful);

    if (m_PendingWorkers.isEmpty() &&
        m_SubmittedWorkers.isEmpty())
    {
        emit AllFinished();
    }

    CALL_OUT("");
}
