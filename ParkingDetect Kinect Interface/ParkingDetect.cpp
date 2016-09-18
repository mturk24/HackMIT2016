#include "stdafx.h"
#include <strsafe.h>
#include <string>
#include <Python.h>
#include "resource.h"
#include "ParkingDetect.h"

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CParkingDetect application;
    application.Run(hInstance, nShowCmd);
}

/// <summary>
/// Constructor
/// </summary>
CParkingDetect::CParkingDetect() :
    m_hWnd(NULL),
    m_nStartTime(0),
    m_nLastCounter(0),
    m_nFramesSinceUpdate(0),
    m_fFreq(0),
    m_nNextStatusTime(0LL),
    m_bSave(0),
	m_bHasSpace(false),
    m_pKinectSensor(NULL),
    m_pDepthFrameReader(NULL),
	m_pColorFrameReader(NULL),
    m_pD2DFactory(NULL),
    m_pDrawColor(NULL),
    m_pColorRGBX(NULL)
{
    LARGE_INTEGER qpf = {0};
    if (QueryPerformanceFrequency(&qpf))
    {
        m_fFreq = double(qpf.QuadPart);
    }

    // create heap storage for color pixel data in RGBX format
    m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];
}
  

/// <summary>
/// Destructor
/// </summary>
CParkingDetect::~CParkingDetect()
{
    // clean up Direct2D renderer
    if (m_pDrawColor)
    {
        delete m_pDrawColor;
        m_pDrawColor = NULL;
    }

    if (m_pColorRGBX)
    {
        delete [] m_pColorRGBX;
        m_pColorRGBX = NULL;
    }

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    // done with frame readers
	SafeRelease(m_pColorFrameReader);
	SafeRelease(m_pDepthFrameReader);

    // close the Kinect Sensor
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    SafeRelease(m_pKinectSensor);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CParkingDetect::Run(HINSTANCE hInstance, int nCmdShow)
{
    MSG       msg = {0};
    WNDCLASS  wc;

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"DepthBasicsAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        NULL,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CParkingDetect::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        Update();

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if (hWndApp && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Main processing function
/// </summary>
void CParkingDetect::Update()
{
    if (!(m_pDepthFrameReader && m_pColorFrameReader))
    {
        return;
    }

 
	IColorFrame* pColorFrame = NULL;
	IDepthFrame* pDepthFrame = NULL;

	HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);

	if (SUCCEEDED(hr))
    {
        INT64 nTime = 0;

		IFrameDescription* pColorFrameDescription = NULL;
		int nColorWidth = 0;
        int nColorHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nColorBufferSize = 0;
		RGBQUAD *pColorBuffer = NULL;

		int nDepthWidth = 0;
		int nDepthHeight = 0;
		USHORT nDepthMinReliableDistance = 0;
		USHORT nDepthMaxDistance = 0;
		UINT nDepthBufferSize = 0;
		IFrameDescription* pDepthFrameDescription = NULL;
		UINT16 *pDepthBuffer = NULL;

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RelativeTime(&nTime);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
        }

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameDescription->get_Width(&nColorWidth);
        }

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameDescription->get_Height(&nColorHeight);
        }

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
			}
			else if (m_pColorRGBX)
			{
				pColorBuffer = m_pColorRGBX;
				nColorBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
			}
			else
			{
				hr = E_FAIL;
			}
        }

		if (SUCCEEDED(hr))
		{
			hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_RelativeTime(&nTime);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Width(&nDepthWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Height(&nDepthHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
		}

		if (SUCCEEDED(hr))
		{
			// In order to see the full range of depth (including the less reliable far field depth)
			// we are setting nDepthMaxDistance to the extreme potential depth threshold
			nDepthMaxDistance = USHRT_MAX;

			// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
			//// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);
		}
		if (SUCCEEDED(hr))
		{
			Process(nTime, pDepthBuffer, pColorBuffer, nDepthWidth, nColorWidth, nDepthHeight, nColorHeight, nDepthMinReliableDistance, nDepthMaxDistance);
        }

		SafeRelease(pColorFrameDescription);
		SafeRelease(pDepthFrameDescription);
    }

	SafeRelease(pColorFrame);
    SafeRelease(pDepthFrame);
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CParkingDetect::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CParkingDetect* pThis = NULL;
    
    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CParkingDetect*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CParkingDetect*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CParkingDetect::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pDrawColor = new ImageRenderer();
            HRESULT hr = m_pDrawColor->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD)); 
            if (FAILED(hr))
            {
                SetStatusMessage(L"Failed to initialize the Direct2D draw device.", 10000, true);
            }

            // Get and initialize the default Kinect sensor
            InitializeDefaultSensor();
        }
        break;

        // If the titlebar X is clicked, destroy app
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Quit the main message pump
            PostQuitMessage(0);
            break;

        // Handle button press
        case WM_COMMAND:
            // If it was for the screenshot control and a button clicked event, save a screenshot next frame 
            if (IDC_BUTTON_SCREENSHOT == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
            {
                m_bSave = 1;
				m_bHasSpace = false;
            }
            break;
    }

    return FALSE;
}

