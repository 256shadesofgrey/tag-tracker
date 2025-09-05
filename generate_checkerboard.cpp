#include <boost/program_options.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include <tag-tracker.h>

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  int verbosity = 0;
  int width = 6;
  int height = 9;
  int pixelsPerSquare = 400; // 400 for a 6x9 checkerboard corresponds to about 300dpi when printing on A4.
  std::string path = "./output/";
  std::string prefix = "checkerboard";

  po::options_description desc("Available options", HELP_LINE_LENGTH, HELP_DESCRIPTION_LENGTH);
  desc.add_options()
    ("help,h", "Show this message.")
    ("verbose,v", "Display additional information.")
    ("width,W", po::value<int>(), std::format("Number of squares for width. (Default: {})", width).c_str())
    ("height,H", po::value<int>(), std::format("Number of squares for height. (Default: {})", height).c_str())
    ("pps,r", po::value<int>(), std::format("Resolution in pixels per square. (Default: {})", pixelsPerSquare).c_str())
    ("prefix,p", po::value<std::string>(), std::format("File name prefix.(Default: {})", prefix).c_str())
    ("output,o", po::value<std::string>(), std::format("Output folder. (Default: {})", path).c_str())
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  verbosity = vm.count("verbose");

  if (vm.count("width")) {
    width = vm["width"].as<int>();
  }

  if (vm.count("height")) {
    height = vm["height"].as<int>();
  }

  if (vm.count("pps")) {
    pixelsPerSquare = vm["pps"].as<int>();
  }

  if (vm.count("output")) {
    path = std::filesystem::path(vm["output"].as<std::string>()).lexically_normal().string();
  }

  if (vm.count("prefix")) {
    prefix = vm["prefix"].as<std::string>();
  }

  if (verbosity > 0) {
    std::cout << "Setting width to: " << width << std::endl;
    std::cout << "Setting height to: " << height << std::endl;
    std::cout << "Setting pixels per square to: " << pixelsPerSquare << std::endl;
    std::cout << "Setting filename prefix to: " << prefix << std::endl;
    std::cout << "Setting output path to: " << path << std::endl;
  }

  if (!std::filesystem::exists(path)) {
    assert(std::filesystem::create_directory(path));
  }

  int imgWidth = width*pixelsPerSquare;
  int imgHeight = height*pixelsPerSquare;

  // Create an SVG file
  std::ofstream svgFile(path + "/" + prefix + ".svg");
  svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << imgWidth
          << "\" height=\"" << imgHeight << "\">";

  for (int c = 0; c < width; c++) {
    for (int r = 0; r < height; r++) {
      if ((r+c) % 2 == 0) {
        svgFile << "<rect x=\"" << pixelsPerSquare * c << "\" y=\"" << pixelsPerSquare * r
                << "\" width=\" " << pixelsPerSquare << "\" height=\" " << pixelsPerSquare
                << "\" fill=\"black\"/>";
      }
    }
  }

  svgFile << "</svg>";
  svgFile.close();
}
