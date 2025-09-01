#include <boost/program_options.hpp>
#include <format>
#include <fstream>
#include <iostream>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <filesystem>

#include <tag-tracker.h>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;

  int markerSize = 8;
  int imageSize = 200;
  std::string prefix = "marker";
  std::vector<int> markerIds = {0};
  std::string path = "./output/";

  po::options_description desc("Available options", 1024);
  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", "Display additional information.")
    ("id,i", po::value<std::vector<int>>()->default_value(markerIds, vec2str(markerIds)), "List of IDs encoded in the marker.")
    ("size,s", po::value<int>()->default_value(markerSize), "Size of the marker in squares per side.")
    ("resolution,r", po::value<int>()->default_value(imageSize), "Size of the generated image in pixels per side.")
    ("prefix,p", po::value<std::string>()->default_value(prefix), "File name prefix.")
    ("output,o", po::value<std::string>()->default_value(path), "Output folder for the generated tags.")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  // po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  verbosity = vm.count("verbose");

  if (vm.count("id")) {
    markerIds = vm["id"].as<std::vector<int>>();
  }

  if (vm.count("size")) {
    markerSize = vm["size"].as<int>();
  }

  if (vm.count("resolution")) {
    imageSize = vm["resolution"].as<int>();
  }

  if (vm.count("prefix")) {
    prefix = vm["prefix"].as<std::string>();
  }

  if (vm.count("output")) {
    path = std::filesystem::path(vm["output"].as<std::string>()).lexically_normal().string();
  }

  if (verbosity > 0) {
    std::cout << "Setting IDs to: " << vec2str(markerIds) << std::endl;
    std::cout << "Setting marker size to: " << markerSize << std::endl;
    std::cout << "Setting image size to: " << imageSize << std::endl;
    std::cout << "Setting filename prefix to: " << prefix << std::endl;
    std::cout << "Setting output path to: " << path << std::endl;
  }

  if (!std::filesystem::exists(path)) {
    assert(std::filesystem::create_directory(path));
  }

  double imageScale = imageSize / markerSize;

  cv::Mat markerImage;
  // TODO: make dictionary selection based on tag size.
  cv::aruco::Dictionary dictionary =
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

  for (unsigned int id = 0; id < markerIds.size(); id++) {
    cv::aruco::generateImageMarker(dictionary, markerIds[id], markerSize,
                                  markerImage, 1);

    // Create an SVG file
    std::ofstream svgFile(path + "/" + prefix + std::to_string(markerIds[id]) + ".svg");
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
    cv::imwrite(path + "/" + prefix + std::to_string(markerIds[id]) + ".png",
                resizedMarkerImage);
  }

  return 0;
}