/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CParkingDetect::InitializeDefaultSensor()
{
	HRESULT hr = GetDefaultKinectSensor(&m_pKinectSensor);
    if (FAILED(hr))
    {
        return hr;
    }

    if (m_pKinectSensor)
    {
        // Initialize the Kinect and get the depth reader
        IDepthFrameSource* pDepthFrameSource = NULL;
		IColorFrameSource* pColorFrameSource = NULL;

        hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
        }
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}
		if (SUCCEEDED(hr))
        {            
			hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
        }

		SafeRelease(pColorFrameSource);
        SafeRelease(pDepthFrameSource);
    }

    if (!m_pKinectSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!", 10000, true);
        return E_FAIL;
    }

    return SUCCEEDED(hr);
}

/// <summary>
/// Handle new frame data
/// <param name="nTime">timestamp of frame</param>
/// <param name="pBuffer">pointer to frame data</param>
/// <param name="nWidth">width (in pixels) of input image data</param>
/// <param name="nHeight">height (in pixels) of input image data</param>
/// <param name="nMinDepth">minimum reliable depth</param>
/// <param name="nMaxDepth">maximum reliable depth</param>
/// </summary>
void CParkingDetect::Process(INT64 nTime, const UINT16* pDepthBuffer, RGBQUAD* pColorBuffer, int nDepthWidth, int nColorWidth, int nDepthHeight, int nColorHeight, USHORT nMinDepth, USHORT nMaxDepth)
{
    if (m_hWnd)
    {
        if (!m_nStartTime)
        {
            m_nStartTime = nTime;
        }

        double fps = 0.0;

        LARGE_INTEGER qpcNow = {0};
        if (m_fFreq)
        {
            if (QueryPerformanceCounter(&qpcNow))
            {
                if (m_nLastCounter)
                {
                    m_nFramesSinceUpdate++;
                    fps = m_fFreq * m_nFramesSinceUpdate / double(qpcNow.QuadPart - m_nLastCounter);
                }
            }
        }

        WCHAR szStatusMessage[64];
        StringCchPrintf(szStatusMessage, _countof(szStatusMessage), L" FPS = %0.2f    Time = %I64d", fps, (nTime - m_nStartTime));

        if (SetStatusMessage(szStatusMessage, 1000, false))
        {
            m_nLastCounter = qpcNow.QuadPart;
            m_nFramesSinceUpdate = 0;
        }
    }

    // Make sure we've received valid data
    if (pColorBuffer && (nColorWidth == cColorWidth) && (nColorHeight == cColorHeight)
		&& pDepthBuffer && (nDepthWidth == cDepthWidth) && (nDepthHeight == cDepthHeight))
    {
        // end pixel is start + width*height - 1
        const UINT16* pBufferEnd = pDepthBuffer + (nDepthWidth * nDepthHeight);
		const UINT16* pBufferBegin = pDepthBuffer;
		const INT8 NUM_REGIONS = 3;
		INT64 regionSum[NUM_REGIONS];
		INT64 regionCount[NUM_REGIONS];
		double regionAvg[NUM_REGIONS];
		for (INT8 i = 0; i < NUM_REGIONS; i++)
		{
			regionSum[i] = 0;
			regionCount[i] = 0;
			regionAvg[i] = 0.;
		}

		while (pDepthBuffer < pBufferEnd)
		{
			USHORT depth = *pDepthBuffer;
			INT8 i = ((NUM_REGIONS * (pDepthBuffer - pBufferBegin)) / (pBufferEnd - pBufferBegin));
			if (i >= 0 && i < NUM_REGIONS)
			{
				regionSum[i] += depth;
				++regionCount[i];
			}
			++pDepthBuffer;
        }
		for (INT8 j = 0; j < NUM_REGIONS; j++)
		{
			regionAvg[j] = (double)regionSum[j] / (double)regionCount[j];
		}
		bool hasSpace = ((regionAvg[0] + regionAvg[2]) / 2 - regionAvg[1]) / regionAvg[1] <= .666 || regionAvg[1] > 2000;
        
		// Draw the data with Direct2D
		m_pDrawColor->Draw(reinterpret_cast<BYTE*>(pColorBuffer), cColorWidth * cColorHeight * sizeof(RGBQUAD));

		if (m_bSave && m_nFramesSinceUpdate % 30 == 0)
		{
			if(hasSpace)
				m_bHasSpace = true;
			TCHAR* pszTxt = TEXT("Color");
			SaveScreenshot(pszTxt, pColorBuffer, nColorWidth, nColorHeight, m_bHasSpace, m_bSave);
			m_bSave = (m_bSave + 1) % 11;
		}
    }
}

