#include <iostream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include "StereoTracker.h"

using namespace std;
using namespace cv;

#define NUM_HORIZ 10
#define NUM_VERT 7

void saveMattoFile(Mat data, string path)
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

void readMatFromFile(Mat &data, string path)
{
	ifstream in;
	in.open(path);
	if (in.is_open())
	{
		assert(in.is_open());
		double a;
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

void get3DMeasurement()
{
	Mat ir = imread("img/R34.bmp", CV_LOAD_IMAGE_GRAYSCALE);
	Mat il = imread("img/L34.bmp", CV_LOAD_IMAGE_GRAYSCALE);

	vector<Point2f> cornersl, cornersr;

	bool found = findChessboardCorners(il, Size(NUM_HORIZ, NUM_VERT), cornersl);

	if (found)
	{
		// If a chessboard was found, perform cornerSubPix and display
		TermCriteria crit = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.01);
		cornerSubPix(il, cornersl, Size(5, 5), Size(-1, -1), crit);
	}

	found = findChessboardCorners(ir, Size(NUM_HORIZ, NUM_VERT), cornersr);

	if (found)
	{
		// If a chessboard was found, perform cornerSubPix and display
		TermCriteria crit = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.01);
		cornerSubPix(ir, cornersr, Size(5, 5), Size(-1, -1), crit);
	}

	// Get corners
	vector<Point2f> lc;
	lc.push_back(cornersl[0]);
	lc.push_back(cornersl[NUM_HORIZ - 1]);
	lc.push_back(cornersl[NUM_HORIZ * (NUM_VERT - 1)]);
	lc.push_back(cornersl[NUM_HORIZ * NUM_VERT - 1]);

	vector<Point2f> rc;
	rc.push_back(cornersr[0]);
	rc.push_back(cornersr[NUM_HORIZ - 1]);
	rc.push_back(cornersr[NUM_HORIZ * (NUM_VERT - 1)]);
	rc.push_back(cornersr[NUM_HORIZ * NUM_VERT - 1]);

	// Get corner world coordinates
	double l = 3.88636;
	vector<Point2f> wc;
	Point2f c(0, 0);
	wc.push_back(c);
	c = Point2f(l * (NUM_HORIZ - 1), 0);
	wc.push_back(c);
	c = Point2f(0, l * (NUM_VERT - 1));
	wc.push_back(c);
	c = Point2f(l * (NUM_HORIZ - 1), l * (NUM_VERT - 1));
	wc.push_back(c);

	// Load all the data for the stereo camera configuration
	Mat R = Mat(Size(3, 3), CV_64FC1);
	Mat T = Mat(Size(1, 3), CV_64FC1);
	Mat cam_matl(Size(3, 3), CV_64F);
	Mat cam_matr(Size(3, 3), CV_64F);
	Mat coeffl(Size(5, 1), CV_64F);
	Mat coeffr(Size(5, 1), CV_64F);

	readMatFromFile(R, "cam_data/R.txt");
	readMatFromFile(T, "cam_data/T.txt");
	readMatFromFile(cam_matl, "cam_data/cam_matl.txt");
	readMatFromFile(coeffl, "cam_data/coeffl.txt");
	readMatFromFile(cam_matr, "cam_data/cam_matr.txt");
	readMatFromFile(coeffr, "cam_data/coeffr.txt");

	Mat R1, R2, P1, P2, Q;
	stereoRectify(cam_matl, coeffl, cam_matr, coeffr, il.size(), R, T, R1, R2, P1, P2, Q);

	undistortPoints(lc, lc, cam_matl, coeffl, R1, P1);
	undistortPoints(rc, rc, cam_matr, coeffr, R2, P2);

	Mat lmap1, lmap2;
	Mat rmap1, rmap2;
	initUndistortRectifyMap(cam_matl, coeffl, R1, P1, il.size(), CV_32FC1, lmap1, lmap2);
	initUndistortRectifyMap(cam_matr, coeffr, R2, P2, ir.size(), CV_32FC1, rmap1, rmap2);

	Mat lrect, rrect;
	remap(il, il, lmap1, lmap2, INTER_NEAREST);
	remap(ir, ir, rmap1, rmap2, INTER_NEAREST);

	Mat ilc, irc;
	cvtColor(il, ilc, CV_GRAY2BGR);
	cvtColor(ir, irc, CV_GRAY2BGR);

	for (int i = 0; i < lc.size(); i++)
	{
		circle(ilc, lc[i], 4, Scalar(0, 255, 0), 2);
		circle(irc, rc[i], 4, Scalar(0, 255, 0), 2);
	}
	imwrite("data/l_corner.jpg", ilc);
	imwrite("data/r_corner.jpg", irc);

	vector<Point3f> lc2, wc2, rc2, wc3;
	for (int i = 0; i < lc.size(); i++)
	{
		lc2.push_back(Point3f(lc[i].x, lc[i].y, lc[i].x-rc[i].x));
	}

	for (int i = 0; i < rc.size(); i++)
	{
		rc2.push_back(Point3f(rc[i].x, rc[i].y, lc[i].x - rc[i].x));
	}

	perspectiveTransform(lc2, wc2, Q);
	perspectiveTransform(rc2, wc3, Q);

	// Write coordinates to file
	ofstream out;
	out.open("data/corners.txt");
	out.precision(8);
	if (out.is_open())
	{
		for (auto p : wc2)
		{
			out << p.x << " " << p.y << " " << p.z << endl;
		}

		out << endl;

		for (auto p : wc3)
		{
			out << p.x << " " << p.y << " " << p.z << endl;
		}
	}
	out.close();
}

int main()
{
	get3DMeasurement();

	auto st = StereoTracker();
	st.initClassMatrix();
	vector<Point3f> ball_loc;
	for (int i = 34; i <= 67; i++)
	{
		string lp = "bb/L" + to_string(i) + ".bmp";
		string rp = "bb/R" + to_string(i) + ".bmp";
		Mat l = imread(lp, CV_LOAD_IMAGE_GRAYSCALE);
		Mat r = imread(rp, CV_LOAD_IMAGE_GRAYSCALE);
		imshow("l", l);
		imshow("r", r);
		st.updateFrames(l, r);
		st.findBall();
		if(st.foundBall())
			st.calculate3DBallLocation();

		waitKey(0);
	}

	ball_loc = st.get3DPoints();
	Point2d xy = st.estimateEndPoint();
	ball_loc.push_back(Point3f(xy.x, xy.y, 0));


	ofstream out;
	out.open("data/ball_loc.txt");
	out.precision(8);
	if (out.is_open())
	{
		for (auto p : ball_loc)
		{
			out << p.x << " " << p.y << " " << p.z << endl;
		}

		out << endl;
	}
	else
		cout << "Unable to open ball location" << endl;
	out.close();

	system("pause");
}