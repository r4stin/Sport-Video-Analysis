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


// Function to create a mask to detect balls on the table
bool BallDetection::processTableObjects(const cv::Mat& frame, const cv::Rect& roiRect) {
    // Extract the region of interest
    cv::Mat roi = frame(roiRect).clone();

    // Apply KMeans to segment the image
    TableDetection vp(this);
    cv::Mat km = vp.KMeans(roi);

    // Convert the image to grayscale
    cv::Mat gray;
    cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    // Apply Median Blur to reduce noise and Gaussian Blur to smooth the image
    cv::medianBlur(gray, gray, 7);
    cv::GaussianBlur(gray, gray, cv::Size(0, 0), 2);
    // Apply Canny Edge Detection
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 100);
    // Apply Morphological Closing to close the gaps in the edges
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    // Create a mask for the contours
    cv::Mat mask_ctr = cv::Mat::zeros(roi.size(), CV_8UC1);
    cv::drawContours(mask_ctr, contours, -1, cv::Scalar(255, 255, 255), 3);
    // Apply Morphological Dilation to thicken the contours
    cv::Mat kernel_dilate = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,5));
    cv::morphologyEx(mask_ctr, mask_ctr, cv::MORPH_DILATE, kernel_dilate, cv::Point(-1, -1), 2);
    // Combine the KMeans mask with the contours mask to improve the segmentation
    cv::bitwise_or(mask_ctr, km, km);
    // Remove groups of pixels with area less than 3000
    removePixel(km, 3000);
    // Create a mask for the table
    cv::Mat combined_mask = cv::Mat::zeros(frame.size(), CV_8UC1);
    km.copyTo(combined_mask(roiRect));

    // Combine the final mask with the original frame
    cv::Mat final_mask;
    cv::bitwise_and(frame, frame, final_mask, combined_mask);
    findCenters fc(this);
    centers_ = fc.findCenter(final_mask);
    if (centers_.empty()) {
        std::cerr << "Error: No circles detected!" << std::endl;
        return false;
    }

    return true;

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
    TableDetection vp(this);
    if (!vp.detectTableCorners(firstFrame)) {
        std::cerr << "Error: Could not detect table corners" << std::endl;
        return false;
    }
    std::vector<cv::Point2f> sortedCorners = sortCorners(vp.tableCorners_);

    cv::Rect boundingRect = cv::boundingRect(sortedCorners);

    cv::Mat black = cv::Mat::zeros(firstFrame.size(), CV_8UC1);

    cv::Mat green = firstFrame.clone();

    cv::Scalar fieldColor(5, 5, 5);

    std::vector<cv::Point> corners;
    for (const auto& pt : sortedCorners) {
        corners.emplace_back(pt);
    }
    cv::fillConvexPoly(black, corners, fieldColor);
    cv::fillConvexPoly(green, corners, cv::Scalar(0, 255, 0));

    while (capture_.read(frame)) {

        cv::Mat frame_border;
        cv::copyMakeBorder(frame, frame_border, N, N, 0, 0, cv::BORDER_CONSTANT);
        // Create a mask for the table to only process the table objects inside the table
        cv::Mat mask_table;
        cv::bitwise_and(frame, frame, mask_table, black);

        // Process the table objects
        if (!processTableObjects(mask_table, boundingRect)) {
            std::cerr << "Error: Could not detect table objects" << std::endl;
            return false;
        }

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
