#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <tag-tracker.h>

class CameraCalibrationHelper {
private:
  const int checkerboardWidth = 0;
  const int checkerboardHeight = 0;

  cv::Mat cameraMatrix;
  cv::Mat distCoeffs;
  cv::Mat rotationVectors;
  cv::Mat translationVectors;
  std::vector<cv::Mat> inputImages;
  std::vector<cv::Mat> processedImages;
  std::vector<cv::String> inputImagePaths;
  std::vector<cv::String> processedImagePaths;

  void resetResults() {
    // inputImages is the first thing to be filled during calibration,
    // so checking that to see if deleting anything is necessary.
    if (inputImages.size() == 0) {
      return;
    }

    cameraMatrix = cv::Mat();
    distCoeffs = cv::Mat();
    rotationVectors = cv::Mat();
    translationVectors = cv::Mat();
    inputImages = std::vector<cv::Mat>();
    processedImages = std::vector<cv::Mat>();
    inputImagePaths = std::vector<cv::String>();
    processedImagePaths = std::vector<cv::String>();
  }

public:
  // Width and height measured in the number of inner corners of the checkerboard pattern.
  // In other words, the number of squares in the respective direction minus 1.
  CameraCalibrationHelper(int checkerboardWidth = 8, int checkerboardHeight = 5) :
    checkerboardWidth(checkerboardWidth), checkerboardHeight(checkerboardHeight) {}

  // path describes the folder containing the images, and their file extension.
  // Return the number of images that were successfully processed.
  // If the number is less than the number of input images, some of them may have been skipped
  // and the calculated calibration results may be less accurate than expected.
  int calibrateWithImages(std::filesystem::path path = "./calibration/*.jpg");
  int calibrateWithImages(const std::vector<cv::Mat>& images);

  // path - describes the folder to save the images to, and their file extension.
  //        If the path is not specified, no images used for calibration will not be saved.
  // Return the number of images that were successfully processed.
  // If the number is less than the number of input images, some of them may have been skipped
  // and the calculated calibration results may be less accurate than expected.
  // If the number is negative, the function failed to open the video source.
  int calibrateInteractively(cv::VideoCapture& videoSource, std::filesystem::path path = "");
  int calibrateInteractively(std::string videoSourceStr = DEFAULT_VIDEO_SOURCE, std::filesystem::path path = "") {
    cv::VideoCapture cap(videoSourceStr);

    if (!cap.isOpened()) {
      std::cerr << "Error: Could not open IP camera at " << videoSourceStr << "."
                << std::endl;
      return -1;
    }

    int processedImgCount = calibrateInteractively(cap, path);

    cap.release();

    return processedImgCount;
  }

  const cv::Mat& getCameraMatrix() {
    return cameraMatrix;
  }

  const cv::Mat& getDistortionCoefficients() {
    return distCoeffs;
  }

  const cv::Mat& getRotationVectors() {
    return rotationVectors;
  }

  const cv::Mat& getTranslationVectors() {
    return translationVectors;
  }

  const std::vector<cv::Mat>& getInputImages() {
    return inputImages;
  }

  const std::vector<cv::Mat>& getProcessedImages() {
    return processedImages;
  }

  const std::vector<cv::String>& getInputImagePaths() {
    return inputImagePaths;
  }

  const std::vector<cv::String>& getProcessedImagePaths() {
    return processedImagePaths;
  }
};
