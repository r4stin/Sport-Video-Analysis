#ifndef BALLDETECTION_H
#define BALLDETECTION_H
#include "header.h"

class BallDetection {

public:
    BallDetection();
    cv::Mat removePixel(cv::Mat img, int rmp);
    bool processTableObjects(const cv::Mat& frame, const cv::Rect& roiRect);
    cv::Mat create_table(int width, int height);
    cv::Mat draw_balls( const std::vector<cv::Point2f>& minimapBallPositions, const cv::Mat& background, int radius, int size, const cv::Mat& img);
    cv::Mat draw_holes(const cv::Mat& input_img);
    bool createTopViewMinimap(const std::vector<cv::Point2f>& ballPositions, const cv::Mat& img, const std::vector<cv::Point2f>& tableCorners);
    bool outputGenerator(const std::vector<cv::Point2f>& ballPositions, const cv::Mat& img, int radius, const cv::Mat& mask_table, const cv::Mat& bb_table, const std::string& filename);
    static void saveDetections(const std::string& filename, const std::vector<cv::Point2f>& centers, const std::vector<int>& labels, const std::vector<cv::Rect>& boundingBoxes);
    bool centerRefinement(cv::Mat img);
    bool process_video(const std::string& input_path,const std::string& output_path);








private:
    cv::VideoCapture capture_;
    cv::Mat top_view_;
    std::vector<cv::Point2f> centers_;
    std::vector<cv::Point2f> centers_ref_;
    std::vector<float> radius_;
    std::vector<cv::Point2f> points_;
    cv::Point2f transformPoint(const cv::Point2f& point, const cv::Mat& transformMatrix);



    int width_ = 400;
    int height_ = 800;




    friend class TableDetection;
    friend class findCenters;

};

class TableDetection {
public:

    explicit TableDetection(BallDetection *ballDetection);
    bool detectTableCorners(const cv::Mat &firstFrame);
    cv::Point2f computeIntersection(cv::Vec2f line1, cv::Vec2f line2);
    cv::Mat KMeans(cv::Mat src);
    std::vector<cv::Point2f> tableCorners_;


private:
    BallDetection *ballDetection_;



};

class findCenters {
public:
    explicit findCenters(BallDetection *ballDetection1);
    std::vector<cv::Point2f> findCenter(cv::Mat img);




private:
    BallDetection *ballDetection_;


};


#endif //BALLDETECTION_H
