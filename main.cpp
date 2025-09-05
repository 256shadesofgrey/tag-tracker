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
  double markerLength = 0.1;
  std::vector<double> camMatrixArray = {3529.184800454334, 0, 2040.965768074567, 0, 3514.936017987171, 1126.105514215219, 0, 0, 1};
  std::vector<double> distCoeffsArray = {0.1111941981103543, -1.233444736852835, 0.0004572563505563506, 0.0004007139313956278, 5.054536061947804};

  po::options_description desc("Available options", HELP_LINE_LENGTH, HELP_DESCRIPTION_LENGTH);

  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", po::value<int>()->default_value(0)->implicit_value(1), "Display additional information. Higher value gives additional output.")
    ("source,s", po::value<std::string>()->default_value(videoSource), "Video stream source.")
    ("ww", po::value<int>()->default_value(windowWidth), "Width of the image display windows.")
    ("wh", po::value<int>()->default_value(windowHeight), "Height of the image display windows.")
    ("dict,d", po::value<int>()->default_value(dict), std::format("ArUco dictionary to expect. These are the possible options:\n{}", dictsString()).c_str())
    ("length,l", po::value<double>()->default_value(markerLength), "Size of the marker in meters.")
    ("cm", po::value<std::vector<double> >()->default_value(camMatrixArray, vec2str(camMatrixArray)), "Camera matrix generated through the camera calibration tool.")
    ("dc", po::value<std::vector<double> >()->default_value(distCoeffsArray, vec2str(distCoeffsArray)), "Distortion coefficients generated through the camera calibration tool.")
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

  if (vm.count("length")) {
    markerLength = vm["length"].as<double>();
  }

  if (vm.count("cm")) {
    camMatrixArray = vm["cm"].as<std::vector<double> >();

    if (camMatrixArray.size() != 9) {
      std::cout << "Expected 9 values for camera matrix, but got "<< camMatrixArray.size() << "." << std::endl;
      return 1;
    }
  }

  if (vm.count("dc")) {
    distCoeffsArray = vm["dc"].as<std::vector<double> >();

    if (distCoeffsArray.size() != 5) {
      std::cout << "Expected 5 values for distortion coefficients, but got "<< distCoeffsArray.size() << "." << std::endl;
      return 1;
    }
  }

  if (verbosity > 0) {
    std::cout << "Video source: " << videoSource << std::endl;
    std::cout << "Window width: " << windowWidth << std::endl;
    std::cout << "Window height: " << windowHeight << std::endl;
    std::cout << "Dictionary used: " << dictName(dict) << std::endl;
  }

  cv::VideoCapture cap(videoSource);

  if (!cap.isOpened()) {
    std::cerr << "Error: Could not open IP camera at " << videoSource << "."
              << std::endl;
    return -1;
  }

  cv::namedWindow("Camera Feed", cv::WINDOW_NORMAL);
  cv::namedWindow("Marker Detect", cv::WINDOW_NORMAL);
  cv::waitKey(100);
  cv::resizeWindow("Camera Feed", windowWidth, windowHeight);
  cv::resizeWindow("Marker Detect", windowWidth, windowHeight);

  cv::Mat frameRaw, frameMarkers;

  // Vars for detection.
  std::vector<int> markerIds = {0};
  std::vector<std::vector<cv::Point2f> > markerCorners, rejectedCandidates;
  cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
  cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(dict);
  cv::aruco::ArucoDetector detector(dictionary, detectorParams);

  // Vars for pose estimation.
  cv::Mat camMatrix = cv::Mat(3, 3, CV_64F, camMatrixArray.data());
  cv::Mat distCoeffs = cv::Mat(1, 5, CV_64F, distCoeffsArray.data());

  cv::Mat objPoints(4, 1, CV_32FC3);
  objPoints.ptr<cv::Vec3f>(0)[0] = cv::Vec3f(-markerLength/2.f, markerLength/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[1] = cv::Vec3f(markerLength/2.f, markerLength/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[2] = cv::Vec3f(markerLength/2.f, -markerLength/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[3] = cv::Vec3f(-markerLength/2.f, -markerLength/2.f, 0);

  while (true) {
    cap >> frameRaw;

    if (frameRaw.empty()) {
      std::cerr << "Error: Could not read frame." << std::endl;
      break;
    }

    cv::imshow("Camera Feed", frameRaw);

    // Detect markers and draw them on the output frame.
    detector.detectMarkers(frameRaw, markerCorners, markerIds, rejectedCandidates);
    frameMarkers = frameRaw.clone();
    cv::aruco::drawDetectedMarkers(frameMarkers, markerCorners, markerIds);

    // Estimate pose and draw the axes on the output frame.
    size_t nMarkers = markerCorners.size();
    std::vector<cv::Vec3d> rvecs(nMarkers), tvecs(nMarkers);
    if(!markerIds.empty()) {
      for (size_t i = 0; i < nMarkers; i++) {
        solvePnP(objPoints, markerCorners.at(i), camMatrix, distCoeffs, rvecs.at(i), tvecs.at(i));
      }
    }

    // Draw pose estimation axes to the frame.
    for(unsigned int i = 0; i < markerIds.size(); i++) {
      cv::drawFrameAxes(frameMarkers, camMatrix, distCoeffs, rvecs[i], tvecs[i], markerLength * 0.7f, 2);
    }

    cv::imshow("Marker Detect", frameMarkers);

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
