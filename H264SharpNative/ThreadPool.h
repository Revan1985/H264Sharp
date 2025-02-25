#ifndef THREADPOOL
#define THREADPOOL
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <functional>
#include <iostream>
#include <atomic>
#include"pch.h"

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h> 
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/semaphore.h>
#endif

#ifdef _WIN32
#include<ppl.h>
#include <limits.h>
#endif


#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()

#elif defined(__aarch64__) || defined(__arm__)
#include <arm_acle.h>
#define CPU_PAUSE() __yield()

#else
#define CPU_PAUSE() std::this_thread::yield()
#endif
class SemaphoreSlim
{
public:

    SemaphoreSlim() {}

    void Set()
    {
        std::lock_guard<std::mutex> _(mainMutex);
        cnt++;
        cv.notify_one();
}

    void Set(int num)
    {
        std::lock_guard<std::mutex> _(mainMutex);
        cnt += num;
        for (int i = 0; i < num; i++)
        {
            cv.notify_one();
        }
    }

    void WaitOne()
    {
        std::unique_lock<std::mutex> lk(mainMutex);
        while (cnt == 0)
            cv.wait(lk);
        cnt--;
    }

private:

    SemaphoreSlim(const SemaphoreSlim&) = delete;
    SemaphoreSlim& operator=(const SemaphoreSlim&) = delete;
    std::mutex mainMutex;
    std::condition_variable cv;
    int cnt = 0;
};
#define WFUTEX
class Semaphore {

public:
    
#if defined(_WIN32)
#ifndef WFUTEX
    Semaphore(int initial = 0)
    {
        win_semaphore = CreateSemaphore(nullptr, initial, INT_MAX, nullptr);
    }

    ~Semaphore()
    {
        CloseHandle(win_semaphore);
    }

    void Set()
    {
        Set(1);
    }

    void Set(int num)
    {
        ReleaseSemaphore(win_semaphore, num, nullptr);
    }

    void WaitOne()
    {
        WaitForSingleObject(win_semaphore, INFINITE);
    }

#else
    
    Semaphore(int initial = 0) : semCnt(initial) {}


    void Set() {
        Set(1);
    }

    void Set(int num) {
        InterlockedAdd(&semCnt, num);

        if (num == 1) {
            WakeByAddressSingle(&semCnt);
        }
        else {
            WakeByAddressAll(&semCnt);
        }
    }

    void WaitOne() {
        while (true) {
            long current = semCnt;

            if (current > 0) {
                if (InterlockedCompareExchange(&semCnt, current - 1, current) == current) {
                    return;
                }
                continue;
            }

            long zero = 0;
            WaitOnAddress(&semCnt, &zero, sizeof(semCnt), INFINITE);
        }
    }
#endif // !WFUTEX

#elif defined(__linux__) || defined(__ANDROID__)
    Semaphore(int initial = 0)
    {
        count.store(initial, std::memory_order_release);
    }

    void Set()
    {
        Set(1);
    }

    void Set(int num)
    {
        count.fetch_add(num, std::memory_order_release);
        syscall(SYS_futex, &count, FUTEX_WAKE_PRIVATE, num, nullptr, nullptr, 0);
    }

    void WaitOne()
    {
        while (true) {
            int expected = count.load(std::memory_order_acquire);
            if (expected > 0 && count.compare_exchange_weak(expected, expected - 1, std::memory_order_acquire)) {
                return;
            }
            syscall(SYS_futex, &count, FUTEX_WAIT_PRIVATE, 0, nullptr, nullptr, 0);
        }
    }
#elif defined(__APPLE__)
    Semaphore(int initial = 0)
    {
        semaphore_create(mach_task_self(), &mach_semaphore, SYNC_POLICY_FIFO, initial);
    }

    ~Semaphore()
    {
        semaphore_destroy(mach_task_self(), mach_semaphore);
    }

    void Set()
    {
        Set(1);
    }

    void Set(int num)
    {
        for (int i = 0; i < num; ++i)
        {
            semaphore_signal(mach_semaphore);
        }
    }

    void WaitOne()
    {
        semaphore_wait(mach_semaphore);
    }
#endif

private:
#if defined(__linux__) || defined(__ANDROID__)
    std::atomic<int> count;
#elif defined(__APPLE__)
    semaphore_t mach_semaphore;
#elif defined(_WIN32)
#ifndef WFUTEX
    HANDLE win_semaphore;
#endif // !WFUTEX
    alignas(8) long semCnt;  
#endif
};

