#include "stdafx.h"
#include "time.h"
#include "math.h"
#include "Hardware.h"

ImagingResources	CTCSys::IR;

CTCSys::CTCSys()
{
	EventEndProcess = TRUE;
	IR.Acquisition = TRUE;
	IR.UpdateImage = TRUE;
	IR.Inspection = FALSE;
	OPENF("c:\\Projects\\RunTest.txt");
}

CTCSys::~CTCSys()
{
	CLOSEF;
}

void CTCSys::QSStartThread()
{
	EventEndProcess = FALSE;
	//QSProcessEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	// Image Processing Thread
	QSProcessThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSProcessThreadFunc,
		this, NULL, (LPDWORD) &QSProcessThreadHandleID);
	ASSERT(QSProcessThreadHandle != NULL);
	SetThreadPriority(QSProcessThreadHandle, THREAD_PRIORITY_HIGHEST);
}

void CTCSys::QSStopThread()
{
	// need to make sure camera acquisiton has stopped
	EventEndProcess = TRUE;
	do { 
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSProcessThreadHandle);
}

long QSProcessThreadFunc(CTCSys *QS)
{
	int		i;
	int		pass = 0;
	while (QS->EventEndProcess == FALSE) {

#ifdef PTGREY
		if (QS->IR.Acquisition == TRUE) {
			for(i=0; i < QS->IR.NumCameras; i++) {
				if (QS->IR.pgrCamera[i]->RetrieveBuffer(&QS->IR.PtGBuf[i]) == PGRERROR_OK) {
					QS->QSSysConvertToOpenCV(&QS->IR.AcqBuf[i], QS->IR.PtGBuf[i]);	
				}
			}
			for(i=0; i < QS->IR.NumCameras; i++) {
#ifdef PTG_COLOR
				mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.ProcBuf[i], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
				QS->IR.AcqBuf[i].copyTo(QS->IR.ProcBuf[i][BufID]);	// Has to copy out of acquisition buffer before processing
#endif
			}
		}
#else
		Sleep (200);
#endif
		// Process Image ProcBuf
		if (QS->IR.Inspection) {
			// Images are acquired into ProcBuf{0] 
			// May need to create child image for processing to exclude background and speed up processing
	
			for(i=0; i < QS->IR.NumCameras; i++) {
				// ########## THIS IS WHERE WE ADDED OUR CODE ##########
				// Input is QS->IR.ProcBuf[i].
				// Output is QS->IR.OutBuf1[i] (what is displayed after inspection button)
				cvtColor(QS->IR.ProcBuf[i], QS->IR.OutBuf1[i], CV_RGB2GRAY);
				//Canny(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], 70, 100); // was already here as an example

				// threshold params
				int threshold_value = 128;
				int max_binary_value = 255;
				int threshold_type = 0;
				string trackbar_value = "value";

				// canny params
				int low_canny = 50;
				int ratio = 3;
				int max_canny_value = 100;
				int kernel_size = 5;

				// morphology params
				int operation = 3;  // 3 for Closing operation
				int morph_elem = 0;   // 0 for square kernel
				int morph_size = 9;   // kernel size
				int erosion_elem = 0;
				int erosion_size = 3;

				// apply threshold
				threshold(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], threshold_value, max_binary_value, threshold_type);
				
				// apply morphology
				Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));
				morphologyEx(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], operation, element);
				
				// apply erode
				Mat element1 = getStructuringElement(MORPH_RECT, Size(2 * erosion_size + 1, 2 * erosion_size + 1), Point(erosion_size, erosion_size));
				erode(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], element1);
				
				// apply canny
				Canny(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], low_canny, low_canny*ratio, kernel_size);
				
				// find contours
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(QS->IR.OutBuf1[i], contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); // Jae had EXTERNAL, Jeff tried TREE
				
				// draw on original image if the contour area is larger than a threshold
				Scalar color = Scalar(0, 0, 0);
				int k = 0;
				int j_cookie = 0;
				for (int j = 0; j<contours.size(); j++)
				{
					double temp_area = contourArea(contours.at(j));
					if (temp_area>5000) // PLOT & INSPECT AREA THRESH HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
					{
						// current selected contour, j, is not an artifact and is large enough to display
						drawContours(QS->IR.ProcBuf[i], contours, j, color, 2, 8, hierarchy, 0, Point());
						k=k+1; // counter of contours > area thresh
						j_cookie = j; // remember which contour represented the cookie
					}
				}
				
				


				// perform pass/fail logic
				if (k > 1)
				{
					// at least 1 contour had area exceeding the threshold
					int area = (int)contourArea(contours.at(j_cookie));
					string disp_area = to_string(area);
					putText(QS->IR.ProcBuf[0], disp_area, Point(10, 200), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
					if (area > 13500) // GOOD & BAD AREA THRESH HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
					{
						// cookie is considered good
						pass = 3;
					}
					else if (area > 11500)
					{
						// cookie is considered ugly
						pass = 2;
					}
					else
					{
						// between 5000 and 10000
						// cookie is considered bad
						pass = 1;
					}

				}
				else
				{
					// no contour was large enough to inspect
					pass = 0; // display nothing
					
				}
				



				// the only way noob Jeff could get this to copy to OutBuf1, other methods mess up the pointers
				cvtColor(QS->IR.ProcBuf[i], QS->IR.OutBuf1[i], COLOR_RGB2GRAY);
				


				 
				// other failed attempts by Jeff to copy to OutBuf1
				//cvtColor(QS->IR.ProcBuf[i], QS->IR.OutBuf1[i], COLOR_RGB2BGR); // convert back and forth #fail
				//cvtColor(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], COLOR_BGR2RGB);
				//QS->IR.ProcBuf[i].copyTo(QS->IR.OutBuf1[i]); // this nor clone() work either

				


				// original cookie logic from Jae
				// (we probably need to take out cookie index the gui is designed to 1 pass/fail at a time
				/*
				double cookie_area = 0;
				int cookie_index = 0;
				for (int i = 0; i < contours.size(); i++){
					double temp_area = contourArea(contours.at(i));
					cout << "temp area: " << temp_area << endl;
					if (temp_area > cookie_area){
						cookie_area = temp_area;
						cookie_index = i;
					}
				}
				cout << "cookie index: " << cookie_index << endl;
				cout << "cookie area: " << cookie_area << endl;
				Scalar color = Scalar(0, 255, 0);
				*/

				// ########## ABOVE HERE IS WHERE WE ADDED OUR CODE ##########

			}

		}
		// Display Image
		if (QS->IR.UpdateImage) {
			for (i=0; i<QS->IR.NumCameras; i++) {
				if (!QS->IR.Inspection) {
					// Example of displaying color buffer ProcBuf
					QS->IR.ProcBuf[i].copyTo(QS->IR.DispBuf[i]);
				} else {
					// Example of displaying B/W buffer OutBuf1
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = QS->IR.OutROI1[i];
					merge(QS->IR.OutBuf, 3, QS->IR.DispROI[i]);
					// Example to show inspection result, print result after the image is copied
					QS->QSSysPrintResult(pass);
				}
			}
			QS->QSSysDisplayImage();
		}
	} 
	QS->EventEndProcess = FALSE;
	return 0;
}