void CParkingDetect::SaveScreenshot(TCHAR* pszTxt, RGBQUAD* pBuffer, int nWidth, int nHeight, bool m_bHasSpace, INT8 m_bSave)
{
	
	WCHAR szScreenshotPath[MAX_PATH];

	// Retrieve the path to My Photos
	TCHAR* pszExtension = TEXT("bmp");
	GetScreenshotFileName(szScreenshotPath, pszTxt, pszExtension, _countof(szScreenshotPath));

	// Write out the bitmap to disk
	HRESULT hr = SaveBitmapToFile(reinterpret_cast<BYTE*>(pBuffer), nWidth, nHeight, sizeof(RGBQUAD) * 8, szScreenshotPath);

	WCHAR szStatusMessage[64 + MAX_PATH];
	if (SUCCEEDED(hr))
	{
		// Set the status bar to show where the screenshot was saved
		StringCchPrintf(szStatusMessage, _countof(szStatusMessage), L"Screenshot saved to %s at screenshot %d with %s Spaces", szScreenshotPath, m_bSave, m_bHasSpace ? L"Parking" : L"No");
	}
	else
	{
		StringCchPrintf(szStatusMessage, _countof(szStatusMessage), L"Failed to write screenshot to %s", szScreenshotPath);
	}

	SetStatusMessage(szStatusMessage, 5000, true);

	// toggle off so we don't save a screenshot again next frame
	Py_Initialize();
	PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pValue;
	pName = PyString_FromString("label.py");
	pModule = PyImport_Import(pName);
	pDict = PyModule_GetDict(pModule);
	pFunc = PyDict_GetItemString(pDict, "m_bHasSpace");
	pArgs = PyTuple_New(1);
	pValue = PyString_FromString(szScreenshotPath);
	PyTuple_SetItem(pArgs, 0, pValue);
	PyTuple_SetItem(pArgs, 1, pValue);
	PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
	long result = PyInt_AsLong(pResult);
	Py_Finalize();
	if (result == 1) {
		//get gps location
		Py_Initialize();
		PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pValue;
		pName = PyString_FromString("get_gps.py");
		pModule = PyImport_Import(pName);
		pDict = PyModule_GetDict(pModule);
		pFunc = PyDict_GetItemString(pDict, "m_bHasSpace");
		pArgs = PyTuple_New(1);
		// pValue = PyString_FromString(szScreenshotPath);
		// PyTuple_SetItem(pArgs, 0, pValue);
		// PyTuple_SetItem(pArgs, 1, pValue);
		PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
		int len = PyTuple_Size(pResult);
		//long result = PyInt_AsLong(pResult);
		int *c_array = malloc(len * 2);
		while (len--) {
			c_array[len] = (int)PyInt_AsLong(PyTuple_GetItem(py_tuple, len));
			//c_array is our array of ints :)
		}
		Py_Finalize();
		//TODO: upload
	}
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
/// <param name="showTimeMsec">time in milliseconds to ignore future status messages</param>
/// <param name="bForce">force status update</param>
bool CParkingDetect::SetStatusMessage(_In_z_ WCHAR* szMessage, DWORD nShowTimeMsec, bool bForce)
{
    INT64 now = GetTickCount64();

    if (m_hWnd && (bForce || (m_nNextStatusTime <= now)))
    {
        SetDlgItemText(m_hWnd, IDC_STATUS, szMessage);
        m_nNextStatusTime = now + nShowTimeMsec;

        return true;
    }

    return false;
}

/// <summary>
/// Get the name of the file where screenshot will be stored.
/// </summary>
/// <param name="lpszFilePath">string buffer that will receive screenshot file name.</param>
/// <param name="nFilePathSize">number of characters in lpszFilePath string buffer.</param>
/// <returns>
/// S_OK on success, otherwise failure code.
/// </returns>
HRESULT CParkingDetect::GetScreenshotFileName(_Out_writes_z_(nFilePathSize) LPWSTR lpszFilePath, TCHAR* pszCustomName, TCHAR* pszExtension, UINT nFilePathSize)
{
	WCHAR* pszKnownPath = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &pszKnownPath);

	if (SUCCEEDED(hr))
	{
		// Get the time
		WCHAR szTimeString[MAX_PATH];
		GetTimeFormatEx(NULL, 0, NULL, L"hh'-'mm'-'ss", szTimeString, _countof(szTimeString));

		// File name will be KinectScreenshotColor-HH-MM-SS.bmp
		StringCchPrintfW(lpszFilePath, nFilePathSize, L"%s\\%s-%s.%s", pszKnownPath, pszCustomName, szTimeString, pszExtension);
	}

	if (pszKnownPath)
	{
		CoTaskMemFree(pszKnownPath);
	}

	return hr;
}

