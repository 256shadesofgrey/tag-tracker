#include <fstream>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <string>

int main() {
  int markerSize = 8;
  int imageScale = 25;
  int imageSize = markerSize * imageScale;
  std::string baseFileName = "marker";
  int markerId = 0;

  cv::Mat markerImage;
  cv::aruco::Dictionary dictionary =
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
  cv::aruco::generateImageMarker(dictionary, markerId, markerSize, markerImage,
                                 1);

  // Create an SVG file
  std::ofstream svgFile(baseFileName + std::to_string(markerId) + ".svg");
  svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << imageSize
          << "\" height=\"" << imageSize << "\">";

  // Convert the marker to SVG
  for (int i = 0; i < markerSize; ++i) {
    for (int j = 0; j < markerSize; ++j) {
      if (markerImage.at<uchar>(j, i) == 0) {
        svgFile << "<rect x=\"" << imageScale * i << "\" y=\"" << imageScale * j
                << "\" width=\" " << imageScale << "\" height=\" " << imageScale
                << "\" fill=\"black\"/>";
      }
    }
  }

  svgFile << "</svg>";
  svgFile.close();

  cv::Mat resizedMarkerImage;
  cv::resize(markerImage, resizedMarkerImage, cv::Size(imageSize, imageSize), 0,
             0, cv::INTER_NEAREST);
  cv::imwrite(baseFileName + std::to_string(markerId) + ".png",
              resizedMarkerImage);

  return 0;
}
