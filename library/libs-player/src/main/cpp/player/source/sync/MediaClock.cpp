#include <PlayerState.h>
#include "MediaClock.h"

MediaClock::MediaClock() {
    init();
}

MediaClock::~MediaClock() {}

void MediaClock::init() {
    mSpeed = 1.0;
    mPaused = 0;
    setClock(NAN);
}

double MediaClock::getClock() {
    if (mPaused) {
        return mPTS;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return mPTS_drift + time - (time - mLastUpdated) * (1.0 - mSpeed);
    }
}

void MediaClock::setClock(double pts, double time) {
    this->mPTS = pts;
    this->mLastUpdated = time;
    // 时间偏移
    this->mPTS_drift = this->mPTS - time;
}

void MediaClock::setClock(double pts) {
    // 当前时间
    double time = av_gettime_relative() / 1000000.0;
    setClock(pts, time);
}

void MediaClock::setSpeed(double speed) {
    setClock(getClock());
    this->mSpeed = speed;
}

void MediaClock::syncToSlave(MediaClock *slave) {
    double clock = getClock();
    double slave_clock = slave->getClock();
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        setClock(slave_clock);
    }
}

double MediaClock::getSpeed() const {
    return mSpeed;
}

