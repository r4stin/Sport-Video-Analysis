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

// Function to transform a point using a perspective transformation matrix
cv::Point2f BallDetection::transformPoint(const cv::Point2f& point, const cv::Mat& transformMatrix) {
    std::vector<cv::Point2f> src(1, point);
    std::vector<cv::Point2f> dst(1);
    perspectiveTransform(src, dst, transformMatrix);
    return dst[0];
}


// Function to create the table
cv::Mat BallDetection::create_table(int width, int height) {
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255, 255, 255)); // create 2D table image
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

    return img;
}



// Function to draw balls on the table
cv::Mat BallDetection::draw_balls(const std::vector<cv::Point2f>& minimapBallPositions, const cv::Mat& background, int radius = 7, int size = -1, const cv::Mat& img = cv::Mat()) {
    cv::Mat final = background.clone(); // canvas
    std::vector<double> l2_norms(minimapBallPositions.size(), 0.0);

    // Calculate the mean color and L2 norm for each position
    for (size_t i = 0; i < minimapBallPositions.size(); ++i) {
        cv::Point2f position = minimapBallPositions[i];

        float cX = position.x;
        float cY = position.y;

        cv::Mat mask = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::circle(mask, cv::Point2f(cX, cY), radius_[i], cv::Scalar(255), -1);

        cv::Scalar meanColor = cv::mean(img, mask);

        // Calculate L2 norm of the mean color
        double l2_norm = cv::norm(meanColor);
        l2_norms[i] = l2_norm;
    }

    // Find the min and max L2 norm values
    auto minmax = std::minmax_element(l2_norms.begin(), l2_norms.end());
    double min_val = *minmax.first;
    double max_val = *minmax.second;

    // Draw the balls with assigned colors based on L2 norms
    for (size_t i = 0; i < minimapBallPositions.size(); ++i) {
//        cv::Point2f position = minimapBallPositions[i];
        cv::Point2f position = minimapBallPositions[i];
        int cX = static_cast<int>(position.x);
        int cY = static_cast<int>(position.y);

        cv::Scalar color;

        if (l2_norms[i] == max_val) {
            color = cv::Scalar(255, 255, 255); // White color for max L2 norm
        } else if (l2_norms[i] == min_val) {
            color = cv::Scalar(0, 0, 0); // Black color for min L2 norm
        } else if (l2_norms[i] < max_val && l2_norms[i] > 200) {
            color = cv::Scalar(255, 0, 0); // Blue color for L2 norm > 200

        } else if (l2_norms[i] < 200 && l2_norms[i] > min_val) {
            color = cv::Scalar(0, 0, 255); // Red color for L2 norm < 200

        } else {
            std::cout << "No color detected" << std::endl;
            continue;
        }
        // Store the points to for tracking
        points_.push_back(cv::Point2f(cX, cY));

        for (const auto& pt : points_) {
            cv::circle(final, pt, 2, cv::Scalar(0, 0, 0), -1);
        }
        // Draw the ball
        cv::circle(final, cv::Point(cX, cY), radius, color, size);

        // Add black color around the drawn ball (for cosmetics)
        cv::circle(final, cv::Point(cX, cY), radius, cv::Scalar(0), 2);

        // Small circle for light reflection
        cv::circle(final, cv::Point(cX - 2, cY - 2), 4, cv::Scalar(255, 255, 255), -1);

    }

    return final;
}



