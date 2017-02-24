package edu.byu.rvl.myopencvapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.JavaCameraView;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;

import java.util.ArrayList;

public class MyOpenCVActivity extends Activity implements View.OnTouchListener, CvCameraViewListener2 {
    private static final String  TAG = "Sample::MyOpenCV::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    static final int 				N_BUFFERS = 2;
    static final int				NUM_FINGERS = 2;
    public static int           	viewMode = 0;
    public static int[]			    TouchX, TouchY;
    public static float				StartX, StartY;
    public static int				actionCode;
    public static int				pointerCount = 0;
    public static int				inputValue = 0;

    private ArrayList<String> MenuItems = new ArrayList<String>();
    Mat mRgba[];
    Mat mHSV;
    Mat mChannel;
    Mat mDisplay;
    int	bufferIndex;
    int FrameHeight;
    int FrameWidth;

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                    mOpenCvCameraView.setOnTouchListener(MyOpenCVActivity.this);
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Log.d(TAG, "Creating and setting view");
        mOpenCvCameraView = (CameraBridgeViewBase) new JavaCameraView(this, -1);
        setContentView(mOpenCvCameraView);
        mOpenCvCameraView.setCvCameraViewListener(this);
    }

    @Override
    public void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_6, this, mLoaderCallback);
    }

    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        int i;
        TouchX = new int[NUM_FINGERS];
        TouchY = new int[NUM_FINGERS];
        inputValue = 0;
        bufferIndex = 0;
        FrameHeight = height;
        FrameWidth = width;

        mRgba = new Mat[N_BUFFERS];
        for (i=0; i<N_BUFFERS; i++) {
            mRgba[i]= new Mat(FrameHeight, FrameWidth, CvType.CV_8UC4);
        }
        mDisplay= new Mat();
        mHSV= new Mat();
        mChannel = new Mat();

        MenuItems.add("RGB");
        MenuItems.add("Difference");
        MenuItems.add("Red");
        MenuItems.add("Green");
        MenuItems.add("Blue");
        MenuItems.add("Hue");
        MenuItems.add("Saturation");
        MenuItems.add("Intensity");
    }

    public void onCameraViewStopped() {

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_my_open_cv, menu);

        Log.i(TAG, "called onCreateOptionsMenu");
        menu.add(MenuItems.get(0));
        menu.add(MenuItems.get(1));
        menu.add(MenuItems.get(2));
        menu.add(MenuItems.get(3));
        menu.add(MenuItems.get(4));
        menu.add(MenuItems.get(5));
        menu.add(MenuItems.get(6));
        menu.add(MenuItems.get(7));
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }
        Log.i(TAG, "Selected: " + item);

        viewMode = MenuItems.indexOf(item.toString());
        mDisplay= new Mat(FrameHeight, FrameWidth, CvType.CV_8UC4);

        return super.onOptionsItemSelected(item);
    }

    public boolean onTouch(View v, MotionEvent event) {
        int i;
        pointerCount = event.getPointerCount();
        actionCode = event.getAction();
        if (actionCode == MotionEvent.ACTION_DOWN) {								// get the starting location from the first touch
            StartX = event.getX(0);
            StartY = event.getY(0);
            for(i=0; i<pointerCount && i<NUM_FINGERS; i++) {						// get locations for up to to 5 touches
                TouchX[i] = (int)event.getX(i);
                TouchY[i] = (int)event.getY(i);
            }
        } else if (actionCode == MotionEvent.ACTION_MOVE) {
            for(i=0; i<pointerCount && i<NUM_FINGERS; i++) {						// get locations for up to to 5 touches
                TouchX[i] = (int)event.getX(i);
                TouchY[i] = (int)event.getY(i);
                inputValue = (int)(TouchX[0] - StartX);
            }
        } else if (actionCode == MotionEvent.ACTION_UP && pointerCount > 0) {		// update the distance
            inputValue = (int)(TouchX[0] - StartX);
        }
        return true;
    }

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        switch (viewMode) {
            case 0:         // RGB
                mDisplay = inputFrame.rgba();
                break;
            case 1:         // Difference
                mRgba[bufferIndex] = inputFrame.rgba();
                Core.absdiff(mRgba[bufferIndex], mRgba[1-bufferIndex], mDisplay);
                bufferIndex = 1 - bufferIndex;
                break;
            case 2:         // R
                Core.extractChannel(inputFrame.rgba(), mChannel, 0);     // R
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
            case 3:         // G
                Core.extractChannel(inputFrame.rgba(), mChannel, 1);     // G
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
            case 4:         // B
                Core.extractChannel(inputFrame.rgba(), mChannel, 2);     // B
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
            case 5:         // H
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 0);         // hue
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
            case 6:         // S
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 1);         // saturation
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
            case 7:         // V
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 2);         // intensity
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                break;
        }
        // Use Core.putText in 2.4.10   Imgproc.putText in 3.1.0 doesn't work
        if (pointerCount == 1) Core.putText(mDisplay, String.valueOf(inputValue), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(255, 0, 0, 255), 2);
        if (pointerCount == 2) {
            Core.putText(mDisplay, String.valueOf(inputValue), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 255, 0, 255), 2);
            Core.rectangle(mDisplay, new Point(TouchX[0], TouchY[0]), new Point(TouchX[1], TouchY[1]), new Scalar(0, 255, 0, 255), 2);
        }
        return mDisplay;
    }

}
