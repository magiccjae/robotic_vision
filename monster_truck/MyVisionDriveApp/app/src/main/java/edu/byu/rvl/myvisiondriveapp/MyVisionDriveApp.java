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

    private ArrayList<String> MenuItems = new ArrayList<String>();
    Mat mRgba[];
    Mat mHSV;
    Mat mChannel;
    Mat mDisplay;
    int	bufferIndex;
    int FrameHeight;
    int FrameWidth;

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

                // populate each grid cell with content from image
                // AND evaluate if it is occupied, if occupied add to running sum
                // CODE HERE <<<<<< --------



                // convert final result of running sum into a steering value
                // CODE HERE <<<<<< --------





                // set the steering and power based on visual processing results
                MYinputValueX = 0; // steering    -1500 to 1500 positive is left, negative is right
                MYinputValueY = 100; // power     -2500 to 2500

                // display the binarized occupancy grid real-time
                mDisplay = inputFrame.rgba();


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
        if (pointerCount == 1) Core.putText(mDisplay, String.valueOf(inputValueX) + " " + String.valueOf(inputValueY), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(255, 0, 0, 255), 2);
        if (pointerCount == 2) {
            Core.putText(mDisplay, String.valueOf(inputValueX) + " " + String.valueOf(inputValueY), new Point(10, 50), Core.FONT_HERSHEY_COMPLEX, 2.0, new Scalar(0, 255, 0, 255), 2);
            Core.rectangle(mDisplay, new Point(TouchX[0], TouchY[0]), new Point(TouchX[1], TouchY[1]), new Scalar(0, 255, 0, 255), 2);
        }
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
        private PulseInput encoderInput_;   // pulse input to measure speed

        @Override
        protected void setup() throws ConnectionLostException {
            showVersions(ioio_, "IOIO connected!");
            led_ = ioio_.openDigitalOutput(0, true);
            turnOutput_ = ioio_.openPwmOutput(12, 100);     // Hard Left: 2000, Straight: 1400, Hard Right: 1000
            pwrOutput_ = ioio_.openPwmOutput(14, 100);      // Fast Forward: 2500, Stop: 1540, Fast Reverse: 500
            encoderInput_ = ioio_.openPulseInput(3, PulseInput.PulseMode.FREQ);
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
