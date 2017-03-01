#include "opencv2/opencv.hpp"
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

string window = "src";
string window1 = "saturation";
string window2 = "grid";

Mat src, grid, saturated;

int main(int, char**){
  string path = "/home/magiccjae/jae_stuff/classes/robotic_vision/monster_truck/test/test1.mp4";
  VideoCapture cap(path); // open the default camera
  if(!cap.isOpened())  // check if we succeeded
      return -1;

  namedWindow(window, CV_WINDOW_AUTOSIZE);

  // how many grids I would like to have
  int grid_num_col = 32;
  int grid_num_row = 18;
  Size size(grid_num_col, grid_num_row);
  // an image from the video is 1920 x 1080. To divide this image into 16 x 9 grid, one grid is 120 x 120 pixels
  cap >> src;
  Size original_size = src.size();
  resize(src,src,size);
  Size image_size = src.size();

  // for thresholding
  int threshold_value = 210;
  int MAX_VALUE = 255;
  // for opening operation
  int morph_elem = 0;
  int morph_size = 3;
  Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
  int operation = 2;

  for(;;){
      cap >> src; // get a new frame from camera
      Mat bgr[3];
      split(src, bgr);
      saturated = bgr[2];
      threshold(saturated, saturated, threshold_value, MAX_VALUE, 0);
      morphologyEx( saturated, saturated, operation, element );
      resize(saturated,grid,size, 0, 0, INTER_NEAREST);
      resize(grid,grid,original_size, 0, 0, INTER_AREA);
      imshow(window1, saturated);
      imshow(window2, grid);
      imshow(window, src);
      if(waitKey(0)==27) break;
  }
  return 0;
}