void CTCSys::QSSysInit()
{
	long i;
	IR.DigSizeX = 640;
	IR.DigSizeY = 480;
	initBitmapStruct(IR.DigSizeX, IR.DigSizeY);
	// Camera Initialization
#ifdef PTGREY
	IR.cameraConfig.asyncBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.isochBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.grabMode = DROP_FRAMES;			// take the last one, block grabbing, same as flycaptureLockLatest
	IR.cameraConfig.grabTimeout = TIMEOUT_INFINITE;	// wait indefinitely
	IR.cameraConfig.numBuffers = 4;					// really does not matter since DROP_FRAMES is set not to accumulate buffers

	// How many cameras are on the bus?
	if(IR.busMgr.GetNumOfCameras((unsigned int *)&IR.NumCameras) != PGRERROR_OK){	// something didn't work correctly - print error message
		AfxMessageBox(L"Connect Failure", MB_ICONSTOP);
	} else {
		IR.NumCameras = (IR.NumCameras > MAX_CAMERA) ? MAX_CAMERA : IR.NumCameras;
		for(i = 0; i < IR.NumCameras; i++) {		
			// Get PGRGuid
			if (IR.busMgr.GetCameraFromIndex(0, &IR.prgGuid[i]) != PGRERROR_OK) {
				AfxMessageBox(L"PGRGuID Failure", MB_ICONSTOP);
			}
			IR.pgrCamera[i] = new Camera;
			if (IR.pgrCamera[i]->Connect(&IR.prgGuid[i]) != PGRERROR_OK) { 
				AfxMessageBox(L"PConnect Failure", MB_ICONSTOP);
			}
			// Set all camera configuration parameters
			if (IR.pgrCamera[i]->SetConfiguration(&IR.cameraConfig) != PGRERROR_OK) { 
				AfxMessageBox(L"Set Configuration Failure", MB_ICONSTOP);
			}
			// Set video mode and frame rate
			if (IR.pgrCamera[i]->SetVideoModeAndFrameRate(VIDEO_FORMAT, CAMERA_FPS) != PGRERROR_OK) { 
				AfxMessageBox(L"Video Format Failure", MB_ICONSTOP);
			}
			// Sets the onePush option off, Turns the control on/off on, disables auto control.  These are applied to all properties.
			IR.cameraProperty.onePush = false;
			IR.cameraProperty.autoManualMode = false;
			IR.cameraProperty.absControl = true;
			IR.cameraProperty.onOff = true;
			// Set shutter sppeed
			IR.cameraProperty.type = SHUTTER;
			IR.cameraProperty.absValue = SHUTTER_SPEED;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				AfxMessageBox(L"Shutter Failure", MB_ICONSTOP);
			}
#ifdef  PTG_COLOR
			// Set white balance (R and B values)
			IR.cameraProperty = WHITE_BALANCE;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = WHITE_BALANCE_R;
			IR.cameraProperty.valueB = WHITE_BALANCE_B;
//			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
//				AfxMessageBox(L"White Balance Failure", MB_ICONSTOP);
//			}
#endif
			// Set gain values (350 here gives 12.32dB, varies linearly)
			IR.cameraProperty = GAIN;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = GAIN_VALUE_A;
			IR.cameraProperty.valueB = GAIN_VALUE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				AfxMessageBox(L"Gain Failure", MB_ICONSTOP);
			}
			// Set trigger state
			IR.cameraTrigger.mode = 0;
			IR.cameraTrigger.onOff = TRIGGER_ON;
			IR.cameraTrigger.polarity = 0;
			IR.cameraTrigger.source = 0;
			IR.cameraTrigger.parameter = 0;
			if(IR.pgrCamera[i]->SetTriggerMode(&IR.cameraTrigger, false) != PGRERROR_OK){	
				AfxMessageBox(L"Trigger Failure", MB_ICONSTOP);
			}
			// Start Capture Individually
			if (IR.pgrCamera[i]->StartCapture() != PGRERROR_OK) {
				char Msg[128];
				sprintf_s(Msg, "Start Capture Camera %d Failure", i);
				AfxMessageBox(CA2W(Msg), MB_ICONSTOP);
			}
		}
		// Start Sync Capture (only need to do it with one camera)
