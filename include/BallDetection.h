#ifndef BALLDETECTION_H
#define BALLDETECTION_H
#include "header.h"

class BallDetection {

public:
    BallDetection();
    bool process_video(const std::string& input_path,const std::string& output_path);









private:



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
