#include "../include/header.h"
#include "../include/BallDetection.h"




int main(int argc, char** argv ) {


    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " < Input video path > " << " < Output video path > " << std::endl;
        return -1;

    }

    BallDetection bd;

    if (!bd.process_video(argv[1], argv[2])) {
        std::cerr << "Error: Could not process video" << std::endl;
        return -1;
    }


    return 0;
}