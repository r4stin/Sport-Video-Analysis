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