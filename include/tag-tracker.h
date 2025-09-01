#pragma once

#include <string>
#include <sstream>
#include <map>
#include <opencv2/aruco.hpp>
#include <opencv2/opencv.hpp>

// Get string from variable name.
#define VARNAME(a) std::string(#a)

// Get the name of the aruco tag dictionary without the namespaces.
//#define DICTNAME(a) std::string(#a).substr(11)

// I am really looking forward to C++26 reflection features to make this unnecessary...
using namespace cv::aruco;
#define X(a) {a, std::string(#a)},
static const std::map<const int, const std::string> arucoDict = {
  X(DICT_4X4_50)
  X(DICT_4X4_100)
  X(DICT_4X4_250)
  X(DICT_4X4_1000)
  X(DICT_5X5_50)
  X(DICT_5X5_100)
  X(DICT_5X5_250)
  X(DICT_5X5_1000)
  X(DICT_6X6_50)
  X(DICT_6X6_100)
  X(DICT_6X6_250)
  X(DICT_6X6_1000)
  X(DICT_7X7_50)
  X(DICT_7X7_100)
  X(DICT_7X7_250)
  X(DICT_7X7_1000)
  X(DICT_ARUCO_ORIGINAL)
  X(DICT_APRILTAG_16h5)
  X(DICT_APRILTAG_25h9)
  X(DICT_APRILTAG_36h10)
  X(DICT_APRILTAG_36h11)
  X(DICT_ARUCO_MIP_36h12)
};
#undef X

std::string dictName(int dict) {
  std::string ret;
  try{
    ret = arucoDict.at(dict);
  } catch (const std::out_of_range&) {
    ret = arucoDict.at(cv::aruco::DICT_6X6_250);
  }

  return ret;
}

std::string dictsString() {
  std::stringstream dictStream;

  for (auto d : arucoDict) {
    dictStream << d.first << "=" << d.second << "\n";
  }
  dictStream.flush();

  return dictStream.str();
}

template <typename T> std::string vec2str(std::vector<T> v) {
  std::string str = "{";

  for (unsigned int i = 0; i < v.size(); i++) {
    if (i > 0) {
      str += ",";
    }
    str += std::to_string(v[i]);
  }

  return str += "}";
}