// Function to draw holes on the table
cv::Mat BallDetection::draw_holes(const cv::Mat& input_img) {
    cv::Scalar color(190, 190, 190);
    cv::Scalar color2(0, 0, 0);

    cv::Mat img = input_img.clone();

    int width = width_;
    int height = height_;



    cv::line(img, cv::Point(0, 0), cv::Point(width, 0), color2, 3); // top
    cv::line(img, cv::Point(0, height), cv::Point(width, height), color2, 3); // bottom
    cv::line(img, cv::Point(0, 0), cv::Point(0, height), color2, 3); // left
    cv::line(img, cv::Point(width, 0), cv::Point(width, height), color2, 3); // right

    int offset = 10;

    cv::line(img, cv::Point(0, offset), cv::Point(width, offset), color2, 3); // top offset
    cv::line(img, cv::Point(0, height - offset), cv::Point(width, height - offset), color2, 3); // bottom offset
    cv::line(img, cv::Point(offset, 0), cv::Point(offset, height), color2, 3); // left offset
    cv::line(img, cv::Point(width - offset, 0), cv::Point(width - offset, height), color2, 3); // right offset


    cv::circle(img, cv::Point(0, 0), 25, color, -1); // top left
    cv::circle(img, cv::Point(width, 0), 25, color, -1); // top right
    cv::circle(img, cv::Point(0, height), 25, color, -1); // bottom left
    cv::circle(img, cv::Point(width, height), 25, color, -1); // bottom right
    cv::circle(img, cv::Point(width, height / 2), 17, color, -1); // mid right
    cv::circle(img, cv::Point(0, height / 2), 17, color, -1); // mid left

    cv::circle(img, cv::Point(width, 0), 17, color2, -1); // top right
    cv::circle(img, cv::Point(0, 0), 17, color2, -1); // top left
    cv::circle(img, cv::Point(0, height), 17, color2, -1); // bottom left
    cv::circle(img, cv::Point(width, height), 17, color2, -1); // bottom right
    cv::circle(img, cv::Point(width, height / 2), 13, color2, -1); // mid right
    cv::circle(img, cv::Point(0, height / 2), 13, color2, -1); // mid left
//    cv::imwrite("table.png", img);
    return img;

}


bool BallDetection::centerRefinement(cv::Mat img){

    // access to friend class
    for (auto & i : centers_) {

        float cX = i.x;
        float cY = i.y;

        int radius1 = 30;
        cv::Mat mask1 = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::circle(mask1, cv::Point2f(cX, cY), radius1, cv::Scalar(255), -1);

        cv::Mat circle_mask;
        cv::bitwise_and(img, img, circle_mask, mask1);


        // split the channels
        std::vector<cv::Mat> channels;
        cv::split(circle_mask, channels);

        // only use the red
        cv::Mat red = channels[2];

        cv::Mat gray;
        cv::cvtColor(circle_mask, gray, cv::COLOR_BGR2GRAY);

        // Apply Hough Circle Transform
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, gray.rows / 16, 107, 10, 5, 15);
        if (circles.empty()) {
            cv::HoughCircles(red, circles, cv::HOUGH_GRADIENT, 1, gray.rows / 16, 107, 10, 5, 15);
        }

        if (circles.empty()) {
            std::cerr << "Error: No circles detected!" << std::endl;
            return false;
        }

        // Create a black image to draw white circles
        cv::Mat mask = cv::Mat::zeros(img.size(), CV_8UC1);
        // Draw the circles
        for (auto c : circles) {
            cv::Point2f center = cv::Point2f(c[0], c[1]);
            float radius = c[2];
            if (radius < 6.5) radius = 7.1;
            // Draw and fill the circle
            cv::circle(mask, center, radius, cv::Scalar(255), -1, cv::LINE_AA);
            radius_.push_back(radius + 2);
            centers_ref_.push_back(center);

        }
    }

    return true;
}


