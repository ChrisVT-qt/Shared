// WorkerDispatch.h
// Class definition

#pragma once

// Project includes

// Qt includes
#include <QHash>
#include <QMutex>
#include <QObject>

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

public:
    // Destructor
    ~WorkerDispatch();



    // ====================================================== Thread Management
public:
    // Number of concurrent threads
    int GetNumberOfConcurrentThreads() const;
    void SetNumberOfConcurrentThreads(const int mcNumber);
private:
    int m_NumberOfConcurrentThreads;

public:
    // Start/stop work
    void Start();
    void Stop();
private:
    void StartNextWorker();
    bool m_IsRunning;

public:
    // Add task to queue
    bool AddWorker(AbstractWorker * mpWorker);
    bool AddWorkers(const QList < AbstractWorker * > & mcrWorker);

    // Active workers
    QSet < int > GetActiveWorkerIDs() const;

    // Remove workers
    bool Terminate(const QSet < int > & mcrWorkerIDs);

private:
    QMutex m_Mutex;
    QList < AbstractWorker * > m_Queue;
    QHash < int, AbstractWorker * > m_ActiveWorkers;
    QHash < int, QThread * > m_WorkerIDToThread;

private slots:
    // Worker finished
    void WorkerFinished(const int mcWorkerID);

private:
    bool m_AllWorkersFinishedSignalSent;

signals:
    void Finished(const int mcWorkerID);
    void AllFinished();
};
