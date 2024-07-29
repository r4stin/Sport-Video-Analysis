/*
 * File:    TableDetection.cpp
 * Author:  ZAHRA RAHGOOY
 * Date:    July 17, 2024
 * Description: This file contains the implementation of the TableDetection class
 *             which is responsible for detecting the table corners in the first frame
 *             and also the implementation of the kmMeans function which is used to
 *             create a mask for the ball detection.
 */

#include "../include/BallDetection.h"


TableDetection::TableDetection(BallDetection* ballDetection) : ballDetection_(ballDetection) {}

// Function to calculate the perpendicular distance from a point to a line
double distanceToLine(cv::Point pt, cv::Vec2f line) {
    float rho = line[0], theta = line[1];
    double a = cos(theta), b = sin(theta);
    double x0 = a * rho, y0 = b * rho;
    return abs(a * pt.x + b * pt.y - rho) / sqrt(a * a + b * b);
}

// Function to draw a line given in polar coordinates
void drawLine(cv::Vec2f line, cv::Mat &img, cv::Scalar color) {
    float rho = line[0], theta = line[1];
    cv::Point pt1, pt2;
    double a = cos(theta), b = sin(theta);
    double x0 = a * rho, y0 = b * rho;
    pt1.x = cvRound(x0 + 1000 * (-b));
    pt1.y = cvRound(y0 + 1000 * (a));
    pt2.x = cvRound(x0 - 1000 * (-b));
    pt2.y = cvRound(y0 - 1000 * (a));
    cv::line(img, pt1, pt2, cv::Scalar(0, 255, 0), 3, cv::LINE_AA);

}

bool TableDetection::detectTableCorners(const cv::Mat& firstFrame) {

    // Convert the first frame to grayscale
    cv::Mat grayFrame;
    cv::cvtColor(firstFrame, grayFrame, cv::COLOR_BGR2GRAY); // Convert the first frame to grayscale
    // Apply Gaussian blur to the grayscale frame
    cv::Mat blurredFrame;
    cv::GaussianBlur(grayFrame, blurredFrame, cv::Size(5, 5), 0); // Apply Gaussian blur to the grayscale frame
    // Apply Canny edge detection to the blurred frame
    cv::Mat edges;
    cv::Canny(blurredFrame, edges, 50, 100); // Apply Canny edge detection to the blurred frame
    // Detect lines in the frame using Hough Line Transform
    std::vector<cv::Vec2f> lines, lines_right, lines_left;
    cv::HoughLines(edges, lines, 0.8, CV_PI / 180, 100); // Apply Hough Line Transform to detect lines

    // Central pixel
    cv::Point center(firstFrame.cols / 2, firstFrame.rows / 2);
    // Distance threshold
    double distanceThreshold = 150.0;
    // Variables to store the nearest lines in each direction
    cv::Vec2f nearestUpLine, nearestDownLine, nearestLeftLine, nearestRightLine;
    double minUpDist = DBL_MAX, minDownDist = DBL_MAX, minLeftDist = DBL_MAX, minRightDist = DBL_MAX;
    // Flag to check if the nearest lines are found
    bool foundUp = false, foundDown = false, foundLeft = false, foundRight = false;
    // Categorize and find the nearest lines
    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0], theta = lines[i][1];
        double distance = distanceToLine(center, lines[i]);
        if (distance < distanceThreshold) {
            continue; // Skip lines that are within the threshold distance
        }

        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;

        if (abs(a) > abs(b)) { // Vertical lines (left or right)

            if (x0 < center.x) { // Left
                if (distance < minLeftDist) {
                    minLeftDist = distance;
                    nearestLeftLine = lines[i];
                    foundLeft = true;
                }
            } else if (x0 > center.x) { // Right


                if (distance < minRightDist) {
                    minRightDist = distance;
                    nearestRightLine = lines[i];
                    foundRight = true;
                }
            }
        } else { // Horizontal lines (up or down)


            if (y0 < center.y) { // Up
                if (distance < minUpDist) {
                    minUpDist = distance;
                    nearestUpLine = lines[i];
                    foundUp = true;
                }
            } else if (y0 > center.y) { // Down
                if (distance < minDownDist) {
                    minDownDist = distance;
                    nearestDownLine = lines[i];
                    foundDown = true;
                }
            }
        }
    }

//    std::cout << "Found up: " << foundUp << " Found down: " << foundDown << " Found left: " << foundLeft
//              << " Found right: " << foundRight << std::endl;
// First refinement step
    //Check if we have not found either the up or down lines or the left or right lines and try to find them
    if (!foundUp || !foundDown || !foundLeft || !foundRight) {
        for (size_t i = 0; i < lines.size(); i++) {
            float rho = lines[i][0], theta = lines[i][1];
            double distance = distanceToLine(center, lines[i]);
            if (distance < distanceThreshold) {
                continue; // Skip lines that are within the threshold distance
            }

            double a = cos(theta), b = sin(theta);
            double x0 = a * rho, y0 = b * rho;

            if (abs(a) > abs(b)) { // Vertical lines (left or right)

                if (x0 < center.x && rho > 0.0) { // Left
                    if (distance < minLeftDist) {
                        minLeftDist = distance;
                        nearestLeftLine = lines[i];
                        foundLeft = true;
                    }
                } else if (x0 > center.x || rho < 0.0) { // Right

                    if (distance < minRightDist) {
                        minRightDist = distance;
                        nearestRightLine = lines[i];
                        foundRight = true;
                    }
                }
            }
        }
    }
