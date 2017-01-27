#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

Mat src, src_gray, dst;
string window1 = "original";
string window2 = "result";

// Threshold
int threshold_value = 128;
int max_binary_value = 255;
int threshold_type = 0;
string trackbar_value = "value";

// Canny
int low_canny = 50;
int ratio = 3;
int max_canny_value = 100;
int kernel_size = 3;

// Morphology
int operation = 3;  // 3 for Closing operation
int morph_elem = 0;   // 0 for square kernel
int morph_size = 9;   // kernel size
int erosion_elem = 0;
int erosion_size = 3;

// void trackbar(int, void*);

int main(int, char**)
{
  // SimpleBlobDetector::Params params;
  // // Change thresholds
  // params.minThreshold = 100;
  // params.maxThreshold = 120;
  // params.filterByColor = true;
  // params.blobColor = 255;
  // // Filter by Area.
  // params.filterByArea = true;
  // params.minArea = 15000;
  // params.maxArea = 1000000;
  // // Filter by Circularity
  // params.filterByCircularity = true;
  // params.minCircularity = 0.1;
  // // Filter by Convexity
  // params.filterByConvexity = true;
  // params.minConvexity = 0.1;
  // // Filter by Inertia
  // params.filterByInertia = true;
  // params.minInertiaRatio = 0.1;
  // Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);


  src = imread("cookie_bad.bmp");
  namedWindow(window1, CV_WINDOW_AUTOSIZE);
  // createTrackbar( trackbar_value,
  //               window1, &low_canny,
  //               max_canny_value, trackbar );

  imshow(window1, src);
  cvtColor(src, src_gray, CV_BGR2GRAY);
  // trackbar(0, 0);
  threshold(src_gray, dst, threshold_value, max_binary_value, threshold_type);
  imshow("temp", dst);
  Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );
  // Apply the specified morphology operation
  morphologyEx( dst, dst, operation, element );
  imshow("binarized1", dst);
  Mat element1 = getStructuringElement( MORPH_RECT, Size( 2*erosion_size + 1, 2*erosion_size+1), Point( erosion_size, erosion_size ) );

  erode(dst, dst, element1);
  imshow("binarized2", dst);


  // vector<KeyPoint> keypoints;
  // detector->detect(dst, keypoints);
  // cout << keypoints.size() << endl;
  // drawKeypoints(dst, keypoints, dst, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );

  Canny( dst, dst, low_canny, low_canny*ratio, kernel_size );
  imshow("Canny", dst);

  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  findContours( dst, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

  double cookie_area = 0;
  int cookie_index = 0;
  for(int i=0; i < contours.size(); i++){
    double temp_area = contourArea(contours.at(i));
    cout << "temp area: " << temp_area << endl;
    if(temp_area > cookie_area){
      cookie_area = temp_area;
      cookie_index = i;
    }
  }
  cout << "cookie index: " << cookie_index << endl;
  cout << "cookie area: " << cookie_area << endl;

  // cout << contours.size() << endl;
  // cout << hierarchy.size() << endl;
  // Draw contours
  Mat drawing = Mat::zeros( dst.size(), CV_8UC3 );
  // for( int i = 0; i< contours.size(); i++ ){
  //   Scalar color = Scalar( i*20, i*20, i*20 );
  //   drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
  // }
  Scalar color = Scalar( 0, 255, 0 );
  drawContours( drawing, contours, cookie_index, color, 2, 8, hierarchy, 0, Point() );

  imshow(window2, drawing);
  while(waitKey(0) != 27){

  }
  return 0;
}

// void trackbar(int, void*){
// }
