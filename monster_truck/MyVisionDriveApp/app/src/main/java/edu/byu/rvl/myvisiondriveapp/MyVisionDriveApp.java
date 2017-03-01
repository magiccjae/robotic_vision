package edu.byu.rvl.myvisiondriveapp;

import android.content.Context;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

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
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import java.util.ArrayList;

import ioio.lib.api.DigitalOutput;
import ioio.lib.api.IOIO;
import ioio.lib.api.PulseInput;
import ioio.lib.api.PwmOutput;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.util.BaseIOIOLooper;
import ioio.lib.util.IOIOLooper;
import ioio.lib.util.android.IOIOActivity;
import ioio.lib.spi.Log;

import static org.opencv.imgproc.Imgproc.INTER_NEAREST;

public class MyVisionDriveApp extends IOIOActivity implements View.OnTouchListener, CvCameraViewListener2 {
    private CameraBridgeViewBase mOpenCvCameraView;
    static final int 				N_BUFFERS = 2;
    static final int				NUM_FINGERS = 2;
    public static int           	viewMode = 1;
    public static int[]			    TouchX, TouchY;
    public static float				StartX, StartY;
    public static int				actionCode;
    public static int				pointerCount = 0;
    public static int				inputValueX = 0;
    public static int				inputValueY = 0;
    public static int				MYinputValueX = 0;
    public static int				MYinputValueY = 0;
    private static final String     TAG = "JeffsMessage"; // tip from Dallin about debugging

    public PulseInput encoderVar;

    private ArrayList<String> MenuItems = new ArrayList<String>();
    Mat mRgba[];
    Mat mHSV;
    Mat mChannel;
    Mat mDisplay;
    Mat cur_image;
    Mat cur_image_mod;
    Mat weights_table;
    Mat weights_table_back;
    Mat weights;
    Mat weights_back;
    Mat weights_abs;
    Size sz;
    int grid_rows;
    int grid_columns;
    int thresh;
    int	bufferIndex;
    int FrameHeight;
    int FrameWidth;
    Scalar sum_out;
    Scalar zz;
    double steer_score;
    double weights_scale;

    Mat stop_table;

    // IOIO Control
    boolean LED = false;
    int     Frequency;
    int     SteerOutput;
    int     PowerOutput;
    boolean IOIO_Setup = false;

    public static final int  STEER_MAX = 1000;
    public static final int  POWER_MAX = 2000;
    public static final int  STEER_OFF = 1000;
    public static final int  POWER_OFF = 500;

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    mOpenCvCameraView.enableView();
                    mOpenCvCameraView.setOnTouchListener(MyVisionDriveApp.this);
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mOpenCvCameraView = (CameraBridgeViewBase) new JavaCameraView(this, -1);
        setContentView(mOpenCvCameraView);
        mOpenCvCameraView.setCvCameraViewListener(this);
        Log.i(TAG,"onCreate");
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
        inputValueX = 0;
        inputValueY = 0;
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

        MenuItems.add("Grid");
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

    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_my_vision_drive, menu);

        MenuItems.add("Grid");
        MenuItems.add("RGB");
        MenuItems.add("Difference");
        MenuItems.add("Red");
        MenuItems.add("Green");
        MenuItems.add("Blue");
        MenuItems.add("Hue");
        MenuItems.add("Saturation");
        MenuItems.add("Intensity");

        menu.add(MenuItems.get(0));
        menu.add(MenuItems.get(1));
        menu.add(MenuItems.get(2));
        menu.add(MenuItems.get(3));
        menu.add(MenuItems.get(4));
        menu.add(MenuItems.get(5));
        menu.add(MenuItems.get(6));
        menu.add(MenuItems.get(7));
        menu.add(MenuItems.get(8));
        return true;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        viewMode = MenuItems.indexOf(item.toString());
        mDisplay= new Mat(FrameHeight, FrameWidth, CvType.CV_8UC4);
        cur_image = new Mat(FrameHeight, FrameWidth, CvType.CV_8UC4);
        cur_image_mod = new Mat();
        weights_abs = new Mat();
        weights_table = new Mat(9,16, CvType.CV_64FC1);
        weights_table_back = new Mat(9,16, CvType.CV_64FC1);
        weights_table.put(0, 0, // row and column number - leave at zero
        -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,
        -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,
        -3, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,
        -6, -4, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  4,  6,
        -7, -5, -4, -3,  0,  0,  0,  0,  0,  0,  0,  0,  3,  4,  5,  7,
        -9, -8, -7, -6, -5,  0,  0,  0,  0,  0,  0,  5,  6,  7,  8,  9,
        -9, -8, -7, -6, -5, -4,  0,  0,  0,  0,  4,  5,  6,  7,  8,  9,
        -9, -9, -8, -7, -6, -5, -5,  0,  0,  5,  5,  6,  7,  8,  9,  9,
        -9, -9, -9, -8, -8, -8, -7, -7,  7,  7,  8,  8,  8,  9,  9,  9);

        weights_table_back.put(0, 0, // row and column number - leave at zero
        0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  9,  9,  9,  9,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  9,  9,  9,  9,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  9,  9,  9,  9,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  9,  9,  9,  9,  0,  0,  0,  0,  0,  0,
        0, 0, 0, 0,  0,  0,  9,  9,  9,  9,  0,  0,  0,  0,  0,  0);

