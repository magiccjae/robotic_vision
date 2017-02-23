package edu.byu.rvl.myioioapp;

import android.content.Context;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import ioio.lib.api.DigitalOutput;
import ioio.lib.api.IOIO;
import ioio.lib.api.PulseInput;
import ioio.lib.api.PwmOutput;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.util.BaseIOIOLooper;
import ioio.lib.util.IOIOLooper;
import ioio.lib.util.android.IOIOActivity;

public class MyIOIOActivity extends IOIOActivity {
    boolean LED = false;
    float   Frequency;
    private TextView textView_steering;
    private TextView textView_power;
    private TextView textView_speed;
    private SeekBar seekBar_turn;
    private SeekBar seekBar_power;
    private ToggleButton toggleButton_;

    public static final int  STEERING_MAX = 1000;
    public static final int  POWER_MAX = 2000;
    public static final int  STEERING_OFF = 1000;
    public static final int  POWER_OFF = 500;

    private PulseInput encoderInput_;     // pulse input to measure speed

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my_ioio);
        textView_steering = (TextView) findViewById(R.id.SteeringView);
        textView_power = (TextView) findViewById(R.id.PowerView);
        textView_speed = (TextView) findViewById(R.id.SpeedView);
        seekBar_turn = (SeekBar) findViewById(R.id.SeekBarTurn);
        seekBar_power = (SeekBar) findViewById(R.id.SeekBarPower);
        toggleButton_ = (ToggleButton) findViewById(R.id.LEDButton);

        seekBar_turn.setMax(STEERING_MAX);
        seekBar_turn.setProgress(STEERING_MAX/2);
        seekBar_power.setMax(POWER_MAX);
        seekBar_power.setProgress(POWER_MAX/2);

        seekBar_turn.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                textView_steering.setText("::" + (progress+STEERING_OFF));
            }
            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }
            public void onStopTrackingTouch(SeekBar seekBar) {
                seekBar_turn.setProgress(STEERING_MAX/2);
            }
        });

        seekBar_power.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                textView_power.setText("::" + (progress+POWER_OFF));
            }
            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }
            public void onStopTrackingTouch(SeekBar seekBar) {
                seekBar_power.setProgress(POWER_MAX/2);
                try {
                    setNumber(encoderInput_.getFrequency());
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } catch (ConnectionLostException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_my_ioio, menu);
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

        return super.onOptionsItemSelected(item);
    }

    /**
     * This is the thread on which all the IOIO activity happens. It will be run
     * every time the application is resumed and aborted when it is paused. The
     * method setup() will be called right after a connection with the IOIO has
     * been established (which might happen several times!). Then, loop() will
     * be called repetitively until the IOIO gets disconnected.
     */
    class Looper extends BaseIOIOLooper {
        /** The on-board LED. */
        private DigitalOutput led_;
        private PwmOutput turnOutput_;		// pwm output for turn motor
        private PwmOutput pwrOutput_;		// pwm output for drive motor
//        private PulseInput encoderInput_;     // pulse input to measure speed
        /**
         * Called every time a connection with IOIO has been established.
         * Typically used to open pins.
         *
         * @throws ConnectionLostException
         *             When IOIO connection is lost.
         *
         * @see ioio.lib.util.IOIOLooper#setup()
         */
        @Override
        protected void setup() throws ConnectionLostException {
            showVersions(ioio_, "IOIO connected!");
            led_ = ioio_.openDigitalOutput(0, true);
            turnOutput_ = ioio_.openPwmOutput(12, 100);     // Hard Left: 2000, Straight: 1400, Hard Right: 1000
            pwrOutput_ = ioio_.openPwmOutput(14, 100);      // Fast Forward: 2500, Stop: 1540, Fast Reverse: 500
            encoderInput_ = ioio_.openPulseInput(3, PulseInput.PulseMode.FREQ);
            enableUi(true);
        }

        /**
         * Called repetitively while the IOIO is connected.
         *
         * @throws ConnectionLostException
         *             When IOIO connection is lost.
         * @throws InterruptedException
         * 				When the IOIO thread has been interrupted.
         *
         * @see ioio.lib.util.IOIOLooper#loop()
         */
        @Override
        public void loop() throws ConnectionLostException, InterruptedException {
            LED = !toggleButton_.isChecked();
            led_.write(LED);
            turnOutput_.setPulseWidth(STEERING_OFF + seekBar_turn.getProgress());   // Offset by 1000
            pwrOutput_.setPulseWidth(POWER_OFF + seekBar_power.getProgress());    // offset by 500
            Thread.sleep(100);
        }

        /**
         * Called when the IOIO is disconnected.
         *
         * @see ioio.lib.util.IOIOLooper#disconnected()
         */
        @Override
        public void disconnected() {
            toast("IOIO disconnected");
        }

        /**
         * Called when the IOIO is connected, but has an incompatible firmware version.
         *
         * @see ioio.lib.util.IOIOLooper#incompatible(IOIO)
         */
        @Override
        public void incompatible() {
            showVersions(ioio_, "Incompatible firmware version!");
        }
    }

    /**
     * A method to create our IOIO thread.
     *
     * @see ioio.lib.util.AbstractIOIOActivity#createIOIOThread()
     */
    @Override
    protected IOIOLooper createIOIOLooper() {
        return new Looper();
    }

    private void enableUi(final boolean enable) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                seekBar_turn.setEnabled(enable);
                seekBar_power.setEnabled(enable);
                toggleButton_.setEnabled(enable);
            }
        });
    }

    private void setNumber(float frequency) {
 //       final String str = String.format("%.2f", f);
        final String str = String.format("%.2f", frequency);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                textView_speed.setText(str);
            }
        });
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
