// AbstractWorker.h
// Class definition

#pragma once

// Qt includes
#include <QAtomicInteger>
#include <QObject>
#include <QRunnable>



// Class definition
class AbstractWorker
    : public QObject
    , public QRunnable
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
    static QAtomicInteger < int > m_NextWorkerID;
    int m_WorkerID;

public:
    // Implement in derived class
    // bool Configure(any parameters);
    bool IsConfigured() const;
protected:
    bool m_IsConfigured;

public:
    // Cancel work
    void Cancel();
protected:
    QAtomicInteger < bool > m_IsCanceled;

private:
    // Start work
    void run() override;

protected:
    // Actual work
    virtual bool DoWork() = 0;


signals:
    void Finished(const bool mcSuccess);
};
