#pragma once
#include "StereoTracker.h"

StereoTracker::StereoTracker()
{
	R = Mat(Size(3, 3), CV_64FC1);
	T = Mat(Size(1, 3), CV_64FC1);
	cml = Mat(Size(3, 3), CV_64F);
	cmr = Mat(Size(3, 3), CV_64F);
	dcl = Mat(Size(5, 1), CV_64F);
	dcr = Mat(Size(5, 1), CV_64F);
	
	string ptstring = "####### StereoTracker Initialized #######\n";
	OutputDebugStringA(ptstring.c_str());


	prev_ptl = START_PTL;
	prev_ptr = START_PTR;
	curr_3dpt = Point3f(100000,10000001,100000000);

	backgroundl = imread(BG_PL, CV_LOAD_IMAGE_GRAYSCALE);
	backgroundr = imread(BG_PR, CV_LOAD_IMAGE_GRAYSCALE);

	added_bg = false;
	found = false;
	tracking = false;
	f_num = 0;
	
	importFromXML();
	//initClassMatrix();

}

void StereoTracker::initClassMatrix()
{
	string ptstring = "####### initClassMatrix #######\n";
	OutputDebugStringA(ptstring.c_str());

	try {
		readMatFromFile(R, "cam_data/R_taylor.txt");
		readMatFromFile(T, "cam_data/T_taylor.txt");
		readMatFromFile(cml, "cam_data/cam_matl.txt");
		readMatFromFile(dcl, "cam_data/coeffl.txt");
		readMatFromFile(cmr, "cam_data/cam_matr.txt");
		readMatFromFile(dcr, "cam_data/coeffr.txt");
	}
	catch(...) {
		cout << "Unable to open one of the files" << endl;
	}
	stereoRectify(cml, dcl, cmr, dcr, F_SIZE, R, T, R1, R2, P1, P2, Q);
	
	ptstring = "####### initClassMatrix END #######\n";
	OutputDebugStringA(ptstring.c_str());

}

void StereoTracker::reset()
{
	pt3d.clear();
	prev_ptl = START_PTL;
	prev_ptr = START_PTR;

	found = false;
}

void StereoTracker::findBall()
{
	found = false;
	// Create ROIs around previous location
	int x1 = prev_ptl.x - ROI_RADIUS;
	int x2 = prev_ptl.x + ROI_RADIUS;
	if (x1 < 0)
		x1 = 0;
	if (x2 > lframe.size().width)
		x2 = lframe.size().width;

	int y1 = prev_ptl.y - 1.5*ROI_RADIUS;
	int y2 = prev_ptl.y + 1.5*ROI_RADIUS;
	if (y1 < 0)
		y1 = 0;
	if (y2 > lframe.size().height)
		y2 = lframe.size().height;

	int x3 = x1;
	int y3 = y1;
	Rect ROIL = Rect(x1, y1, x2 - x1, y2 - y1);

	x1 = prev_ptr.x - ROI_RADIUS;
	x2 = prev_ptr.x + ROI_RADIUS;
	if (x1 < 0)
		x1 = 0;
	if (x2 > rframe.size().width)
		x2 = rframe.size().width;

	y1 = prev_ptr.y - 1.5*ROI_RADIUS;
	y2 = prev_ptr.y + 1.5*ROI_RADIUS;
	if (y1 < 0)
		y1 = 0;
	if (y2 > rframe.size().height)
		y2 = rframe.size().height;
	Rect ROIR = Rect(x1, y1, x2 - x1, y2 - y1);

	Mat bl = Mat(backgroundl, ROIL);
	Mat br = Mat(backgroundr, ROIR);

	Mat fl = Mat(lframe, ROIL);
	Mat fr = Mat(rframe, ROIR);

	// Difference background and current frame and threshold
	Mat diffl, diffr;
	absdiff(fl, bl, diffl);
	absdiff(fr, br, diffr);

	//imshow("l", diffl);
	//imshow("r", diffr);
	//waitKey(0);

	threshold(diffl, diffl, 15, 255, THRESH_BINARY);
	threshold(diffr, diffr, 15, 255, THRESH_BINARY);

	// Find contours
	vector<vector<Point>> conl, conr;
	findContours(diffl, conl, RETR_LIST, CHAIN_APPROX_NONE);
	findContours(diffr, conr, RETR_LIST, CHAIN_APPROX_NONE);

	if (conl.size() > 0 && conr.size() > 0)
	{
		Point2f ptl_new, ptr_new;

		// Find center of largest contour in left image
		found = true;
		tracking = true;
		double max = 0;
		int index = -1;
		for (int i = 0; i < conl.size(); i++)
		{
			double s = contourArea(conl[i]);
			if (s > max)
			{
				max = s;
				index = i;
			}
		}

		if (max > 10)
		{
			auto m = moments(conl[index]);
			ptl_new = Point2f(x3 + m.m10 / m.m00, y3 + m.m01 / m.m00);
		}
		else
			found = false;

		// Find center of largest contour in right image
		max = 0;
		index = -1;
		for (int i = 0; i < conr.size(); i++)
		{
			double s = contourArea(conr[i]);
			if (s > max)
			{
				max = s;
				index = i;
			}
		}

		if (max > 10)
		{
			auto m = moments(conr[index]);
			ptr_new = Point2f(x1 + m.m10 / m.m00, y1 + m.m01 / m.m00);
		}
		else
			found = false;

		// Update the last know point if both cameras find the ball
		if (found)
		{
			prev_ptl = ptl_new;
			prev_ptr = ptr_new;
		}
	}
	else
		cout << "Unable to find ball" << endl;
}

