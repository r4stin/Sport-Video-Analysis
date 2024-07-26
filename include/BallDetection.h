#ifndef BALLDETECTION_H
#define BALLDETECTION_H
#include "header.h"

class BallDetection {

public:
    BallDetection();
    cv::Mat removePixel(cv::Mat img, int rmp);
    bool processTableObjects(const cv::Mat& frame, const cv::Rect& roiRect);
    bool process_video(const std::string& input_path,const std::string& output_path);









private:
    cv::VideoCapture capture_;
    std::vector<cv::Point2f> centers_;
    std::vector<cv::Point2f> centers_ref_;
    std::vector<float> radius_;




    friend class TableDetection;
        friend class findCenters;

};

class TableDetection {
public:

    explicit TableDetection(BallDetection *ballDetection);


private:
    BallDetection *ballDetection_;



};

class findCenters {
public:
    explicit findCenters(BallDetection *ballDetection1);



private:
    BallDetection *ballDetection_;


};


#endif //BALLDETECTION_H
