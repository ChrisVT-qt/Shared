// AbstractWorker.h
// Class definition

#pragma once

// Qt includes
#include <QObject>



// Class definition
class AbstractWorker
    : public QObject
{
    Q_OBJECT



    // ============================================================== Lifecycle
protected:
    // Constructor
    AbstractWorker();

public:
    // Destructor
    virtual ~AbstractWorker();



    // ======================================================== Everything else
public:
    // Worker ID
    int GetWorkerID() const;
protected:
    static int m_NextWorkerID;
    int m_WorkerID;

public:
    // Implement in derived class
    // bool Configure(any parameters);
    bool IsConfigured() const;
protected:
    bool m_IsConfigured;

public:
    // Start work
    virtual bool StartWork() = 0;

    // Check if worker is idle
    bool IsIdle() const;
protected:
    bool m_IsIdle;

public:
    // Cancel work
    void Cancel();
protected:
    bool m_IsCanceled;

signals:
    void Finished();
};
