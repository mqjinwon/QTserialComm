#ifndef IMGFORMAT_H
#define IMGFORMAT_H

#include "opencv4/opencv2/opencv.hpp"
#include "image_opencv.h"

cv::Mat image_to_mat(image img);


image mat_to_image(cv::Mat mat);



#endif // IMGFORMAT_H
