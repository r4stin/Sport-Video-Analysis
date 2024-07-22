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