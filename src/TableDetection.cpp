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
