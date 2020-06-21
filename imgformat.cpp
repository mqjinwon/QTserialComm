#include "imgformat.h"

cv::Mat image_to_mat(image img)
{
    int channels = img.c;
    int width = img.w;
    int height = img.h;
    cv::Mat mat = cv::Mat(height, width, CV_8UC(channels));
    int step = mat.step;

    for (int y = 0; y < img.h; ++y) {
        for (int x = 0; x < img.w; ++x) {
            for (int c = 0; c < img.c; ++c) {
                float val = img.data[c*img.h*img.w + y*img.w + x];
                mat.data[y*step + x*img.c + c] = (unsigned char)(val * 255);
            }
        }
    }
    return mat;
}

image mat_to_image(cv::Mat mat)
{
    int w = mat.cols;
    int h = mat.rows;
    int c = mat.channels();
    image im = make_image(w, h, c);
    unsigned char *data = (unsigned char *)mat.data;
    int step = mat.step;
    for (int y = 0; y < h; ++y) {
        for (int k = 0; k < c; ++k) {
            for (int x = 0; x < w; ++x) {
                //uint8_t val = mat.ptr<uint8_t>(y)[c * x + k];
                //uint8_t val = mat.at<Vec3b>(y, x).val[k];
                //im.data[k*w*h + y*w + x] = val / 255.0f;

                im.data[k*w*h + y*w + x] = data[y*step + x*c + k] / 255.0f;
            }
        }
    }
    return im;
}