/// <summary>
/// Save passed in image data to disk as a bitmap
/// </summary>
/// <param name="pBitmapBits">image data to save</param>
/// <param name="lWidth">width (in pixels) of input image data</param>
/// <param name="lHeight">height (in pixels) of input image data</param>
/// <param name="wBitsPerPixel">bits per pixel of image data</param>
/// <param name="lpszFilePath">full file path to output bitmap to</param>
/// <returns>indicates success or failure</returns>
HRESULT CParkingDetect::SaveBitmapToFile(BYTE* pBitmapBits, LONG lWidth, LONG lHeight, WORD wBitsPerPixel, LPCWSTR lpszFilePath)
{
	DWORD dwByteCount = lWidth * lHeight * (wBitsPerPixel / 8);

	BITMAPINFOHEADER bmpInfoHeader = { 0 };

	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);  // Size of the header
	bmpInfoHeader.biBitCount = wBitsPerPixel;             // Bit count
	bmpInfoHeader.biCompression = BI_RGB;                    // Standard RGB, no compression
	bmpInfoHeader.biWidth = lWidth;                    // Width in pixels
	bmpInfoHeader.biHeight = -lHeight;                  // Height in pixels, negative indicates it's stored right-side-up
	bmpInfoHeader.biPlanes = 1;                         // Default
	bmpInfoHeader.biSizeImage = dwByteCount;               // Image size in bytes

	BITMAPFILEHEADER bfh = { 0 };

	bfh.bfType = 0x4D42;                                           // 'M''B', indicates bitmap
	bfh.bfOffBits = bmpInfoHeader.biSize + sizeof(BITMAPFILEHEADER);  // Offset to the start of pixel data
	bfh.bfSize = bfh.bfOffBits + bmpInfoHeader.biSizeImage;        // Size of image + headers

																   // Create the file on disk to write to
	HANDLE hFile = CreateFileW(lpszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Return if error opening file
	if (NULL == hFile)
	{
		return E_ACCESSDENIED;
	}

	DWORD dwBytesWritten = 0;

	// Write the bitmap file header
	if (!WriteFile(hFile, &bfh, sizeof(bfh), &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Write the bitmap info header
	if (!WriteFile(hFile, &bmpInfoHeader, sizeof(bmpInfoHeader), &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Write the RGB Data
	if (!WriteFile(hFile, pBitmapBits, bmpInfoHeader.biSizeImage, &dwBytesWritten, NULL))
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Close the file
	CloseHandle(hFile);
	return S_OK;
}