// Second refinement step
//    // Check if the left flag is false and draw a line on the opposite side
    if (!foundLeft) {
        distanceThreshold = 120.0;
        for (size_t i = 0; i < lines.size(); i++) {
            float rho = lines[i][0], theta = lines[i][1];
            double distance = distanceToLine(center, lines[i]);
            if (distance < distanceThreshold) {
                continue; // Skip lines that are within the threshold distance
            }

            double a = cos(theta), b = sin(theta);
            double x0 = a * rho, y0 = b * rho;

            if (abs(a) < abs(b)) { // Vertical lines (left or right)
            }

            if (y0 > center.y) { // Down
                if (distance < minLeftDist) {
                    minLeftDist = distance;
                    nearestLeftLine = lines[i];
                    foundLeft = true;
                }
            }

        }
    }

    // Pair the nearest lines to compute the intersection points
    std::vector<std::pair<cv::Vec2f, cv::Vec2f>> linePairs = {
            {nearestUpLine,   nearestLeftLine},
            {nearestUpLine,   nearestRightLine},
            {nearestDownLine, nearestLeftLine},
            {nearestDownLine, nearestRightLine}
    };

    // Compute the intersections and store them in tableCorners
    for (const auto &linePair: linePairs) {
        if (foundUp && foundLeft && foundRight && foundDown) {
            cv::Point2f intersection = computeIntersection(linePair.first, linePair.second);
            tableCorners_.push_back(intersection);
        }
    }

    cv::Mat result = firstFrame.clone();

// Draw the nearest lines
    if (minUpDist < DBL_MAX) drawLine(nearestUpLine, result, cv::Scalar(0, 0, 255)); // Red for up
    if (minDownDist < DBL_MAX) drawLine(nearestDownLine, result, cv::Scalar(0, 255, 0)); // Green for down
    if (minLeftDist < DBL_MAX) drawLine(nearestLeftLine, result, cv::Scalar(255, 0, 0)); // Blue for left
    if (minRightDist < DBL_MAX) drawLine(nearestRightLine, result, cv::Scalar(255, 255, 0)); // Cyan for right

    for (const cv::Point2f &corner: tableCorners_) {
        cv::circle(result, corner, 5, cv::Scalar(255, 0, 0), -1);

    }

//    cv::imwrite("table_corners.jpg", result);



//    std::cout << "Found up: " << foundUp << " Found down: " << foundDown << " Found left: " << foundLeft
//              << " Found right: " << foundRight << std::endl;

    return true;

}


// Function to compute the intersection point of two lines given in polar coordinates
cv::Point2f TableDetection::computeIntersection(cv::Vec2f line1, cv::Vec2f line2) {
    float rho1 = line1[0], theta1 = line1[1];
    float rho2 = line2[0], theta2 = line2[1];

    float a1 = cos(theta1), b1 = sin(theta1);
    float a2 = cos(theta2), b2 = sin(theta2);

    float x = (b2 * rho1 - b1 * rho2) / (a1 * b2 - a2 * b1);
    float y = (a1 * rho2 - a2 * rho1) / (a1 * b2 - a2 * b1);

    return cv::Point2f(x, y);
}

cv::Mat TableDetection::KMeans(const cv::Mat src) {
    cv::Mat blurred;
    cv::blur(src, blurred, cv::Size(7, 7));
    // Convert the image to the XYZ color space
    cv::cvtColor(src, blurred, cv::COLOR_BGR2XYZ);
    cv::Mat reshaped = blurred.reshape(1, blurred.rows * blurred.cols);
    reshaped.convertTo(reshaped, CV_32F);
    // Apply KMeans clustering to create a mask for the ball
    int k = 2;
    cv::Mat labels, centers;
    cv::kmeans(reshaped, k, labels, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0), 3, cv::KMEANS_PP_CENTERS, centers);

    labels = labels.reshape(0, blurred.rows);
    cv::Mat mask(blurred.size(), CV_8UC1);
    int ballCluster = 0; // Assuming cluster 0 is the ball, you may need to adjust this
    for (int i = 0; i < blurred.rows; i++) {
        for (int j = 0; j < blurred.cols; j++) {
            mask.at<uchar>(i, j) = (labels.at<int>(i, j) == ballCluster) ? 255 : 0;
        }
    }

    cv::Mat result;
    cv::bitwise_and(src, src, result, mask);


    cv::cvtColor(result, result, cv::COLOR_BGR2GRAY);
    cv::threshold(result, result, 1, 255, cv::THRESH_BINARY);

    // calculate the mean color of the ball
    cv::Scalar meanColor = cv::mean(src, mask);
    double norm = cv::norm(meanColor);

    if (norm > 200) cv::threshold(result, result, 1, 255, cv::THRESH_BINARY_INV);

    return result;

}


