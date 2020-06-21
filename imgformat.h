#ifndef IMGFORMAT_H
#define IMGFORMAT_H

#include "opencv4/opencv2/opencv.hpp"
#include "image_opencv.h"

cv::Mat image_to_mat(image img);

image mat_to_image(cv::Mat mat);

image resize_image(image im, int w, int h);

#endif // IMGFORMAT_H
