// 复制自Android源码中的锁对象
#ifndef CONDITION_H
#define CONDITION_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

#include <Mutex.h>

typedef int64_t nsecs_t; // nano-seconds

class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    enum WakeUpType {
        WAKE_UP_ONE = 0,
        WAKE_UP_ALL = 1
    };

    Condition();

    Condition(int type);

    ~Condition();

    status_t wait(Mutex &mutex);

    status_t waitRelative(Mutex &mutex, nsecs_t rel_time);

    void signal();

    void signal(WakeUpType type) {
        if (type == WAKE_UP_ONE) {
            signal();
        } else {
            broadcast();
        }
    }

    void broadcast();

private:
    pthread_cond_t mCond;
};

inline Condition::Condition() {
    //初始化
    pthread_cond_init(&mCond, NULL);
}

inline Condition::Condition(int type) {
    if (type == SHARED) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&mCond, &attr);
        pthread_condattr_destroy(&attr);
    } else {
        pthread_cond_init(&mCond, NULL);
    }
}

inline Condition::~Condition() {
    pthread_cond_destroy(&mCond);
}

inline status_t Condition::wait(Mutex &mutex) {
    // 因为 Condition 是 Mutex 的友联，所以才能使用 Mutex 的私有变量 mutex
    // 先会解除当前线程的互斥锁，然后挂线线程，等待条件变量满足条件。一旦条件变量满足条件，则会给线程上锁，继续执行
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}

inline status_t Condition::waitRelative(Mutex &mutex, nsecs_t rel_time) {
    struct timeval t;
    struct timespec ts;
    gettimeofday(&t, NULL);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec = t.tv_usec * 1000;

    ts.tv_sec += rel_time / 1000000000;
    ts.tv_nsec += rel_time % 1000000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
    }
    // 有 timeout 时间
    return -pthread_cond_timedwait(&mCond, &mutex.mMutex, &ts);
}

/** 通知 */
inline void Condition::signal() {
    // 激活等待列表中的线程，但是要明白的是，在一个线程的加锁代码中调用了此方法，并不是立即激活等待中的线程并执行，而是等当前线程释放锁以后，其他等待的线程才有机会取锁
    // 执行等待后面的代码
    pthread_cond_signal(&mCond);
}

inline void Condition::broadcast() {
    // 激活所有等待线程列表中最先入队的线程
    pthread_cond_broadcast(&mCond);
}

#endif //CONDITION_H
