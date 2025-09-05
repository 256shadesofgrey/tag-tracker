#include <boost/program_options.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <tag-tracker.h>

// Change to 1 to attempt screen size detection for window arrangement.
// Disabled by default because it does not seem to work consistently.
// Use --sw and --sh command line options instead.
#ifndef SCREEN_SIZE_DETECTION
#define SCREEN_SIZE_DETECTION 0
#endif

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;
  std::string path = "../calibration/*.jpg";
  int checkerboardWidth = 8;
  int checkerboardHeight = 5;
  int windowWidth = 960;
  int windowHeight = 540;
  int screenWidth = 3840;
  int screenHeight = 2160;
  int autoarrange = 0;

  po::options_description desc("Available options", HELP_LINE_LENGTH, HELP_DESCRIPTION_LENGTH);

  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", po::value<int>()->default_value(0)->implicit_value(1), "Display additional information. Higher value gives additional output.")
    ("width,W", po::value<int>()->default_value(checkerboardWidth), "Number of inner corners horizontally (i.e. columns-1).")
    ("height,H", po::value<int>()->default_value(checkerboardHeight), "Number of inner corners vertically (i.e. rows-1).")
    ("images,i", po::value<std::string>()->default_value(path), "Folder containing calibration images.")
    ("ww", po::value<int>()->default_value(windowWidth), "Width of the image display windows.")
    ("wh", po::value<int>()->default_value(windowHeight), "Height of the image display windows.")
    ("autoarrange,a", "Arrange windows to optimally fill the screen. This does not work on wayland.")
    ("sw", po::value<int>()->default_value(screenWidth), "Width of the screen.")
    ("sh", po::value<int>()->default_value(screenHeight), "Height of the screen.")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("verbose")) {
    verbosity = vm["verbose"].as<int>();
  }

  if (vm.count("width")) {
    checkerboardWidth = vm["width"].as<int>();
  }

  if (vm.count("height")) {
    checkerboardHeight = vm["height"].as<int>();
  }

  if (vm.count("images")) {
    path = vm["images"].as<std::string>();
  }

  if (vm.count("ww")) {
    windowWidth = vm["ww"].as<int>();
  }

  if (vm.count("wh")) {
    windowHeight = vm["wh"].as<int>();
  }

  autoarrange = vm.count("autoarrange");

  if (vm.count("sw")) {
    screenWidth = vm["sw"].as<int>();
  }

  if (vm.count("sh")) {
    screenHeight = vm["sh"].as<int>();
  }

#if SCREEN_SIZE_DETECTION
  cv::namedWindow("screen", cv::WND_PROP_FULLSCREEN);
  cv::setWindowProperty("screen", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
  cv::waitKey(10);
  cv::Rect screen = cv::getWindowImageRect("screen");
  cv::destroyWindow("screen");

  if (verbosity > 0) {
    std::cout << "Detected screen size: " << screen.width << "x" << screen.height << std::endl;
  }

  if (!vm.count("sw")) {
    screenWidth = screen.width;
  }

  if (!vm.count("sh")) {
    screenHeight = screen.height;
  }
#endif // SCREEN_SIZE_DETECTION

  if (verbosity > 0) {
    std::cout << "Setting width to: " << checkerboardWidth << std::endl;
    std::cout << "Setting height to: " << checkerboardHeight << std::endl;
    std::cout << "Calibration images: " << path << std::endl;
    std::cout << "Window width: " << windowWidth << std::endl;
    std::cout << "Window height: " << windowHeight << std::endl;
    std::cout << "Autoarrange windows: " << autoarrange << std::endl;
    std::cout << "Screen width: " << screenWidth << std::endl;
    std::cout << "Screen height: " << screenHeight << std::endl;
  }

  if (verbosity > 1) {
    std::cout << "Build info: " << std::endl << cv::getBuildInformation() << std::endl;
  }

  std::vector<std::vector<cv::Point3f> > objpoints;
  std::vector<std::vector<cv::Point2f> > imgpoints;

  std::vector<cv::Point3f> objp;
  for(int c = 0; c < checkerboardWidth; c++) {
    for(int r = 0; r < checkerboardHeight; r++){
      objp.push_back(cv::Point3f(r,c,0));
    }
  }

  std::vector<cv::String> images;
  cv::glob(path, images);

  cv::Mat frame, gray;

  std::vector<cv::Point2f> corner_pts;
  bool success;

  int winCols = 0;
  int winRows = 0;

  if (autoarrange) {
    winCols = screenWidth / windowWidth;
    winRows = screenHeight / windowHeight;
  }

  for (unsigned int img = 0; img < images.size(); img++) {
    cv::namedWindow(std::format("Image{}: {}", img, images[img]), cv::WINDOW_NORMAL);
    cv::resizeWindow(std::format("Image{}: {}", img, images[img]), windowWidth, windowHeight);

    frame = cv::imread(images[img]);
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    success = cv::findChessboardCorners(gray, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

    if(success)
    {
      cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
      cv::cornerSubPix(gray,corner_pts,cv::Size(11,11), cv::Size(-1,-1),criteria);
      cv::drawChessboardCorners(frame, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, success);

      objpoints.push_back(objp);
      imgpoints.push_back(corner_pts);
    }

    cv::imshow(std::format("Image{}: {}", img, images[img]), frame);
    cv::waitKey(1);
    cv::resizeWindow(std::format("Image{}: {}", img, images[img]), windowWidth, windowHeight);

    if (autoarrange) {
      cv::moveWindow(std::format("Image{}: {}", img, images[img]), windowWidth * (img%winCols), windowHeight * (img/winCols % winRows));
    }
  }

  cv::waitKey(0);

  cv::destroyAllWindows();

  cv::Mat cameraMatrix,distCoeffs,R,T;

  cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), cameraMatrix, distCoeffs, R, T);

  std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
  std::cout << "distCoeffs : " << distCoeffs << std::endl;

  if (verbosity > 0) {
    std::cout << "Rotation vector : " << R << std::endl;
    std::cout << "Translation vector : " << T << std::endl;
  }

  return 0;
}
