#include <camera_calibration_helper.h>

#include <iostream>

#define DEFAULT_CALIBRATION_IMAGE_COUNT 9
#define PROCESSED_IMAGE_FILENAME_PREFIX "processed_"
#define PROCESSED_IMAGE_SUBFOLDER "processed"

int CameraCalibrationHelper::calibrateWithImages(std::filesystem::path path) {
  resetResults();

  cv::glob(path.string(), inputImagePaths);

  cv::Mat frame;

  for (unsigned int img = 0; img < inputImagePaths.size(); img++) {
    frame = cv::imread(inputImagePaths[img]);

    inputImages.push_back(frame);
  }

  return calibrateWithImages(inputImages);
}

int CameraCalibrationHelper::calibrateWithImages(const std::vector<cv::Mat>& images) {
  bool calledInternally = false;
  if (&images == &inputImages) {
    calledInternally = true;
  }

  if (!calledInternally) {
    resetResults();
  }

  std::vector<std::vector<cv::Point3f> > objpoints;
  std::vector<std::vector<cv::Point2f> > imgpoints;

  std::vector<cv::Point3f> objp;
  for(int c = 0; c < checkerboardWidth; c++) {
    for(int r = 0; r < checkerboardHeight; r++){
      objp.push_back(cv::Point3f(r,c,0));
    }
  }

  cv::Mat frame, gray;

  std::vector<cv::Point2f> corner_pts;
  bool success;
  int successfullyProcessedImages = 0;

  for (unsigned int img = 0; img < images.size(); img++) {
    frame = images[img];

    if (!calledInternally) {
      inputImages.push_back(frame);
    }

    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    success = cv::findChessboardCorners(gray, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

    if(success)
    {
      cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
      cv::cornerSubPix(gray,corner_pts,cv::Size(11,11), cv::Size(-1,-1),criteria);

      cv::Mat processedFrame = frame.clone();
      cv::drawChessboardCorners(processedFrame, cv::Size(checkerboardHeight, checkerboardWidth), corner_pts, success);

      processedImages.push_back(processedFrame);

      // If function was called by calibrateWithImages(std::filesystem::path), inputImagePaths will be filled,
      // and we can also populate the processedImagePaths.
      if (inputImagePaths.size() > img) {
        processedImagePaths.push_back(inputImagePaths.at(img));
      }

      objpoints.push_back(objp);
      imgpoints.push_back(corner_pts);

      successfullyProcessedImages++;
    }
  }

  cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), cameraMatrix, distCoeffs, rotationVectors, translationVectors);

  return successfullyProcessedImages;
}

int CameraCalibrationHelper::calibrateInteractively(cv::VideoCapture& videoSource, std::filesystem::path path) {
  resetResults();

  bool saveImages = false;
  std::filesystem::path folder = "";
  std::filesystem::path extension = "";
  if (path.string().length() > 0) {
    saveImages = true;
    path = path.lexically_normal();
    folder = path.parent_path();
    extension = path.extension();
  }

  cv::Mat frame;
  cv::namedWindow("Calibration Preview", cv::WINDOW_NORMAL);

  // Take images for calibration until user aborts the process.
  bool takePic = true;
  bool cont = true;
  int picsTaken = 0;
  std::cout << "Press P, I or ENTER to take a picture, Q or ESC to abort." << std::endl;
  std::cout << "ENTER will abort after enough pictures are taken, P will continue taking pictures indefinitely." << std::endl;
  std::cout << "Pictures taken: " << picsTaken << std::flush;
  std::cout.flush();
  while (cont) {
    videoSource >> frame;

    if (frame.empty()) {
      continue;
    }

    takePic = false;

    cv::imshow("Calibration Preview", frame);

    int key = cv::waitKey(1);

    if (key == 'p' || key == 'P' || key == 'i' || key == 'I') {
      takePic = true;
    } else if (key == 'q' || key == 'Q' || key == '\x1B') {
      takePic = false;
      cont = takePic;
    } else if (key == '\n' || key == '\r') {
      takePic = picsTaken < DEFAULT_CALIBRATION_IMAGE_COUNT ? true : false;
      cont = takePic;
    }

    if (takePic) {
      picsTaken++;
      std::cout << CLEAR_LINE_ESCAPE_SEQUENCE << "Pictures taken: " << picsTaken << std::flush;
      inputImages.push_back(frame.clone());
    }
  }

  std::cout << "\n";

  int processedImgCount = calibrateWithImages(inputImages);

  cv::destroyWindow("Calibration Preview");

  if (!saveImages) {
    return processedImgCount;
  }

  std::filesystem::path processedFolder = folder;
  processedFolder /= PROCESSED_IMAGE_SUBFOLDER;
  if (!std::filesystem::exists(processedFolder)) {
    assert(std::filesystem::create_directories(processedFolder));
  }

  for (unsigned int i = 0; i < inputImages.size(); i++) {
    std::filesystem::path inputFileName(std::to_string(i) + extension.string());
    std::filesystem::path inputImPath = folder;
    inputImPath /= inputFileName;
    cv::imwrite(inputImPath, inputImages.at(i));
  }

  for (unsigned int i = 0; i < processedImages.size(); i++) {
    std::filesystem::path processedImageFileName(PROCESSED_IMAGE_FILENAME_PREFIX + std::to_string(i) + extension.string());
    std::filesystem::path processedImPath = processedFolder;
    processedImPath /= processedImageFileName;
    cv::imwrite(processedImPath, processedImages.at(i));
  }

  return processedImgCount;
}