//        weights_table.put(0, 0, // row and column number - leave at zero
//         0,  0,  0,  0,  0,  0,  0, -1, 1, 0, 0, 0, 0, 0, 0, 0,
//         0,  0,  0,  0,  0,  0, -1, -2, 2, 1, 0, 0, 0, 0, 0, 0,
//         0,  0,  0,  0,  0, -1, -2, -3, 3, 2, 1, 0, 0, 0, 0, 0,
//         0,  0,  0,  0, -1, -2, -3, -4, 4, 3, 2, 1, 0, 0, 0, 0,
//         0,  0,  0, -1, -2, -3, -4, -5, 5, 4, 3, 2, 1, 0, 0, 0,
//         0,  0, -1, -2, -3, -4, -5, -6, 6, 5, 4, 3, 2, 1, 0, 0,
//         0, -1, -2, -3, -4, -5, -6, -7, 7, 6, 5, 4, 3, 2, 1, 0,
//        -1, -2, -3, -4, -5, -6, -7, -8, 8, 7, 6, 5, 4, 3, 2, 1,
//        -2, -3, -4, -5, -6, -7, -8, -9, 9, 8, 7, 6, 5, 4, 3, 2);

        stop_table = new Mat(9,16, CvType.CV_64FC1);
        stop_table.put(0, 0, // row and column number - leave at zero
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);



        thresh = 120;
        grid_rows = 18;
        grid_columns = 32;
        sz = new Size(grid_columns, grid_rows);
        weights = new Mat();
        weights_back = new Mat();


        // resize weights to same dimensions of grid
        Imgproc.resize(weights_table, weights, sz , 0, 0, Imgproc.INTER_CUBIC);
        Imgproc.resize(weights_table_back, weights_back, sz , 0, 0, Imgproc.INTER_CUBIC);

        // generate the weight scale
        zz = new Scalar(0,0,0);
        Core.absdiff(weights, zz, weights_abs);     // absolute value of weights_abs
        zz = Core.sumElems(weights_abs);            // total sum of abs elements
        weights_scale = zz.val[0];                  // extract the double of the total sum

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
                inputValueX = (int)(TouchX[0] - StartX);
                inputValueY = (int)(TouchY[0] - StartY);
            }
        } else if (actionCode == MotionEvent.ACTION_UP && pointerCount > 0) {		// update the distance
            inputValueX = (int)(TouchX[0] - StartX);
            inputValueY = (int)(TouchY[0] - StartY);
        }
        return true;
    }

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        switch (viewMode) {
            case 0:         // Grid

                // this mode is accessed via in-app menu "Grid" mode
                // the default mode is "RGB" which will not control the truck!

                 //////   ///////  //    // //////// ////////   ///////  //
                //    // //     // ///   //    //    //     // //     // //
                //       //     // ////  //    //    //     // //     // //
                //       //     // // // //    //    ////////  //     // //
                //       //     // //  ////    //    //   //   //     // //
                //    // //     // //   ///    //    //    //  //     // //
                 //////   ///////  //    //    //    //     //  ///////  ////////

                // inputFrame.rgba() is the current frame, mDisplay is the Mat displayed live
                cur_image = inputFrame.rgba();

                // populate each grid cell with content from image
                // AND evaluate if it is occupied, if occupied add to running sum

                // resize current frame to size of occupancy grid
                Imgproc.resize(cur_image, cur_image_mod, sz , 0, 0, Imgproc.INTER_NEAREST);

                // cur_image_mod is the color grid, now convert to HSV
                Imgproc.cvtColor(cur_image_mod, cur_image_mod, Imgproc.COLOR_RGB2HSV);

                // extract the saturation channel
                Core.extractChannel(cur_image_mod, cur_image_mod, 1);

                // apply the threshold
                Imgproc.threshold(cur_image_mod, cur_image_mod, thresh, 255, Imgproc.THRESH_BINARY);

                // scale the occupancy grid back up for display
                Imgproc.resize(cur_image_mod, mDisplay, mDisplay.size(), 0, 0, Imgproc.INTER_AREA );

                // scale cur_image_mod to 0-1
                Core.normalize(cur_image_mod, cur_image_mod, 1, 0, Core.NORM_MINMAX);

                // element-wise multiply the 0-1 grid by the weights
                cur_image_mod.convertTo(cur_image_mod, CvType.CV_64FC1); // conversion necessary
                Core.multiply(cur_image_mod, weights, cur_image_mod);

                // sum all the elements of the resulting weighted grid
                sum_out = Core.sumElems(cur_image_mod);
                steer_score = sum_out.val[0];

                // normalize the steer score based on the max possible magnitude
                //steer_score = steer_score/weights_scale;

                // NEED TO CONVERT THE SCORE (-1, 1 value) TO PWM BELOW

                // set the steering and power based on visual processing results

                MYinputValueX = (int)(steer_score*4);   // steering    -1500 to 1500 positive is left, negative is right
                // upper and lower limit for steering value
                int steer_limit = 500;
                if(MYinputValueX > steer_limit){
                    MYinputValueX = steer_limit;
                }
                else if(MYinputValueX < -steer_limit){
                    MYinputValueX = -steer_limit;
                }

                // upper and lower limit for SPEED
                int pup = 1000;
                int plow = 600;
                double slope = -(double)(pup-plow)/(double)steer_limit;

                float desired_speed = (float)(slope*Math.abs(MYinputValueX)+pup);

                Imgproc.cvtColor(mDisplay, mDisplay, Imgproc.COLOR_GRAY2RGB);
                Core.putText(mDisplay, "S " + Integer.toString(MYinputValueX), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 0, 255, 255), 3);

                // encoder value reading
                float encoder_frequency = 0;
                try {
                    encoder_frequency = encoderVar.getFrequency();
                }
                catch(InterruptedException e){
                    e.printStackTrace();
                }
                catch(ConnectionLostException e){
                    e.printStackTrace();
                }
                Core.putText(mDisplay, "Encoder " + Float.toString(encoder_frequency), new Point(10, 200), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 0, 255, 255), 3);

                // for testing