//		if (IR.pgrCamera[0]->StartSyncCapture(IR.NumCameras, (const Camera**)IR.pgrCamera, NULL, NULL) != PGRERROR_OK) {
//			AfxMessageBox(L"Start Sync Capture Failure", MB_ICONSTOP);
//		}
	}
#else
	IR.NumCameras = MAX_CAMERA;
#endif
	Rect R = Rect(0, 0, IR.DigSizeX, IR.DigSizeY);
	// create openCV image
	for(i=0; i<IR.NumCameras; i++) {
#ifdef PTG_COLOR
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
#else 
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][0].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][1].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
#endif
		IR.AcqPtr[i] = IR.AcqBuf[i].data;
		IR.DispROI[i] = IR.DispBuf[i](R); 

		IR.OutBuf1[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI1[i] = IR.OutBuf1[i](R); 
		IR.OutBuf2[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI2[i] = IR.OutBuf2[i](R); 
		IR.DispBuf[i] = Scalar(0);
		IR.ProcBuf[i] = Scalar(0);
	}
	IR.from_to[0] = 0;
	IR.from_to[1] = 2;
	IR.from_to[2] = 1;
	IR.from_to[3] = 1;
	IR.from_to[4] = 2;
	IR.from_to[5] = 0;
	QSStartThread();
}

