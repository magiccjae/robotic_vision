#include "opencv2/opencv.hpp"
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

string window_name = "video";
string trackbar_value = "value";
int threshold_value = 200;
int const MAX_VALUE = 255;

string window_blue = "blue";

Mat src, gray, thresholded, dst, dst1;

void threshold_change(int, void*);

int main(int, char**){
  string path = "/home/magiccjae/jae_stuff/classes/robotic_vision/monster_truck/test/test1.mp4";
  VideoCapture cap(path); // open the default camera
  if(!cap.isOpened())  // check if we succeeded
      return -1;

  namedWindow(window_name, CV_WINDOW_AUTOSIZE);
  namedWindow(window_blue, CV_WINDOW_AUTOSIZE);
  createTrackbar(trackbar_value, window_blue, &threshold_value, MAX_VALUE, threshold_change);

  Size size(640, 360);
  // an image from the video is 1920 x 1080. To divide this image into 16 x 9 grid, one grid is 120 x 120 pixels
  cap >> src;
  resize(src,src,size);
  Size image_size = src.size();
  // how many grids I would like to have
  int grid_num_col = 16;
  int grid_num_row = 9;

  // how many pixels are horizontally and vertically in a grid respectively
  int grid_hor_size = image_size.width/grid_num_col;
  int grid_ver_size = image_size.height/grid_num_row;
  // how many pixels in one grid
  int grid_total_pixels = grid_hor_size*grid_ver_size;

  int grid_values[grid_num_row][grid_num_col];

  // for Opening operation to remove noise on blue channel
  int morph_elem = 0;
  int morph_size = 3;
  Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
  int operation = 2;

  // threshold to determine if a grid is occupied or not
  int occupied = grid_total_pixels/4;

  for(;;){
      cap >> src; // get a new frame from camera
      resize(src,src,size);
      Mat bgr[3];
      split(src, bgr);
      Mat blue = bgr[0];
      // imshow(window_blue, blue);
      threshold(blue, thresholded, threshold_value, MAX_VALUE, 0);
      imshow("thresholded", thresholded);
      morphologyEx( thresholded, dst, operation, element );

      // initialize 2d array that stores the number of non-zero pixels
      for(int i=0; i<grid_num_row; i++){
        for(int j=0; j<grid_num_col; j++){
          grid_values[i][j] = 0;
        }
      }
      for(int i=0; i<grid_num_row; i++){
        for(int j=0; j<grid_num_col; j++){
          // cout << "(" << i << " " << j << ")" << endl;
          for(int k=0; k<grid_ver_size; k++){
            for(int l=0; l<grid_hor_size; l++){
              // cout << "(" << grid_ver_size*i+l << "," << grid_hor_size*j+k << ") ";
              int pixel_value = dst.at<uint8_t>(grid_ver_size*i+k, grid_hor_size*j+l);
              // cout << pixel_value << " ";
              if(pixel_value > 150){
                grid_values[i][j] = grid_values[i][j]+1;
              }
            }
          }
          if(grid_values[i][j] > occupied){
            putText(src, to_string(grid_values[i][j]), Point(grid_hor_size*j, grid_ver_size*i), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 255, 0), 1, 0);
          }
          // cout << endl;
          // cout << "The number of non-zero pixels: " << grid_values[i][j] << endl;
          // waitKey(0);
        }
      }
      imshow("dst", dst);
      imshow(window_name, src);
      if(waitKey(30)==27) break;
  }
  return 0;
}

void threshold_change(int, void*){
  cout << "new threshold: " << threshold_value << endl;
}