Point3f StereoTracker::calculate3DBallLocation()
{
	vector<Point2f> pl, plo, pr, pro;
	pl.push_back(prev_ptl);
	pr.push_back(prev_ptr);
	undistortPoints(pl, plo, cml, dcl, R1, P1);
	undistortPoints(pr, pro, cmr, dcr, R2, P2);

	ptl = plo[0];
	ptr = pro[0];

	vector<Point3f> d, wc;
	d.push_back(Point3f(ptl.x, ptl.y, ptl.x - ptr.x));
	perspectiveTransform(d, wc, Q);

	curr_3dpt = Point3f(-(wc[0].x - CC_OFFX), -(wc[0].y - CC_OFFY), wc[0].z - CC_OFFZ);
	this->pt3d.push_back(curr_3dpt);

	if (prev_ptl.x > 620 || prev_ptl.y > 460)
	{
		// reset
		this->reset();
	}

	if (prev_ptr.x < 20 || prev_ptr.y > 460)
	{
		this->reset();
	}

	if (pt3d.size() > 100)
	{
		this->reset();
	}

	return curr_3dpt;
}

Point2d StereoTracker::estimateEndPoint()
{
	string intro = "estimateEndPoint\n";
	OutputDebugStringA(intro.c_str());
	for (auto pt : pt3d)
	{
		string ptstring = "(" + to_string(pt.x) + ", " + to_string(pt.y) + ", " + to_string(pt.z) + ")\n";
		OutputDebugStringA(ptstring.c_str());
	}
	int size = pt3d.size();
	Mat x = Mat(size, 1, CV_64F);
	Mat y = Mat(size, 1, CV_64F);
	Mat xz = Mat(size, 3, CV_64F);
	Mat yz = Mat(size, 3, CV_64F);
	// Construct matrices needed for lsq
	// X direction is linear estimator, Y direction is quadratic estimator
	for (int i = 0; i < size; i++)
	{
		xz.at<double>(i, 0) = pt3d[i].z * pt3d[i].z;
		xz.at<double>(i, 1) = pt3d[i].z;
		xz.at<double>(i, 2) = 1;

		yz.at<double>(i, 0) = pt3d[i].z * pt3d[i].z;
		yz.at<double>(i, 1) = pt3d[i].z;
		yz.at<double>(i, 2) = 1;

		x.at<double>(i, 0) = pt3d[i].x;
		y.at<double>(i, 0) = pt3d[i].y;

	}

	Mat x_coeff, y_coeff;
	// method 1: using SVD to computer pseudo inverse
	// method 2: using direct matrix calculation
	int method = 2;
	if(method==1) {
		// Compute the least squares solution to az + b = x
		Mat xz_inv = xz.inv(DECOMP_SVD);
		x_coeff = xz_inv*x;

		// Compute the least squares solution to az^2 + bz + c = y
		Mat yz_inv = yz.inv(DECOMP_SVD);
		y_coeff = yz_inv*y;
	}
	else if(method==2) {
		Mat xz_t = xz.t();
		x_coeff = (xz_t*xz).inv()*xz_t*x;

		Mat yz_t = yz.t();
		y_coeff = (yz_t*yz).inv()*yz_t*y;
	}

	double x_end = x_coeff.at<double>(2,0);
	double y_end = y_coeff.at<double>(2,0);

	tracking = false;
	
	double x_scaling = 0.75;
	double y_scaling = 1;
	end_pt = Point2d(x_end*x_scaling, y_end*y_scaling);
	string end_point = "end point: (" + to_string(end_pt.x) + ", " + to_string(end_pt.y) + ")\n";
	OutputDebugStringA(end_point.c_str());

	return end_pt;
}

void StereoTracker::updateBackground(Mat bl, Mat br)
{
	backgroundl = bl.clone();
	backgroundr = br.clone();
	added_bg = true;
}

void StereoTracker::saveMattoFile(Mat data, string path)
{
	ofstream out;
	out.open(path);
	out.precision(15);
	assert(out.is_open());
	for (int i = 0; i < data.rows; i++)
	{
		for (int j = 0; j < data.cols; j++)
			out << data.at<double>(i, j) << " ";
		out << endl;
	}
	out.close();
}

void StereoTracker::readMatFromFile(Mat &data, string path)
{
	// This function assumes the Mat was already created with the expected size
	ifstream in;
	in.open(path);
	if (in.is_open())
	{
		assert(in.is_open());
		for (int i = 0; i < data.rows; i++)
		{
			for (int j = 0; j < data.cols; j++)
			{
				in >> data.at<double>(i, j);
			}
		}
	}
	else
		cout << "Unable to open file " << path << endl;
	
}

void StereoTracker::importFromXML(){
	
	//Mat cml, cmr, dcl, dcr;
	//Mat R, T;
	//Mat R1, P1, R2, P2, Q;

	FileStorage fs("cam_data/left_calib.xml", FileStorage::READ);
	fs["intrinsic"] >> cml;
	fs["distortion"] >> dcl;
	fs.release();

	FileStorage fs1("cam_data/right_calib.xml", FileStorage::READ);
	fs1["intrinsic"] >> cmr;
	fs1["distortion"] >> dcr;
	fs1.release();

	FileStorage fs2("cam_data/new_stereo3.xml", FileStorage::READ);
	fs2["rotation"] >> R;
	fs2["translation"] >> T;
	fs2.release();

	FileStorage fs3("cam_data/new_rect3.xml", FileStorage::READ);
	fs3["r1"] >> R1;
	fs3["p1"] >> P1;
	fs3["r2"] >> R2;
	fs3["p2"] >> P2;
	fs3["q"] >> Q;
	fs3.release();


	string intro = "####### importFromXML #######\n";
	OutputDebugStringA(intro.c_str());
	//string q1 = to_string(this->Q.at<double>(1, 3));
	//OutputDebugStringA(q1.c_str());

}
