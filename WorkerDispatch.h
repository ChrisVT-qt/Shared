// WorkerDispatch.h
// Class definition

#pragma once

// Project includes

// Qt includes
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QThreadPool>

// Forward declaration
class AbstractWorker;



class WorkerDispatch :
    public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
private:
    // Constructor
    WorkerDispatch();

public:
    // Instance
    static WorkerDispatch * Instance();
private:
    static WorkerDispatch * m_Instance;

protected:
    // Destructor
    ~WorkerDispatch();



    // ====================================================== Thread Management
public:
    // Number of concurrent threads
    int GetNumberOfConcurrentThreads() const;
    void SetNumberOfConcurrentThreads(const int mcNumber);

    // Start/stop work
    void Start();
    void Start_WaitForFinish();
    void Stop();
private:
    bool m_IsRunning;

public:
    // Add task to queue
    // (connect signals for each worker before calling AddWorker() or
    // AddWorkers() to avoid a race condition)
    bool AddWorker(AbstractWorker * mpWorker);
    bool AddWorkers(const QList < AbstractWorker * > & mcrWorker);

    // Active workers
    QSet < int > GetActiveWorkerIDs() const;

    // Returns true iff every requested worker was removed before it ever
    // started running. Workers already running are asked (cooperatively)
    // to cancel, but keep running until they notice.
    bool Terminate(const QSet < int > & mcrWorkerIDs);

private:
    // Submit worker to thread pool
    void SubmitWorker(AbstractWorker * mpWorker);

    // Thread pool
    QThreadPool m_ThreadPool;

    // Workers that haven't been started yet
    QList < AbstractWorker * > m_PendingWorkers;

    // Workers doing work
    QHash < int, AbstractWorker * > m_SubmittedWorkers;

    // Worker finished (completed or aborted)
    void WorkerFinished(const int mcWorkerID, const bool mcWasSuccessful);

signals:
    void Finished(const int mcWorkerID, const bool mcWasSuccessful);
    void AllFinished();
};