void CTCSys::QSSysFree()
{
	QSStopThread(); // Move to below PTGREY if on Windows Vista
#ifdef PTGREY
	for(int i=0; i<IR.NumCameras; i++) {
		if (IR.pgrCamera[i]) {
			IR.pgrCamera[i]->StopCapture();
			IR.pgrCamera[i]->Disconnect();
			delete IR.pgrCamera[i];
		}
	}
#endif
}

void CTCSys::initBitmapStruct(long iCols, long iRows)
{
	m_bitmapInfo.bmiHeader.biSize			= sizeof( BITMAPINFOHEADER );
	m_bitmapInfo.bmiHeader.biPlanes			= 1;
	m_bitmapInfo.bmiHeader.biCompression	= BI_RGB;
	m_bitmapInfo.bmiHeader.biXPelsPerMeter	= 120;
	m_bitmapInfo.bmiHeader.biYPelsPerMeter	= 120;
    m_bitmapInfo.bmiHeader.biClrUsed		= 0;
    m_bitmapInfo.bmiHeader.biClrImportant	= 0;
    m_bitmapInfo.bmiHeader.biWidth			= iCols;
    m_bitmapInfo.bmiHeader.biHeight			= -iRows;
    m_bitmapInfo.bmiHeader.biBitCount		= 24;
	m_bitmapInfo.bmiHeader.biSizeImage = 
      m_bitmapInfo.bmiHeader.biWidth * m_bitmapInfo.bmiHeader.biHeight * (m_bitmapInfo.bmiHeader.biBitCount / 8 );
}

void CTCSys::QSSysDisplayImage()
{
	SetDIBitsToDevice(ImageDC[0]->GetSafeHdc(), 1, 1,
		m_bitmapInfo.bmiHeader.biWidth,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		0, 0, 0,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		IR.DispBuf[0].data,
		&m_bitmapInfo, DIB_RGB_COLORS);
}


#ifdef PTGREY
void CTCSys::QSSysConvertToOpenCV(Mat* openCV_image, Image PGR_image)
{
	openCV_image->data = PGR_image.GetData();	// Pointer to image data
	openCV_image->cols = PGR_image.GetCols();	// Image width in pixels
	openCV_image->rows = PGR_image.GetRows();	// Image height in pixels
	openCV_image->step = PGR_image.GetStride(); // Size of aligned image row in bytes
}
#endif

void CTCSys::QSSysPrintResult(int pass)
{
	if (pass == 3)
	{
		// good
		putText(IR.DispBuf[0],  "Good" , Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
	}
	else if (pass==2)
	{
		// ugly
		putText(IR.DispBuf[0], "Ugly", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
	}
	else if (pass == 1)
	{
		// bad
		putText(IR.DispBuf[0], "Bad", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
	}
	else
	{
		// no contour large enough, nothing evaluated
		putText(IR.DispBuf[0], "Nothing", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
	}

	// if pass = 0, nothing is evaluated and nothing is displayed

	//putText(IR.DispBuf[0], (pass) ? "Pass" : "Fail", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2); // original syntax with bool input
}