//                MYinputValueX = 0;
//                desired_speed = 500;
                // speed controller
                if(encoder_frequency < desired_speed){
                    MYinputValueY = MYinputValueY + 2;
                }
                else{
                    MYinputValueY = MYinputValueY - 10;
                }
                int power_max = 200;
                int power_min = 100;
                if(MYinputValueY > power_max){
                    MYinputValueY = power_max;
                }
                else if(MYinputValueY < power_min){
                    MYinputValueY = power_min;
                }
                Core.putText(mDisplay, "P " + Integer.toString(MYinputValueY), new Point(10, 100), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 0, 255, 255), 3);

                // display the binarized occupancy grid real-time
                //mDisplay = inputFrame.rgba();
                //Imgproc.cvtColor(cur_image_mod, mDisplay, Imgproc.COLOR_GRAY2RGB);

                break;
            case 1:         // RGB
                mDisplay = inputFrame.rgba();
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 2:         // Difference
                mRgba[bufferIndex] = inputFrame.rgba();
                Core.absdiff(mRgba[bufferIndex], mRgba[1-bufferIndex], mDisplay);
                bufferIndex = 1 - bufferIndex;
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 3:         // R
                Core.extractChannel(inputFrame.rgba(), mChannel, 0);     // R
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 4:         // G
                Core.extractChannel(inputFrame.rgba(), mChannel, 1);     // G
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 5:         // B
                Core.extractChannel(inputFrame.rgba(), mChannel, 2);     // B
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 6:         // H
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 0);         // hue
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 7:         // S
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 1);         // saturation
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
            case 8:         // V
                Imgproc.cvtColor(inputFrame.rgba(), mHSV, Imgproc.COLOR_RGB2HSV);
                Core.extractChannel(mHSV, mChannel, 2);         // intensity
                Imgproc.cvtColor(mChannel, mDisplay, Imgproc.COLOR_GRAY2RGB);
                // in this mode, kill the steering and power
                MYinputValueX = 0;
                MYinputValueY = 0;
                break;
        }
        // Use Core.putText in 2.4.10   Imgproc.putText in 3.1.0 doesn't work
