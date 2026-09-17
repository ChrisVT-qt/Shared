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

    // Not running, not stopping
    m_NumberOfConcurrentThreads = QThread::idealThreadCount();

    // Queue empty signal hasn't been sent yet
    m_AllWorkersFinishedSignalSent = false;

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

    // Nothing to do

    CALL_OUT("");
}



// ====================================================== Thread Management



///////////////////////////////////////////////////////////////////////////////
// Number of concurrent threads
int WorkerDispatch::GetNumberOfConcurrentThreads() const
{
    CALL_IN("");

    CALL_OUT("");
    return m_NumberOfConcurrentThreads;
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

    m_NumberOfConcurrentThreads = mcNumber;

    CALL_OUT("");
}



///////////////////////////////////////////////////////////////////////////////
// Start work
void WorkerDispatch::Start()
{
    CALL_IN("");

    m_IsRunning = true;
    StartNextWorker();

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
void WorkerDispatch::StartNextWorker()
{
    CALL_IN("");

    // Check if we're actually running
    if (!m_IsRunning)
    {
        // Nope.
        CALL_OUT("");
        return;
    }

    // Check if we should kick off more work
    const int active_workers = m_ActiveWorkers.size();
    if (active_workers >= m_NumberOfConcurrentThreads)
    {
        // Nope.
        CALL_OUT("");
        return;
    }

    // Check if there is more work to do
    if (m_Queue.isEmpty())
    {
        // No.
        CALL_OUT("");
        return;
    }

    // Kick off one more worker
    AbstractWorker * worker = m_Queue.takeFirst();
    const int worker_id = worker -> GetWorkerID();
    m_ActiveWorkers[worker_id] = worker;
    QThread * thread = new QThread();
    m_WorkerIDToThread[worker_id] = thread;
    connect (thread, &QThread::started,
        worker, &AbstractWorker::StartWork);
    connect (worker, &AbstractWorker::Finished,
        this, [=]()
        {
            WorkerFinished(worker_id);
        });
    worker -> moveToThread(thread);
    thread -> start();

    // Potentially start another thread
    StartNextWorker();

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
    if (m_Queue.contains(mpWorker))
    {
        const QString reason = tr("Worker %1 is already in the queue.")
            .arg(QString::number(worker_id));
        MessageLogger::Error(CALL_METHOD, reason);
        CALL_OUT(reason);
        return false;
    }

    // Check if worker is already being worked on
    if (m_WorkerIDToThread.contains(worker_id))
    {
        const QString reason = tr("Worker %1 is already performing work.")
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

    // Add worker
    m_Queue << mpWorker;
    m_AllWorkersFinishedSignalSent = false;

    // Start worker=
    StartNextWorker();

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Add several workers at once
bool WorkerDispatch::AddWorkers(const QList < AbstractWorker * > & mcrWorkers)
{
    CALL_IN(QString("mcrWorkers=%1")
        .arg("..."));

    for (AbstractWorker * worker : mcrWorkers)
    {
        // Check if this worker exists
        if (!worker)
        {
            const QString reason = tr("No worker object provided.");
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }

        // Check if worker is already in the queue
        const int worker_id = worker -> GetWorkerID();
        if (m_Queue.contains(worker))
        {
            const QString reason = tr("Worker %1 is already in the queue.")
                .arg(QString::number(worker_id));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }

        // Check if worker is already being worked on
        if (m_WorkerIDToThread.contains(worker_id))
        {
            const QString reason = tr("Worker %1 is already performing work.")
                .arg(QString::number(worker_id));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }

        // Check if worker is configured
        if (!worker -> IsConfigured())
        {
            const QString reason = tr("Worker %1 is not yet configured.")
                .arg(QString::number(worker_id));
            MessageLogger::Error(CALL_METHOD, reason);
            CALL_OUT(reason);
            return false;
        }
    }

    // Add worker
    m_Queue << mcrWorkers;
    m_AllWorkersFinishedSignalSent = false;

    // Start worker
    StartNextWorker();

    CALL_OUT("");
    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Active workers
QSet < int > WorkerDispatch::GetActiveWorkerIDs() const
{
    CALL_IN("");

    const QSet < int > active_ids(m_ActiveWorkers.keyBegin(),
        m_ActiveWorkers.keyEnd());

    CALL_OUT("");
    return active_ids;
}



///////////////////////////////////////////////////////////////////////////////
// Remove workers
bool WorkerDispatch::Terminate(const QSet < int > & mcrWorkerIDs)
{
    CALL_IN(QString("mcrWorkerIDs=%1")
        .arg(CALL_SHOW(mcrWorkerIDs)));

    // First remove workers from the queue
    int index = 0;
    while (index < m_Queue.size())
    {
        AbstractWorker * worker = m_Queue[index];
        const int worker_id = worker -> GetWorkerID();
        if (mcrWorkerIDs.contains(worker_id))
        {
            m_Queue.removeAt(index);
        } else
        {
            index++;
        }
    }

    // Terminate currently running workers
    for (const int id : mcrWorkerIDs)
    {
        if (!m_ActiveWorkers.contains(id))
        {
            continue;
        }

        // Terminate worker
        m_ActiveWorkers[id] -> Cancel();
        m_ActiveWorkers.remove(id);

        // Delete thread
        delete m_WorkerIDToThread[id];
        m_WorkerIDToThread.remove(id);
    }

    CALL_OUT("");
    return false;
}



///////////////////////////////////////////////////////////////////////////////
// Worker finished
void WorkerDispatch::WorkerFinished(const int mcWorkerID)
{
    CALL_IN(QString("mcWorkerID=%1")
        .arg(CALL_SHOW(mcWorkerID)));

    // Lock while processing
    m_Mutex.lock();

    // Move worker to this thread
    m_WorkerIDToThread[mcWorkerID] -> exit();

    // Worker no longer active
    m_ActiveWorkers.remove(mcWorkerID);

    // Delete thread
    m_WorkerIDToThread.remove(mcWorkerID);

    // Lock while processing
    m_Mutex.unlock();

    // We don't have ownership of the worker object, so we don't delete it.
    emit Finished(mcWorkerID);

    if (m_Queue.isEmpty() &&
        m_ActiveWorkers.isEmpty())
    {
        emit AllFinished();
    }

    // Start next worker in queue
    StartNextWorker();

    CALL_OUT("");
}
