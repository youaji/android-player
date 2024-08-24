#ifndef FFMPEG4_COORDINATEUTILS_H
#define FFMPEG4_COORDINATEUTILS_H

/**
 * 旋转模式
 */
typedef enum {
    ROTATE_NONE = 0,        //
    ROTATE_90,              // 旋转90度
    ROTATE_180,             // 旋转180度
    ROTATE_270,             // 旋转270度
    ROTATE_FLIP_VERTICAL,   // 纵向翻转
    ROTATE_FLIP_HORIZONTAL, // 横向翻转
} RotationMode;

/**
 * 坐标点工具
 */
class CoordinateUtils {

public:
    /**
     * @return 获取顶点坐标
     */
    static const float *getVertexCoordinates();

    /**
     * @return 获取默认索引
     */
    static const short *getDefaultIndices();

    /**
     * @param rotationMode
     * @return 获取纹理坐标
     */
    static const float *getTextureCoordinates(const RotationMode &rotationMode);

    /**
     * @param rotationMode
     * @return 获取纹理坐标
     */
    static const float *getInputTextureCoordinates(const RotationMode &rotationMode);

private:
    CoordinateUtils() = default;

    virtual ~CoordinateUtils() {}
};

#endif //FFMPEG4_COORDINATEUTILS_H
