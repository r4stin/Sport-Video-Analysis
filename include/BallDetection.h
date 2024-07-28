#ifndef BALLDETECTION_H
#define BALLDETECTION_H
#include "header.h"

class BallDetection {

public:
    BallDetection();
    cv::Mat removePixel(cv::Mat img, int rmp);
    bool processTableObjects(const cv::Mat& frame, const cv::Rect& roiRect);
    bool process_video(const std::string& input_path,const std::string& output_path);
    cv::Mat create_table(int width, int height);
    cv::Mat draw_balls( const std::vector<cv::Point2f>& minimapBallPositions, const cv::Mat& background, int radius, int size, const cv::Mat& img);









private:
    cv::VideoCapture capture_;
    cv::Mat top_view_;
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
