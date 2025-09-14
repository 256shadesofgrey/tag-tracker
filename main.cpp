#include <boost/program_options.hpp>
#include <iostream>
#include <format>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

#include <tag-tracker.h>
#include <camera_calibration_helper.h>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;
  std::string videoSource = DEFAULT_VIDEO_SOURCE;
  int windowWidth = 1920;
  int windowHeight = 1080;
  cv::aruco::PredefinedDictionaryType dict = cv::aruco::DICT_6X6_250;
  double markerLength = 0.1;
  std::vector<double> camMatrixArray = {3529.184800454334, 0, 2040.965768074567, 0, 3514.936017987171, 1126.105514215219, 0, 0, 1};
  std::vector<double> distCoeffsArray = {0.1111941981103543, -1.233444736852835, 0.0004572563505563506, 0.0004007139313956278, 5.054536061947804};
  std::string calibrationFile = "calibration.txt";
  bool useCalFileCamMat = true;
  bool useCalFileDistCoeffs = true;

  std::string path = "";
  int checkerboardWidth = 8;
  int checkerboardHeight = 5;
  bool interactiveCalibration = false;
  bool calibration = false;
  bool saveCalFile = false;

  po::options_description desc("Available options", HELP_LINE_LENGTH, HELP_DESCRIPTION_LENGTH);

  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", po::value<int>()->default_value(0)->implicit_value(1), "Display additional information. Higher value gives additional output.")
    ("source,s", po::value<std::string>()->default_value(videoSource), "Video stream source.")
    ("ww", po::value<int>()->default_value(windowWidth), "Width of the image display windows.")
    ("wh", po::value<int>()->default_value(windowHeight), "Height of the image display windows.")
    ("dict,d", po::value<int>()->default_value(dict), std::format("ArUco dictionary to expect. These are the possible options:\n{}", dictsString()).c_str())
    ("length,l", po::value<double>()->default_value(markerLength), "Size of the marker in meters.")
    ("cm", po::value<std::vector<double> >()->default_value(camMatrixArray, vec2str(camMatrixArray)), "Camera matrix generated through the camera calibration tool. "
                                                                                                      "The default value is overridden if the program detects a calibration file. "
                                                                                                      "If this is set explicitly, the calibration file is ignored even if it exists.")
    ("dc", po::value<std::vector<double> >()->default_value(distCoeffsArray, vec2str(distCoeffsArray)), "Distortion coefficients generated through the camera calibration tool. "
                                                                                                        "The default value is overridden if the program detects a calibration file. "
                                                                                                        "If this is set explicitly, the calibration file is ignored even if it exists.")
    ("calibration-file,c", po::value<std::string>()->default_value(calibrationFile)->implicit_value(calibrationFile), "The file to attempt to read calibration values from. If this file does not exist, and cm and dc are not set, use default values for cm and dc. "
                                                                                                                      "If this parameter is explicitly set and calibration is performed, it will be used as output file for the calibration values.")
    ("calibration-images,i", po::value<std::string>()->default_value(path)->implicit_value("./calibration/*.jpg"), "Folder containing calibration images. If it is set, "
                                                                                                                   "calibration will be done with images matching the pattern. This overrides the cm and dc options. "
                                                                                                                   "This will be used as output folder (and file extension) instead if using interactive calibration.")
    ("width,W", po::value<int>()->default_value(checkerboardWidth), "Number of inner corners horizontally (i.e. columns-1).")
    ("height,H", po::value<int>()->default_value(checkerboardHeight), "Number of inner corners vertically (i.e. rows-1).")
    ("ic", "Does interactive calibration before starting to track the markers. You will have to point the camera at the chessboard pattern from different positions. This overrides the cm and dc options.")
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

  if (vm.count("calibration-file")) {
    if (!vm["calibration-file"].defaulted()) {
      saveCalFile = true;
    }
    calibrationFile = vm["calibration-file"].as<std::string>();
  }

  if (vm.count("cm")) {
    if (!vm["cm"].defaulted()) {
      useCalFileCamMat = false;
    }

    camMatrixArray = vm["cm"].as<std::vector<double> >();

    if (camMatrixArray.size() != 9) {
      std::cout << "Expected 9 values for camera matrix, but got "<< camMatrixArray.size() << "." << std::endl;
      return 1;
    }
  }

  if (vm.count("dc")) {
    if (!vm["dc"].defaulted()) {
      useCalFileDistCoeffs = false;
    }

    distCoeffsArray = vm["dc"].as<std::vector<double> >();

    if (distCoeffsArray.size() != 5) {
      std::cout << "Expected 5 values for distortion coefficients, but got "<< distCoeffsArray.size() << "." << std::endl;
      return 1;
    }
  }

  // Attempt to read calibration file if it exists and either of the values from it are not set explicitly.
  std::filesystem::path cfp = std::filesystem::path(calibrationFile);
  if (std::filesystem::exists(cfp) && (useCalFileCamMat || useCalFileDistCoeffs)) {
    bool dataValid = true;

    std::ifstream cfs(cfp);
    if (!cfs.is_open()) {
      std::cout << "Could not open calibration file. Using default values for parameters not explicitly set." << std::endl;
    } else {
      std::vector<double> calVals[2];

      std::string line;
      for (int i = 0; i < 2 && dataValid; i++) {
        if (!std::getline(cfs, line)) {
          std::cout << "Incorrect file format for the calibration file. Using default values for parameters not explicitly set." << std::endl;
          dataValid = false;
          break;
        }

        // Remove the {} brackets.
        line = line.substr(1, line.length()-2);
        // Read all values of a line as doubles.
        std::istringstream lineStream(line);
        std::string stringNum;
        while (std::getline(lineStream, stringNum, ',')) {
          double num;

          try {
            num = std::stod(stringNum);
          } catch(const std::exception&) {
            std::cout << "Incorrect number format in the calibration file. Using default values for parameters not explicitly set." << std::endl;
            dataValid = false;
            break;
          }

          calVals[i].push_back(num);
        }
      }

      if (dataValid) {
        camMatrixArray = calVals[0];
        distCoeffsArray = calVals[1];

        if (verbosity > 2) {
          std::cout << "Camera matrix from file: " << vec2str(camMatrixArray) << std::endl;
          std::cout << "Distortion coefficients from file: " << vec2str(distCoeffsArray) << std::endl;
        }
      }

      cfs.close();
    }
  }

  if (vm.count("calibration-images")) {
    path = vm["calibration-images"].as<std::string>();
    if (!vm["calibration-images"].defaulted()) {
      calibration = true;
    }
  }

  if (vm.count("width")) {
    checkerboardWidth = vm["width"].as<int>();
  }

  if (vm.count("height")) {
    checkerboardHeight = vm["height"].as<int>();
  }

  if (vm.count("ic")) {
    calibration = true;
    interactiveCalibration = true;
  }

  cv::VideoCapture cap(videoSource);

  if (!cap.isOpened()) {
    std::cerr << "Error: Could not open IP camera at " << videoSource << "."
              << std::endl;
    return -1;
  }

  // Setting the camera calibration values to the values read from the file, or the parameters.
  cv::Mat camMatrix = cv::Mat(3, 3, CV_64F, camMatrixArray.data());
  cv::Mat distCoeffs = cv::Mat(1, 5, CV_64F, distCoeffsArray.data());

  if (calibration) {
    CameraCalibrationHelper cch(checkerboardWidth, checkerboardHeight);

    if (interactiveCalibration) {
      cch.calibrateInteractively(cap, path);
    } else {
      cch.calibrateWithImages(path);
    }

    // Setting camera calibration values to the ones just determined, overwriting the ones from the config file or program options.
    camMatrix = cch.getCameraMatrix();
    distCoeffs = cch.getDistortionCoefficients();

    if (saveCalFile) {
      saveCalibrationFile(calibrationFile, camMatrix, distCoeffs);
    }
  }

  if (verbosity > 1) {
    std::cout << "Final camera matrix: " << vec2str(camMatrixArray) << std::endl;
    std::cout << "Final distortion coefficients: " << vec2str(distCoeffsArray) << std::endl;
  }

  cv::namedWindow("Marker Detect", cv::WINDOW_NORMAL);
  cv::waitKey(100);
  cv::resizeWindow("Marker Detect", windowWidth, windowHeight);

  cv::Mat frameRaw, frameMarkers;

  // Vars for detection.
  std::vector<int> markerIds = {0};
  std::vector<std::vector<cv::Point2f> > markerCorners, rejectedCandidates;
  cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
  cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(dict);
  cv::aruco::ArucoDetector detector(dictionary, detectorParams);

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

    // Detect markers and draw them on the output frame.
    detector.detectMarkers(frameRaw, markerCorners, markerIds, rejectedCandidates);
    frameMarkers = frameRaw.clone();
    cv::aruco::drawDetectedMarkers(frameMarkers, markerCorners, markerIds);

    for (unsigned int i = 0; i < markerCorners.size() && verbosity > 2; i++) {
      std::cout << "Corners for marker id=" << markerIds.at(i) << ":\n" << markerCorners.at(i) << std::endl;
    }

    // Estimate pose.
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

    // Write marker position under the marker.
    for (unsigned int i = 0; i < markerCorners.size(); i++) {
      // Bottom left corner of the marker.
      cv::Point2f textStart = markerCorners.at(i).at(3);
      // Text reference point is bottom left, and we want it to be top left, so offset origin by font height.
      textStart.y += TEXT_SCALE * FONT_HEIGHT;

      cv::putText(frameMarkers, "X: " + std::to_string(tvecs.at(i)[0]), textStart, cv::FONT_HERSHEY_SIMPLEX, TEXT_SCALE, RED, TEXT_LINE_THICKNESS, cv::LINE_AA);
      textStart.y += TEXT_SCALE * FONT_HEIGHT;
      cv::putText(frameMarkers, "Y: " + std::to_string(tvecs.at(i)[1]), textStart, cv::FONT_HERSHEY_SIMPLEX, TEXT_SCALE, GREEN, TEXT_LINE_THICKNESS, cv::LINE_AA);
      textStart.y += TEXT_SCALE * FONT_HEIGHT;
      cv::putText(frameMarkers, "Z: " + std::to_string(tvecs.at(i)[2]), textStart, cv::FONT_HERSHEY_SIMPLEX, TEXT_SCALE, BLUE, TEXT_LINE_THICKNESS, cv::LINE_AA);
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
