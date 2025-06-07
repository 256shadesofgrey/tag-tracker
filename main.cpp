#include <iostream>
#include <opencv2/opencv.hpp>

int main(int argc, char *argv[]) {
  std::string video_source = "http://192.168.178.10:8080/video";

  if (argc < 2) {
    std::cout << "No video source was given. Using the default " << video_source
              << std::endl;
  } else {
    video_source = std::string(argv[1]);
    std::cout << "Using the following source: " << video_source << std::endl;
  }

  cv::VideoCapture cap(video_source);

  if (!cap.isOpened()) {
    std::cerr << "Error: Could not open IP camera at " << video_source << "."
              << std::endl;
    return -1;
  }

  cv::namedWindow("Camera Feed", cv::WINDOW_NORMAL);
  cv::namedWindow("Edge detection", cv::WINDOW_NORMAL);

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
    if (cv::waitKey(1) >= 0)
      break;
  }

  // Release the VideoCapture object and close all windows
  cap.release();
  cv::destroyAllWindows();

  return 0;
}