class SpinWait {
public:
    void wait() {
        int backoff = 1;
        while (!flag.load(std::memory_order_acquire)) 
        {
            for (size_t i = 0; i < backoff; i++)
            {
                CPU_PAUSE();
            }
            if (backoff < 16)
                backoff *= 2;
        }
           
    }

    void notify() {
        flag.store(true, std::memory_order_release);
    }

    void reset() {
        flag.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic<bool> flag{false};
};


class SpinLock {
public:
    SpinLock() { flag.clear(std::memory_order_relaxed); }

    void lock() 
    {
        int backoff = 1;
        while (flag.test_and_set(std::memory_order_acquire))
        {
            for (size_t i = 0; i < backoff; i++)
            {
                CPU_PAUSE();
            }
            if (backoff < 8)
                backoff*=2;
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

template<typename T>
class ConcurrentQueue {
public:
    virtual void Enqueue(T&& item) = 0;
    virtual bool TryDequeue(T& out) = 0;
};

template<typename T>
class LockingQueue : public ConcurrentQueue<T>
{
public:

    void Enqueue(T&& item)
    {
        spinLock.lock();
        workQueue.emplace_back(std::forward<T>(item));
        spinLock.unlock();
    };

    bool TryDequeue(T& out)
    {
        spinLock.lock();
        if (!workQueue.empty())
        {
            out = std::move(workQueue.front());
            workQueue.pop_front();
            spinLock.unlock();
            return true;
        }
        spinLock.unlock();
        return false;
    };

private:
    std::mutex qLock;
    std::deque<T> workQueue;
    SpinLock spinLock;
};


class ThreadPoolC
{
private:

    std::vector<std::thread> threads;
    std::atomic_bool kill = false;
    int poolSize=0;
    int maxPoolSize = std::thread::hardware_concurrency() - 1;

    std::vector< LockingQueue<std::function<void()>>* > threadLocalQueues;
    std::vector< Semaphore* > threadLocalSignals;
    std::mutex m;
    std::atomic<int> activeThreadCount = 0;


public:

    ThreadPoolC()
    {  
        threadLocalQueues.reserve(maxPoolSize);
        threadLocalSignals.reserve(maxPoolSize);

    };
    ThreadPoolC(int poolSize)
    {
        threadLocalQueues.reserve(maxPoolSize);
        threadLocalSignals.reserve(maxPoolSize);

		ExpandPool(poolSize);
    };

    ~ThreadPoolC()
    {
        kill = true;
        for (size_t i = 0; i < activeThreadCount; i++)
        {
            if(threadLocalSignals[i] != nullptr)
                threadLocalSignals[i]->Set();
        }
        for (int i = 0; i < poolSize; i++)
        {
            if (threads[i].joinable())
                threads[i].join();
            //std::cout << "Joined" << "\n";

        }
    }

    inline void ExpandPool(int numThreads)
    {
        int expansion = numThreads - poolSize;
        if (expansion < 1)
            return;  
        if (poolSize==maxPoolSize)
            return;

        std::lock_guard<std::mutex> _(m);

        expansion = numThreads - poolSize;
        if (expansion < 1)
            return;

        if (expansion > (maxPoolSize - poolSize))
            expansion = maxPoolSize - poolSize;

        std::cout << "Expanded by " << expansion << " threads\n";
       
        for (int i = 0; i < expansion; i++)
        {
            SpinWait initSignal;
            std::thread t([this, &initSignal]() { Execution(initSignal); });
          
            threads.emplace_back(std::move(t));
            initSignal.wait();
        }

        poolSize += expansion;
    }

    template<typename F>
    void For2(int fromInclusive, int toExclusive, F&& lambda)
    {

        const int numIter = toExclusive - fromInclusive;

        if (numIter <= 0)
            return;

        if (numIter == 1)
        {
            lambda(toExclusive - 1);
            return;
        }

        ExpandPool(numIter - 1);

        

        std::mutex m1;
        std::atomic<int> remainingWork(numIter);
        std::unique_lock<std::mutex> completionLock(m1, std::defer_lock);

        std::condition_variable cnd;
        bool alreadySignalled = false;

        signal.Set(numIter - 1);
        for (int i = fromInclusive; i < toExclusive - 1; i++)
        {
            workQueue.Enqueue([i, &cnd, &m1, &lambda, &alreadySignalled, &remainingWork]()
                {
                    lambda(i);
                    if (--remainingWork < 1)
                    {
                        std::lock_guard<std::mutex> lock(m1);
                        alreadySignalled = true;
                        cnd.notify_one();
                    }

                });
            //signal.Set();
        }
        signal.Set(numIter-1);
        lambda(toExclusive - 1);

        if (--remainingWork > 0)
        {
            Steal(remainingWork);

            completionLock.lock();
            if (!alreadySignalled)
                cnd.wait(completionLock, [&alreadySignalled] { return alreadySignalled; });
        }

    };

    template<typename F>
    void For(int fromInclusive, int toExclusive, F&& lambda)
    {

        const int numIter = toExclusive - fromInclusive;

        if (numIter <= 0)
            return;

        if (numIter == 1)
        {
            lambda(toExclusive - 1);
            return;
        }

        ExpandPool(numIter - 1);
        std::atomic<int> remainingWork(numIter);
        SpinWait spin;

        for (int i = fromInclusive; i < toExclusive - 1; i++)
        {
            threadLocalQueues[i]->Enqueue([i, &lambda, &spin, &remainingWork]()
                {
                    lambda(i);
                    if (--remainingWork < 1)
                    {
                        spin.notify();
                    }

                });
            threadLocalSignals[i]->Set();
        }

        lambda(toExclusive - 1);

        if (--remainingWork > 0)
        {
            Steal(remainingWork);
            spin.wait();
        }

    };
private:

    inline void Execution(SpinWait& qsignal)
    {
        thread_local std::function<void()> task;
        LockingQueue< std::function<void()> > workQueue;
        Semaphore signal;

        threadLocalQueues[activeThreadCount] = &workQueue;
        threadLocalSignals[activeThreadCount] = &signal;
        activeThreadCount++;

        qsignal.notify();

        while (true)
        {
            signal.WaitOne();
            if (kill)
            {
                signal.Set();
                break;
            }

            while (workQueue.TryDequeue(task))
            {

                try
                {
                    task();
                }
                catch (const std::exception& ex)
                {
                    std::cout << "Exception occured on thread pool delegate: " << ex.what() << "\n";
                }
            }

            for (size_t i = 0; i < activeThreadCount; i++)
            {
                if (threadLocalQueues[i] != nullptr)
                {
                    if (threadLocalQueues[i]->TryDequeue(task))
                    {
                        try
                        {
                            task();
                        }
                        catch (const std::exception& ex)
                        {
                            std::cout << "Exception occured on thread pool delegate : " << ex.what() << "\n";
                        }
                    }
                }
            }

        }

    };


    inline void Steal(std::atomic<int>& remainingWork)
    {
        thread_local std::function<void()> task;
        if (remainingWork > 0)
        {

            for (size_t i = 0; i < activeThreadCount; i++)
            {
                if (threadLocalQueues[i] != nullptr)
                {
                    if (threadLocalQueues[i]->TryDequeue(task))
                    {
                        try
                        {
                            //std::cout << "Stole : ";
                            task();
                            if(remainingWork == 0)
                                return;
                        }
                        catch (const std::exception& ex)
                        {
                            std::cout << "Exception occured on thread pool delegate : " << ex.what() << "\n";
                        }
                    }
                }
            }
        }
    }

};

class ThreadPool {
private:
   

public:
   
    static std::unique_ptr<ThreadPoolC> pool;
    

#ifdef _WIN32
    template<typename F>
    static void For(int i, int j, F&& lamb)
    {
        if (UseCustomPool>0) 
        {
            pool->For(i, j, std::forward<F>(lamb));
        }
        else 
        {
            concurrency::parallel_for(i, j, std::forward<F>(lamb));
        }
       
    }

    static int UseCustomPool;
    static int CustomPoolInitialized;
    static void SetCustomPool(int value);

#else
    
    template<typename F>
    static void For(int i, int j, F&& lamb)
    {
        pool->For(i, j, std::forward<F>(lamb));
    }

#endif

};


#endif





