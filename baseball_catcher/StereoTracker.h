#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "stdafx.h"
#include "time.h"
#include "math.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

using namespace std;
using namespace cv;

class StereoTracker
{
private:
	Mat lframe, rframe;
	Mat backgroundr, backgroundl;
	Mat cml, cmr, dcl, dcr;
	Mat R, T;
	Mat R1, P1, R2, P2, Q;

	// Distorted left and right ball points
	Point2f prev_ptl;
	Point2f prev_ptr;
	// Undistorted left and right ball points
	Point2f ptl, ptr;

	Point2d end_pt = Point2d(-1, -1);

	Point3f curr_3dpt;

	vector<Point3f> pt3d;

	bool added_bg;
	bool found;
	bool tracking;
	int f_num;

	// Constants
	const Point2f START_PTL = Point2f(360, 115);
	const Point2f START_PTR = Point2f(295, 120);

	const string BG_PL = "images/backgroundL.bmp";
	const string BG_PR = "images/backgroundR.bmp";

	const Size F_SIZE = Size(640, 480);

	const int NOT_FOUND_LIMIT = 20;

	const int ROI_RADIUS = 50;
	// Ball catcher offsets from left camera
	/*const int CC_OFFX = 12;
	const int CC_OFFY = 23;
	const int CC_OFFZ = 7;*/
	
	// 3 catch, 2 rim, 2 miss(catcher went to the right)
	/*const int CC_OFFX = 12;
	const int CC_OFFY = 20;
	const int CC_OFFZ = 7;*/

	// pretty good 5 catch, 2 rim, 3 miss(catcher went higher when ball was thrown high)
	//const int CC_OFFX = 11;
	//const int CC_OFFY = 18;
	//const int CC_OFFZ = 7;

	// increasing Y offset means catcher goes higher
	// increasing X offset means catcher move more right

	// realy good catch in the low region. Inconsistent on mid and high region
	const int CC_OFFX = 13;
	const int CC_OFFY = 16;
	const int CC_OFFZ = 7;

	/*const int CC_OFFX = 14;
	const int CC_OFFY = 7;
	const int CC_OFFZ = 80;*/

public:
	// Constructor
	StereoTracker();

	// Initialization functions
	void initClassMatrix();
	void reset();

	// Update functions
	void updateFrames(Mat fl, Mat fr) {
		this->lframe = fl;
		this->rframe = fr;

		if (!added_bg)
		{
			updateBackground(fl, fr);
		}

		found = false;
	}
	void updateBackground(Mat bl, Mat br);
	void updatePrevPoints() {
		prev_ptl = ptl;
		prev_ptr = ptr;
	}
	void findBall();
	bool foundBall() {
		return found;
	}
	Point3f calculate3DBallLocation();
	Point2d estimateEndPoint();

	// Getters
	Mat getLFrame() {
		return this->lframe;
	}
	Mat getRFrame() {
		return this->rframe;
	}
	Mat getLDiff() {
		Mat diff;
		absdiff(lframe, backgroundl, diff);
		threshold(diff, diff, 15, 255, THRESH_BINARY);
		//cvtColor(diff, diff, CV_GRAY2BGR);
		String pt = "(" + to_string(curr_3dpt.x) + ", " + to_string(curr_3dpt.y) + ", " + to_string(curr_3dpt.z) + ")";
		String str1 = "Last location: " + pt;

		String prev_point = to_string(prev_ptl.x) + ", " + to_string(prev_ptl.y);
		putText(diff, prev_point, Point(10, 70), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);
		putText(diff, str1, Point(10, 95), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);
		putText(diff, "vector size: " + to_string(pt3d.size()), Point(10, 120), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);
		String end = "end point: (" + to_string(end_pt.x) + ", " + to_string(end_pt.y) + ")";
		putText(diff, end, Point(10, 145), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);

		if (found){
			putText(diff, "ball found", Point(10, 145), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);
		}
		circle(diff, prev_ptl, 5, Scalar(255), 2);
		return diff;
	}
	Mat getRDiff() {
		Mat diff;
		absdiff(rframe, backgroundr, diff);
		threshold(diff, diff, 15, 255, THRESH_BINARY);
		//cvtColor(diff, diff, CV_GRAY2BGR);
		String prev_point = to_string(prev_ptr.x) + ", " + to_string(prev_ptr.y);
		putText(diff, prev_point, Point(10, 70), FONT_HERSHEY_SIMPLEX, .65, Scalar(255, 255, 255), 2);
		circle(diff, prev_ptr, 5, Scalar(255), 2);
		return diff;
	}
	vector<Point3f> get3DPoints() {
		return pt3d;
	}
	Point3f getCurrent3DPoint(){
		return curr_3dpt;
	}
	Point2f getLeftBallLocation() {
		return prev_ptl;
	}
	Point2f getRightBallLocation() {
		return prev_ptr;
	}
	bool bgAdded() {
		return added_bg;
	}
	void getNewBG() {
		added_bg = false;
	}
	int getNotFound() {
		return f_num;
	}
	bool isTracking() {
		return tracking;
	}

	// Utility functions
	void saveMattoFile(Mat data, string path);
	void readMatFromFile(Mat &data, string path);
	void importFromXML();
};