// Function to segment the image and produce outputs
bool BallDetection::outputGenerator(const std::vector<cv::Point2f>& ballPositions, const cv::Mat& img, int radius, const cv::Mat& mask_table, const cv::Mat& bb_table, const std::string& filename) {
    if (ballPositions.empty()) {
        std::cerr << "Error: No ball positions detected!" << std::endl;
        return false;
    }
    cv::Mat rec_table = img.clone();
    std::vector<int> labels;
    std::vector<cv::Rect> boundingBoxes;

    // Define the colors for each category
    cv::Scalar whiteColor(1, 1, 1);
    cv::Scalar blackColor(2, 2, 2);
    cv::Scalar solidColor(3, 3, 3);
    cv::Scalar stripedColor(4, 4, 4);

    // Calculate the mean color and L2 norm for each position
    std::vector<double> l2_norms(ballPositions.size(), 0.0);
    for (size_t i = 0; i < ballPositions.size(); ++i) {

        float cX = ballPositions[i].x;
        float cY = ballPositions[i].y;


        cv::Mat mask = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::circle(mask, cv::Point2f(cX, cY), radius_[i], cv::Scalar(255), -1);


        cv::Scalar meanColor = cv::mean(img, mask);

        // Calculate L2 norm of the mean color
        double l2_norm = cv::norm(meanColor);
        l2_norms[i] = l2_norm;

    }

    // Find the min and max L2 norm values
    auto minmax = std::minmax_element(l2_norms.begin(), l2_norms.end());
    double min_val = *minmax.first;
    double max_val = *minmax.second;

    // Draw the balls with assigned colors based on L2 norms
    for (size_t i = 0; i < ballPositions.size(); ++i) {

        float cX = centers_ref_[i].x;
        float cY = centers_ref_[i].y;


        cv::Scalar color_mask, color_bb;
        int label;

        if (l2_norms[i] == max_val) {
            color_mask = whiteColor; // White ball for max L2 norm
            color_bb = cv::Scalar(255, 255, 255); // White border for max L2 norm
            label = 1;
        } else if (l2_norms[i] == min_val) {
            color_mask = blackColor; // Black ball for min L2 norm
            color_bb = cv::Scalar(0, 0, 0); // Black border for min L2 norm
            label = 2;
        } else if (l2_norms[i] < max_val && l2_norms[i] > 200) {
            color_mask = stripedColor; // Striped ball for L2 norm > 200
            color_bb = cv::Scalar(255, 0, 0); //Blue border for L2 norm > 200
            label = 4;
        } else if (l2_norms[i] < 200 && l2_norms[i] > min_val) {
            color_mask = solidColor; // Solid ball for L2 norm <= 200
            color_bb = cv::Scalar(0, 0, 255); // Red border for L2 norm <= 200
            label = 3;
        } else {
            continue;
        }

        // Draw the ball on the segmentation mask
        cv::circle(mask_table, cv::Point2f(cX, cY), radius_[i], color_mask, -1);
        // Draw the ball on the bounding box mask
        cv::circle(bb_table, cv::Point2f(cX, cY), radius_[i], color_bb, -1);

        cv::Rect2f rect(cX - radius_[i], cY - radius_[i], 2.0 * radius_[i], 2.0 * radius_[i]);

        // Draw the rectangle on the output image
        cv::rectangle(rec_table, rect, color_bb, 2);


        labels.push_back(label);
        boundingBoxes.push_back(rect);

    }
    // Define output image names by adding a name into filename

    std::string mask_table_name = filename + "_mask_table.png";
    std::string bb_output_name = filename + "_bb.txt";
    std::string rec_table_name = filename + "_output1.png";
    std::string bb_table_name = filename + "_output2.png";

    cv::imwrite(mask_table_name, mask_table);
    cv::imwrite(rec_table_name, rec_table);
    cv::imwrite(bb_table_name, bb_table);


    saveDetections(bb_output_name, ballPositions, labels, boundingBoxes);


    return true;
}


void BallDetection::saveDetections(const std::string& filename, const std::vector<cv::Point2f>& centers, const std::vector<int>& labels, const std::vector<cv::Rect>& boundingBoxes) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to to save outputs" << std::endl;
        return;
    }

    // sort according to labels
    std::vector<std::pair<int, std::pair<cv::Point2f, cv::Rect>>> sorted;
    for (size_t i = 0; i < centers.size(); ++i) {
        sorted.push_back({labels[i], {centers[i], boundingBoxes[i]}});
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    // Write the data to the file
    for (auto & i : sorted) {
        const auto& data = i.second;
        const auto& center = data.first;
        const auto& box = data.second;
        int x = box.x;
        int y = box.y;
        int width = box.width;
        int height = box.height;
        int label = i.first;

        file << x << " " << y << " " << width << " " << height << " " << label << std::endl;
    }

    file.close();
}


