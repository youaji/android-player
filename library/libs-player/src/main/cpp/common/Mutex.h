// 复制自Android源码中的锁对象
#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <pthread.h>
#include "Errors.h"

class Condition;

/** 互斥锁 */
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();

    Mutex(const char *name);

    Mutex(int type, const char *name = NULL);

    ~Mutex();

    // lock or unlock the mutex
    status_t lock();

    status_t unlock();

    // lock if possible; returns 0 on success, error otherwise
    status_t tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        // Autolock(Mutex& mutex) 属于构造函数，后面 ：mLock(mutex) 是给 mLock 变量赋值为 mutex 的意思
        // 构造函数，mLock 是给变量 mLock 赋值，然后在方法里面调用对应的 lock，因为调用了构造函数相当于创建一个对象，如果是在函数类调用构造函数，
        // 那么等函数执行完，自然就要释放这个局部变量对象，所以就自动会调用 ~Autolock 析构函数，然后就会进行解锁 mLock.unlock()
        inline Autolock(Mutex &mutex) : mLock(mutex) { mLock.lock(); }

        inline Autolock(Mutex *mutex) : mLock(*mutex) { mLock.lock(); }

        inline ~Autolock() { mLock.unlock(); }

    private:
        // 互斥锁的引用
        Mutex &mLock;
    };

private:
    // Condition 是友联类，代表 Condition 类可以用 Mutex 类的私有成员变量
    friend class Condition;

    // A mutex cannot be copied
    Mutex(const Mutex &);

    // operator 是重载的关键字，后面跟要重载的运算符，然后还有返回值和参数，如果要实现重载运算符的话，就要在大括号里面去实现
    // 这个 = 号的重载没有实现，所以当用 = 号 copy 的时候，是没有用的，所以说不能 copy
    Mutex &operator=(const Mutex &);

    pthread_mutex_t mMutex;
};

inline Mutex::Mutex() {
    // 初始化互斥锁
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::Mutex(const char *name) {
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::Mutex(int type, const char *name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}

inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}

inline status_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}

inline status_t Mutex::unlock() {
    return -pthread_mutex_unlock(&mMutex);
}

inline status_t Mutex::tryLock() {
    // 函数 pthread_mutex_trylock 会尝试对互斥量加锁，如果该互斥量已经被锁住，函数调用失败，返回 EBUSY，否则加锁成功返回0，线程不会被阻塞
    return -pthread_mutex_trylock(&mMutex);
}

typedef Mutex::Autolock AutoMutex;

#endif //MUTEX_H
