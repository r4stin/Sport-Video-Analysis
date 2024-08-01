/*
 * File:    findCenters.cpp
 * Author:  GIACOMO D'ANDRIA
 * Date:    July 17, 2024
 * Description: This file contains the implementation of the findCenters class.
 *             The findCenters class is used to find the centers of the balls in the image.
 *             The findCenter method takes an image as input and returns a vector of points
 *             representing the centers of the balls.
 */

#include "../include/BallDetection.h"

findCenters::findCenters(BallDetection *ballDetection1) : ballDetection_(ballDetection1) {}
std::vector<cv::Point2f> findCenters::findCenter(cv::Mat img) {
    // Convert the image to grayscale
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    // Apply Median Blur to reduce noise
    cv::medianBlur(gray, gray, 1);
    // Apply Hough Circle Transform
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, gray.rows / 16, 107, 10, 5, 12);

    std::vector<cv::Point2f> centers;

    // Create a black image to draw white circles
    cv::Mat mask = cv::Mat::zeros(img.size(), CV_8UC1);
    // Draw the circles
    for (size_t i = 0; i < circles.size(); i++) {
        cv::Vec3f c = circles[i];
        cv::Point2f center = cv::Point2f(c[0], c[1]);
        centers.push_back(center);

        float radius = c[2];

        // Draw circle center
//        cv::circle(mask, center, 1, cv::Scalar(255), 4, cv::LINE_AA);
//        // Draw circle outline
//        cv::circle(mask, center, radius, cv::Scalar(255), 3, cv::LINE_AA);
        // Draw and fill the circle
        cv::circle(mask, center, radius, cv::Scalar(255), -1, cv::LINE_AA);

    }

    return centers;
}