bool BallDetection::createTopViewMinimap(const std::vector<cv::Point2f>& ballPositions, const cv::Mat& img, const std::vector<cv::Point2f>& tableCorners) {
    cv::Mat output = cv::Mat::zeros(img.size(), CV_8UC1); // Create a black image

    std::vector<cv::Point2f> pts2;
    pts2.emplace_back(0, 0);                                      // top left
    pts2.emplace_back(static_cast<float>(width_ - 1), 0);         // top right
    pts2.emplace_back(0, static_cast<float>(height_ - 1));        // bottom left
    pts2.emplace_back(static_cast<float>(width_ - 1), height_ - 1); // bottom right

    // Calculate the perspective transformation matrix
    cv::Mat transformMatrix = cv::getPerspectiveTransform(tableCorners, pts2);

    // Transform ball positions to the minimap
    std::vector<cv::Point2f> minimapBallPositions;
    for (const auto& pos : ballPositions) {
        minimapBallPositions.push_back(transformPoint(pos, transformMatrix));
    }
    // Warp the image to the minimap
    cv::Mat minimap;
    cv::warpPerspective(img, minimap, transformMatrix, cv::Size(width_, height_));
    // Create the table
    cv::Mat background = create_table(width_, height_);
    // Draw the balls on the minimap
    cv::Mat final = draw_balls(minimapBallPositions, background, 12, -1, minimap);
    // Draw the holes on the table
    top_view_ = draw_holes(final);
    if (top_view_.empty()) {
        std::cerr << "Error: Could not create the minimap" << std::endl;
        return false;
    }

    return true;

}


bool compareY(const cv::Point2f &a, const cv::Point2f &b) {
    return a.y < b.y;
}

bool compareX(const cv::Point2f &a, const cv::Point2f &b) {
    return a.x < b.x;
}
// Function to sort the corners of the table
std::vector<cv::Point2f> sortCorners(const std::vector<cv::Point2f>& corners) {
    // Ensure there are exactly 4 points
    if (corners.size() != 4) {
        throw std::runtime_error("There must be exactly 4 points to sort.");
    }

    // Sort the points by y-coordinate
    std::vector<cv::Point2f> sortedCorners = corners;
    std::sort(sortedCorners.begin(), sortedCorners.end(), compareY);

    // The top-most points will be the first two in the sorted list
    std::vector<cv::Point2f> topMost(sortedCorners.begin(), sortedCorners.begin() + 2);
    // The bottom-most points will be the last two in the sorted list
    std::vector<cv::Point2f> bottomMost(sortedCorners.begin() + 2, sortedCorners.end());

    // Sort top-most points by x-coordinate to get top-left and top-right
    std::sort(topMost.begin(), topMost.end(), compareX);
    cv::Point2f topLeft = topMost[0];
    cv::Point2f topRight = topMost[1];

    // Sort bottom-most points by x-coordinate to get bottom-left and bottom-right
    std::sort(bottomMost.begin(), bottomMost.end(), compareX);
    cv::Point2f bottomLeft = bottomMost[0];
    cv::Point2f bottomRight = bottomMost[1];

    // Return the points in the order: top-left, top-right, bottom-right, bottom-left
    return {topLeft, topRight, bottomRight, bottomLeft};
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
        if (!centerRefinement(frame)){
            std::cerr << "Error: Could not refine the circles" << std::endl;
            return false;
        }
        // Create the minimap
        if (!createTopViewMinimap(centers_ref_, frame, vp.tableCorners_)) {
            std::cerr << "Error: Could not create the minimap" << std::endl;
            return false;
        }

        // Generate outputs only on first frame and last frame
        if (frame_num == 0){
//            cv::imwrite("first_frame.png", frame);
            if (!outputGenerator(centers_ref_, frame, 10, black, green, "first")) {
                std::cerr << "Error: Could not segment the image" << std::endl;
                return false;
            }

        } else if (frame_num == total_frames - 2){
            cv::imwrite("final_2d.png", top_view_);
            if (!outputGenerator(centers_ref_, frame, 10, black, green, "last")) {
                std::cerr << "Error: Could not segment the image" << std::endl;
                return false;
            }

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
