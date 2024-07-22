/*
 * File:    BallDetection.cpp
 * Author:  MOHAMMADHOSSEIN AKBARI MOAFI
 * Date:    July 17, 2024
 * Description: This file contains the implementation of the BallDetection class.
 *             The class provides the create a mask to detect center of each ball on the table, refine the circles,
 *             create the table,a function to remove groups of pixels with area less than a specified value,
 *             draw balls on the table, draw holes on the table, transform a point using a perspective transformation matrix,
 *             create the top view minimap, save the detections.
 */

#include "BallDetection.h"

BallDetection::BallDetection() = default;

// Function to remove groups of pixels with area less than rmp
cv::Mat BallDetection::removePixel(cv::Mat img, int rmp)
{
    cv::Mat labels;
    int num_components = cv::connectedComponents(img, labels);

    int min_size = rmp;
    for (int i = 1; i < num_components; i++) {
        cv::Mat component_mask = (labels == i);
        int area = cv::countNonZero(component_mask);
        if (area > min_size) {
            img.setTo(0, component_mask);
        }

    }
    return img;
}



// Function to process the video
bool BallDetection::process_video(const std::string& input_path,const std::string& output_path) {
    std::cout << "Processing video..." << std::endl;

    capture_.open(input_path);
    if (!capture_.isOpened()) {
        std::cerr << "Error opening video stream or file" << std::endl;
        return false;
    }

    int W = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
    int H = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Size final_size(W, H);

    int N = 10;
    int frame_num = 0;

    int total_frames = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_COUNT));
    int FPS = static_cast<int>(capture_.get(cv::CAP_PROP_FPS));
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter out(output_path, fourcc, FPS, final_size);

    cv::Mat firstFrame, frame;
    // Read the first frame to detect the table corners
    capture_ >> firstFrame;

    while (capture_.read(frame)) {

        cv::Mat frame_border;
        cv::copyMakeBorder(frame, frame_border, N, N, 0, 0, cv::BORDER_CONSTANT);

        // Resize the minimap according to the frame size
        cv::Size mini_map_size(static_cast<int>(frame_border.rows * 0.25), static_cast<int>(frame_border.cols * 0.25));
        cv::Mat resized_top_view;
        cv::resize(top_view_, resized_top_view, mini_map_size);
        cv::rotate(resized_top_view, resized_top_view, cv::ROTATE_90_CLOCKWISE);
        cv::copyMakeBorder(resized_top_view, resized_top_view, 10, 10, 10, 10, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

        int offset_x = 10;
        int offset_y = frame_border.rows - resized_top_view.rows - 10;

        // Create final output
        cv::Mat final = frame_border.clone();
        resized_top_view.copyTo(final(cv::Rect(offset_x, offset_y, resized_top_view.cols, resized_top_view.rows)));

        cv::resize(final,final,  final_size, 0, 0, cv::INTER_AREA);
        cv::imshow("Output", final);
        out.write(final);
        centers_.clear();
        centers_ref_.clear();
        radius_.clear();
        frame_num++;
        if (cv::waitKey(1) == 27) break;
    }

    capture_.release();
    out.release();
    cv::destroyAllWindows();

    return true;
}
