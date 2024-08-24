#ifndef MEDIACLOCK_H
#define MEDIACLOCK_H

#include <math.h>

extern "C" {
#include "libavutil/time.h"
}

class MediaClock {

public:
    /**
     * 媒体时钟
     */
    MediaClock();

    /**
     * 媒体时钟
     */
    virtual ~MediaClock();

    /**
     * 初始化
     */
    void init();

    /**
     * 获取时钟
     * @return 时钟
     */
    double getClock();

    /**
     * 设置时钟
     * @param pts
     * @param time
     */
    void setClock(double pts, double time);

    /**
     * 设置时钟
     * @param pts
     */
    void setClock(double pts);

    /**
     * 设置速度
     * @param speed
     */
    void setSpeed(double speed);

    /**
     * 同步到从属时钟
     * @param slave
     */
    void syncToSlave(MediaClock *slave);

    /**
     * 获取时钟速度
     * @return 时钟速度
     */
    double getSpeed() const;

private:
    double mPTS;             // PTS（Presentation Time Stamp）显示时间戳，这个时间戳用来告诉播放器该在什么时候显示这一帧的数据。具体可科普PTS概念！
    double mPTS_drift;       //
    double mLastUpdated;     //
    double mSpeed;           //
    int mPaused;             //
};

#endif //MEDIACLOCK_H
