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

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;
  std::string path = "../calibration/*.jpg";
  int checkerboardWidth = 8;
  int checkerboardHeight = 5;

  po::options_description desc("Available options");

  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", "Display additional information.")
    ("width,W", po::value<int>(), std::format("Number of inner corners horizontally (i.e. columns-1). (Default: {})", checkerboardWidth).c_str())
    ("height,H", po::value<int>(), std::format("Number of inner corners vertically (i.e. rows-1). (Default: {})", checkerboardHeight).c_str())
    ("images,i", po::value<std::string>(), std::format("Output folder. (Default: {})", path).c_str())
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  verbosity = vm.count("verbose");

  if (vm.count("width")) {
    checkerboardWidth = vm["width"].as<int>();
  }

  if (vm.count("height")) {
    checkerboardHeight = vm["height"].as<int>();
  }

  if (vm.count("images")) {
    path = vm["images"].as<std::string>();
  }

  if (verbosity > 0) {
    std::cout << "Setting width to: " << checkerboardWidth << std::endl;
    std::cout << "Setting height to: " << checkerboardHeight << std::endl;
    std::cout << "Calibration images: " << path << std::endl;
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

  cv::namedWindow("Image", cv::WINDOW_NORMAL);

  for (unsigned int img = 0; img < images.size(); img++) {
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

    cv::imshow("Image", frame);
    cv::waitKey(0);
  }

  cv::destroyAllWindows();

  cv::Mat cameraMatrix,distCoeffs,R,T;

  cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), cameraMatrix, distCoeffs, R, T);

  std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
  std::cout << "distCoeffs : " << distCoeffs << std::endl;
  std::cout << "Rotation vector : " << R << std::endl;
  std::cout << "Translation vector : " << T << std::endl;

  return 0;
}
