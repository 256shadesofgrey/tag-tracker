#include <boost/program_options.hpp>
#include <iostream>
#include <format>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

#include <tag-tracker.h>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;
  std::string videoSource = "http://192.168.178.10:8080/video";
  int windowWidth = 1920;
  int windowHeight = 1080;
  cv::aruco::PredefinedDictionaryType dict = cv::aruco::DICT_6X6_250;
  std::vector<int> markerIds = {0};

  po::options_description desc("Available options", 1024);

  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", po::value<int>()->default_value(0)->implicit_value(1), "Display additional information. Higher value gives additional output.")
    ("source,s", po::value<std::string>()->default_value(videoSource), "Video stream source.")
    ("ww", po::value<int>()->default_value(windowWidth), "Width of the image display windows.")
    ("wh", po::value<int>()->default_value(windowHeight), "Height of the image display windows.")
    ("dict,d", po::value<int>()->default_value(dict), std::format("ArUco dictionary to expect. These are the possible options:\n{}", dictsString()).c_str())
    ("id,i", po::value<std::vector<int>>()->default_value(markerIds, vec2str(markerIds)), "List of IDs encoded in the tracked marker.")
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

  if (vm.count("ww")) {
    windowWidth = vm["ww"].as<int>();
  }

  if (vm.count("wh")) {
    windowHeight = vm["wh"].as<int>();
  }

  if (vm.count("dict")) {
    dict = (cv::aruco::PredefinedDictionaryType)vm["dict"].as<int>();
  }

  if (vm.count("id")) {
    markerIds = vm["id"].as<std::vector<int>>();
  }

  if (verbosity > 0) {
    std::cout << "Video source: " << videoSource << std::endl;
    std::cout << "Window width: " << windowWidth << std::endl;
    std::cout << "Window height: " << windowHeight << std::endl;
    std::cout << "Dictionary used: " << dictName(dict) << std::endl;
    std::cout << "Marker IDs tracked: " << vec2str(markerIds) << std::endl;
  }

  cv::VideoCapture cap(videoSource);

  if (!cap.isOpened()) {
    std::cerr << "Error: Could not open IP camera at " << videoSource << "."
              << std::endl;
    return -1;
  }

  cv::namedWindow("Camera Feed", cv::WINDOW_NORMAL);
  cv::namedWindow("Edge detection", cv::WINDOW_NORMAL);
  cv::waitKey(100);
  cv::resizeWindow("Camera Feed", windowWidth, windowHeight);
  cv::resizeWindow("Edge detection", windowWidth, windowHeight);

  cv::Mat frame, frame_blur, frame_canny;

  while (true) {
    cap >> frame;

    if (frame.empty()) {
      std::cerr << "Error: Could not read frame." << std::endl;
      break;
    }

    cv::imshow("Camera Feed", frame);

    // Blur image to reduce noise.
    cv::GaussianBlur(frame, frame_blur, cv::Size(3, 3), 3, 0);

    // Edge detection.
    cv::Canny(frame_blur, frame_canny, 50, 150);

    cv::imshow("Edge detection", frame_canny);

    // Wait for X milliseconds. If a key is pressed, break from the loop.
    if (cv::waitKey(1) >= 0) {
      break;
    }
  }

  // Release the VideoCapture object and close all windows
  cap.release();
  cv::destroyAllWindows();

  return 0;
}
