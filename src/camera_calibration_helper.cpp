#include <camera_calibration_helper.h>

int CameraCalibrationHelper::calibrateWithImages(std::string path) {
  resetResults();

  std::vector<std::vector<cv::Point3f> > objpoints;
  std::vector<std::vector<cv::Point2f> > imgpoints;

  std::vector<cv::Point3f> objp;
  for(int c = 0; c < checkerboardWidth; c++) {
    for(int r = 0; r < checkerboardHeight; r++){
      objp.push_back(cv::Point3f(r,c,0));
    }
  }

  cv::glob(path, inputImagePaths);

  cv::Mat frame, gray;

  std::vector<cv::Point2f> corner_pts;
  bool success;
  int successfullyProcessedImages = 0;

  for (unsigned int img = 0; img < inputImagePaths.size(); img++) {
    frame = cv::imread(inputImagePaths[img]);

    inputImages.push_back(frame);

    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    success = cv::findChessboardCorners(gray, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

    if(success)
    {
      cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
      cv::cornerSubPix(gray,corner_pts,cv::Size(11,11), cv::Size(-1,-1),criteria);
      cv::drawChessboardCorners(frame, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, success);

      processedImages.push_back(frame);
      processedImagePaths.push_back(inputImagePaths[img]);

      objpoints.push_back(objp);
      imgpoints.push_back(corner_pts);

      successfullyProcessedImages++;
    }
  }

  cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), cameraMatrix, distCoeffs, rotationVectors, translationVectors);

  return successfullyProcessedImages;
}

int CameraCalibrationHelper::calibrateInteractively(std::string path) {
  resetResults();

  return 0;
}
