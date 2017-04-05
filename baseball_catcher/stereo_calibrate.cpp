#include "opencv2/opencv.hpp"
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

string window_name = "src";
Mat src, gray;

int main(int argc, char* argv[])
{
  string left_header = "/home/magiccjae/jae_stuff/classes/robotic_vision/baseball_catcher/images/left";
  string right_header = "/home/magiccjae/jae_stuff/classes/robotic_vision/baseball_catcher/images/right";
  string ending = ".bmp";

  namedWindow(window_name, CV_WINDOW_AUTOSIZE);
  src = imread(left_header+to_string(1)+ending);
  Size image_size = src.size();
  cout << "image size" << image_size << endl;
  Size patternsize(10,7); //interior number of corners

  vector<vector<Point3f>> object_points;    // This vector contains the same number of object points as the number of images with patterns. In this case 40
  vector<Point3f> object_point;   // how many points are on the chessboard. In this case 7x10 (7 rows 10 columns)
  vector<vector<Point2f>> image_points;   // This vector contains the same number of object points as the number of images with patterns. In this case 40
  vector<Point2f> corners;   //this will be filled by the detected corners

  for(int i=0; i<patternsize.height; i++){
    for(int j=0; j<patternsize.width; j++){
      object_point.push_back(Point3f(3.88*j,3.88*i,0.0f));
    }
  }

  // ========== camera intrinsic calibration
  int num_images = 50;
  for(int i=0; i<=num_images; i++){
    cout << i << endl;

    src = imread(left_header+to_string(i)+ending);
    cvtColor(src,gray,CV_BGR2GRAY);
    bool patternfound = findChessboardCorners(gray, patternsize, corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
    cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 50, 0.01));
    // cout << corners << endl;
    drawChessboardCorners(src, patternsize, Mat(corners), patternfound);

    if(patternfound==true){
      cout << "good" << endl;
      object_points.push_back(object_point);
      image_points.push_back(corners);
    }
    else{
      cout << "bad" << endl;
    }
    imshow(window_name,src);
    waitKey(0);
  }

  // These are variables that will hold the camera intrinsic matrix, distortion coefficient, rotation matrix, translation matrix.
  Mat left_intrinsic = Mat(3, 3, CV_32FC1);
  Mat left_distCoeffs;
  vector<Mat> rvecs;
  vector<Mat> tvecs;

  calibrateCamera(object_points, image_points, image_size, left_intrinsic, left_distCoeffs, rvecs, tvecs);
  cout << "left_intrinsic" << endl;
  cout << left_intrinsic << endl;
  cout << "left_distCoeffs" << endl;
  cout << left_distCoeffs << endl;

  object_points.clear();
  image_points.clear();

  for(int i=0; i<=num_images; i++){
    cout << i << endl;

    src = imread(right_header+to_string(i)+ending);
    cvtColor(src,gray,CV_BGR2GRAY);
    bool patternfound = findChessboardCorners(gray, patternsize, corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
    cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 50, 0.01));
    // cout << corners << endl;
    drawChessboardCorners(src, patternsize, Mat(corners), patternfound);

    if(patternfound==true){
      cout << "good" << endl;
      object_points.push_back(object_point);
      image_points.push_back(corners);
    }
    else{
      cout << "bad" << endl;
    }
    imshow(window_name,src);
    waitKey(0);
  }

  // These are variables that will hold the camera intrinsic matrix, distortion coefficient, rotation matrix, translation matrix.
  Mat right_intrinsic = Mat(3, 3, CV_32FC1);
  Mat right_distCoeffs;

  calibrateCamera(object_points, image_points, image_size, right_intrinsic, right_distCoeffs, rvecs, tvecs);
  cout << "right_intrinsic" << endl;
  cout << right_intrinsic << endl;
  cout << "right_distCoeffs" << endl;
  cout << right_distCoeffs << endl;

  object_points.clear();
  image_points.clear();

  // ========== stereo calibration
  string left_stereo_header = "/home/magiccjae/jae_stuff/classes/robotic_vision/baseball_catcher/images/stereoL";
  string right_stereo_header = "/home/magiccjae/jae_stuff/classes/robotic_vision/baseball_catcher/images/stereoR";
  vector<vector<Point2f>> image_points_left;   // This vector contains the same number of object points as the number of images with patterns.
  vector<vector<Point2f>> image_points_right;   // This vector contains the same number of object points as the number of images with patterns.

  for(int i=1; i<=num_images; i++){
    cout << i << endl;

    // left
    Mat left_src = imread(left_stereo_header+to_string(i)+ending);
    Mat left_gray;
    cvtColor(left_src,left_gray,CV_BGR2GRAY);

    vector<Point2f> left_corners;   // This will be filled by the detected corners
    bool left_patternfound = findChessboardCorners(gray, patternsize, left_corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
    cornerSubPix(left_gray, left_corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 100, 0.001));
    drawChessboardCorners(left_src, patternsize, Mat(left_corners), left_patternfound);

    // right
    Mat right_src = imread(right_stereo_header+to_string(i)+ending);
    Mat right_gray;
    cvtColor(right_src,right_gray,CV_BGR2GRAY);

    vector<Point2f> right_corners;   // This will be filled by the detected corners
    bool right_patternfound = findChessboardCorners(gray, patternsize, right_corners, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE + CALIB_CB_FAST_CHECK);
    cornerSubPix(right_gray, right_corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 100, 0.001));
    drawChessboardCorners(right_src, patternsize, Mat(right_corners), right_patternfound);

    if(left_patternfound==true && right_patternfound==true){
      image_points_left.push_back(left_corners);
      image_points_right.push_back(right_corners);
      object_points.push_back(object_point);
      cout << "good" << endl;
    }
    else{
      cout << "bad" <<endl;
    }

    imshow("left",left_src);
    imshow("right",right_src);
    waitKey(0);
  }

  // stereoCalibrate output place holders
  Mat rotation;
  Mat translation;
  Mat essential;
  Mat fundamental;

  stereoCalibrate(object_points, image_points_left, image_points_right, left_intrinsic, left_distCoeffs, \
                  right_intrinsic, right_distCoeffs, image_size, rotation, translation, essential, fundamental, \
                  CV_CALIB_FIX_INTRINSIC, TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 100, 0.001));

  cout << "===== R =====" << endl;
  cout << rotation << endl;
  cout << "===== T =====" << endl;
  cout << translation << endl;
  cout << "===== E =====" << endl;
  cout << essential << endl;
  cout << "===== F =====" << endl;
  cout << fundamental << endl;

  Mat R1, R2, P1, P2, Q;
  stereoRectify(left_intrinsic, left_distCoeffs, right_intrinsic, right_distCoeffs, \
    src.size(), rotation, translation, R1, R2, P1, P2, Q, 0, -1, image_size, 0, 0);
  cout << "===== R1 =====" << endl;
  cout << R1 << endl;
  cout << "===== R2 =====" << endl;
  cout << R2 << endl;
  cout << "===== P1 =====" << endl;
  cout << P1 << endl;
  cout << "===== P2 =====" << endl;
  cout << P2 << endl;
  cout << "===== Q =====" << endl;
  cout << Q << endl;


  FileStorage fs("cam_data.xml", FileStorage::WRITE);
  fs <<"left_intrinsic" << left_intrinsic;
  fs << "left_distCoeffs" << left_distCoeffs;
  fs <<"right_intrinsic" << right_intrinsic;
  fs << "right_distCoeffs" << right_distCoeffs;
  fs << "rotation" << rotation;
  fs << "translation" << translation;
  fs << "Q" << Q;

  fs.release();


  return 0;
}
