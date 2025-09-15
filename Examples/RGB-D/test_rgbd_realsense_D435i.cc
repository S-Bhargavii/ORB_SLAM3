#include <iostream>
#include <librealsense2/rs.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <System.h>

using namespace std;

int main(int argc, char **argv)
{
    if(argc < 3) {
        cerr << "Usage: ./rgbd_realsense path_to_vocabulary path_to_settings (optional_trajectory_file)" << endl;
        return -1;
    }

    string file_name;
    bool bFileName = false;
    if(argc == 4){
        file_name = argv[3];
        bFileName = true;
    }

    // Initialize ORB-SLAM3 system
    cout << "Initializing ORB-SLAM3 RGB-D system..." << endl;
    ORB_SLAM3::System SLAM(argv[1], argv[2], ORB_SLAM3::System::RGBD, true, 0, file_name);
    float imageScale = SLAM.GetImageScale();
    cout << "ORB-SLAM3 ready." << endl;

    // RealSense pipeline
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);
    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);

    rs2::pipeline_profile pipe_profile = pipe.start(cfg);

    // Align depth to color
    rs2::align align(RS2_STREAM_COLOR);

    // Get intrinsics
    auto cam_stream = pipe_profile.get_stream(RS2_STREAM_COLOR);
    auto intrinsics_cam = cam_stream.as<rs2::video_stream_profile>().get_intrinsics();
    int width_img = intrinsics_cam.width;
    int height_img = intrinsics_cam.height;

    cv::Mat im, depth;

    while(!SLAM.isShutDown())
    {
        // Wait for a new set of frames
        rs2::frameset fs = pipe.wait_for_frames();

        // Align depth to color
        fs = align.process(fs);

        rs2::video_frame color_frame = fs.get_color_frame();
        rs2::depth_frame depth_frame = fs.get_depth_frame();

        // Convert to OpenCV
        im = cv::Mat(cv::Size(width_img, height_img), CV_8UC3, (void*)(color_frame.get_data()), cv::Mat::AUTO_STEP);
        depth = cv::Mat(cv::Size(width_img, height_img), CV_16U, (void*)(depth_frame.get_data()), cv::Mat::AUTO_STEP);

        if(imageScale != 1.f){
            int w = im.cols * imageScale;
            int h = im.rows * imageScale;
            cv::resize(im, im, cv::Size(w,h));
            cv::resize(depth, depth, cv::Size(w,h));
        }

        // Feed RGB-D to ORB-SLAM3
        SLAM.TrackRGBD(im, depth, color_frame.get_timestamp() * 1e-3);
    }

    cout << "System shutdown!" << endl;
    SLAM.Shutdown();
    pipe.stop();

    return 0;
}