//        if (pointerCount == 1) Core.putText(mDisplay, String.valueOf(inputValueX) + " " + String.valueOf(inputValueY), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(255, 0, 0, 255), 2);
//        if (pointerCount == 2) {
//            Core.putText(mDisplay, String.valueOf(inputValueX) + " " + String.valueOf(inputValueY), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 255, 0, 255), 2);
//            Core.rectangle(mDisplay, new Point(TouchX[0], TouchY[0]), new Point(TouchX[1], TouchY[1]), new Scalar(0, 255, 0, 255), 2);
//        }
        return mDisplay;
    }

    /**
     * This is the thread on which all the IOIO activity happens. It will be run
     * every time the application is resumed and aborted when it is paused. The
     * method setup() will be called right after a connection with the IOIO has
     * been established (which might happen several times!). Then, loop() will
     * be called repetitively until the IOIO gets disconnected.
     */
    class Looper extends BaseIOIOLooper {
        private DigitalOutput led_;
        private PwmOutput turnOutput_;		// pwm output for turn motor
        private PwmOutput pwrOutput_;		// pwm output for drive motor
        public PulseInput encoderInput_;   // pulse input to measure speed

        @Override
        protected void setup() throws ConnectionLostException {
            showVersions(ioio_, "IOIO connected!");
            led_ = ioio_.openDigitalOutput(0, true);
            turnOutput_ = ioio_.openPwmOutput(12, 100);     // Hard Left: 2000, Straight: 1400, Hard Right: 1000
            pwrOutput_ = ioio_.openPwmOutput(14, 100);      // Fast Forward: 2500, Stop: 1540, Fast Reverse: 500
            encoderInput_ = ioio_.openPulseInput(3, PulseInput.PulseMode.FREQ);
            encoderVar = encoderInput_;
        }

        @Override
        public void loop() throws ConnectionLostException, InterruptedException {
            LED = (pointerCount == 1);
            led_.write(LED);
            // STEER_MAX = 1000; // these values are set above
            // STEER_OFF = 1000;
            // POWER_MAX = 2000;
            // POWER_OFF = 500;
            turnOutput_.setPulseWidth(STEER_OFF + MYinputValueX + STEER_MAX/2);       // offset by 1000
            pwrOutput_.setPulseWidth(POWER_OFF + MYinputValueY + POWER_MAX/2);        // offset by 500
            Thread.sleep(100);
        }

        @Override
        public void disconnected() {
            toast("IOIO disconnected");
        }

        @Override
        public void incompatible() {
            showVersions(ioio_, "Incompatible firmware version!");
        }
    }

    protected IOIOLooper createIOIOLooper() {
        return new Looper();
    }

    private void showVersions(IOIO ioio, String title) {
        toast(String.format("%s\n" +
                        "IOIOLib: %s\n" +
                        "Application firmware: %s\n" +
                        "Bootloader firmware: %s\n" +
                        "Hardware: %s",
                title,
                ioio.getImplVersion(IOIO.VersionType.IOIOLIB_VER),
                ioio.getImplVersion(IOIO.VersionType.APP_FIRMWARE_VER),
                ioio.getImplVersion(IOIO.VersionType.BOOTLOADER_VER),
                ioio.getImplVersion(IOIO.VersionType.HARDWARE_VER)));
    }

    private void toast(final String message) {
        final Context context = this;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context, message, Toast.LENGTH_LONG).show();
            }
        });
    }

}
