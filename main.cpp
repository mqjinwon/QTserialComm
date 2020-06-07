#include "mainwindow.h"
#include <iostream>
#include <QApplication>
#include "darknet.h"
#include "network.h"
#include "opencv4/opencv2/opencv.hpp"

#define OPENCV 1

#include "image_opencv.h"

using namespace std;
using namespace cv;

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


// -----------------------------------------------------------------------------------------------------------------
// Define constants that were used when Darknet network was trained.
// This is pretty much hardcoded code zone, just to give an idea what is needed.
// -----------------------------------------------------------------------------------------------------------------

// Path to configuration file.
static char *cfg_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/yolov3-tiny.cfg");
// Path to weight file.
static char *weight_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/yolov3-tiny.weights");
// Path to a file describing classes names.
static char *names_file = const_cast<char *>("/home/jin/QTproject/QTserialComm/coco.names");
// This is an image to test.
static char *input = const_cast<char *>("/home/jin/QTproject/QTserialComm/dog.jpg");
// Define thresholds for predicted class.
float thresh = 0.5;
float hier_thresh = 0.5;
// Uncomment this if you need sort predicted result.
//    float nms = 0.45;

int main(int argc, char *argv[])
{

    cout << "Darknet application" << endl;

    //동영상 파일로부터 부터 데이터 읽어오기 위해 준비
        VideoCapture cap("/home/jin/QTproject/QTserialComm/videoplayback.mp4");
        if (!cap.isOpened())        {
            printf("동영상 파일을 열수 없습니다. \n");
        }

        //동영상 플레이시 크기를  320x240으로 지정
        cap.set(CAP_PROP_FRAME_WIDTH,320);
        cap.set(CAP_PROP_FRAME_HEIGHT,240);

        Mat frame;
        namedWindow("video", 1);

        // Number of classes in "obj.names"
        // This is very rude method and in theory there must be much more elegant way.
        size_t classes = 0;
        char **labels = get_labels(names_file);
        while (labels[classes] != nullptr) {
            classes++;
        }
        cout << "Num of Classes " << classes << endl;

        // -----------------------------------------------------------------------------------------------------------------
        // Do actual logic of classes prediction.
        // -----------------------------------------------------------------------------------------------------------------

        // Load Darknet network itself.
        network *net = load_network(cfg_file, weight_file, 0);
        // In case of testing (predicting a class), set batch number to 1, exact the way it needs to be set in *.cfg file
        set_batch_network(net, 1);

        while(1)        {
            //웹캡으로부터 한 프레임을 읽어옴
            cap >> frame;

            Mat rectImg(frame);

            image sized = letterbox_image(mat_to_image(frame), net->w, net->h);

            // Uncomment this if you need sort predicted result.
            //    layer l = net->layers[net->n - 1];

            // Get actual data associated with test image.
            float *frame_data = sized.data;

            // Do prediction.
            double time = what_time_is_it_now();
            network_predict(*net, frame_data);
            cout << "'" << input << "' predicted in " << (what_time_is_it_now() - time) << " sec." << endl;

            // Get number fo predicted classes (objects).
            int num_boxes = 0;
            detection *detections = get_network_boxes(net, frame.cols, frame.rows, thresh, hier_thresh, nullptr, 1, &num_boxes, 0);
            cout << "Detected " << num_boxes << " obj, class " << detections->classes << endl;


            // Uncomment this if you need sort predicted result.
            //    do_nms_sort(detections, num_boxes, l.classes, nms);

            // -----------------------------------------------------------------------------------------------------------------
            // Print results.
            // -----------------------------------------------------------------------------------------------------------------

            // Iterate over predicted classes and print information.
            for (int8_t i = 0; i < num_boxes; ++i) {
                for (uint8_t j = 0; j < classes; ++j) {
                    if (detections[i].prob[j] > thresh) {
                        // More information is in each detections[i] item.
                        cout << labels[j] << " " << (int16_t) (detections[i].prob[j] * 100) << "\t\t";
                        cout << " x: " << detections[i].bbox.x << " y: " << detections[i].bbox.y
                             << " w: " << detections[i].bbox.w  << " h: " << detections[i].bbox.h <<endl;

                        rectangle(rectImg, Point((detections[i].bbox.x - detections[i].bbox.w/2.)*rectImg.cols,
                                                 (detections[i].bbox.y - detections[i].bbox.h)*rectImg.rows),
                                           Point((detections[i].bbox.x + detections[i].bbox.w/2.)*rectImg.cols,
                                            (detections[i].bbox.y + detections[i].bbox.h)*rectImg.rows), Scalar(155, 155, 155) );
                    }
                }
            }

            // -----------------------------------------------------------------------------------------------------------------
            // Free resources.
            // -----------------------------------------------------------------------------------------------------------------

            imshow("video", rectImg);
            //30ms 정도 대기하도록 해야 동영상이 너무 빨리 재생되지 않음.
            if ( waitKey(20) == 27 ) break; //ESC키 누르면 종료

            free_detections(detections, num_boxes);
            free_image(sized);
        }


        free(labels);


    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
