#ifndef gui__h_
#define gui__h_
#include "imuread.h"

#include <wx/wx.h>
#include "wx/timer.h"
#include "wx/glcanvas.h"
#include "wx/math.h"
#include "wx/log.h"
#include "wx/grid.h"
#include "wx/wfstream.h"
#include "wx/zstream.h"
#include "wx/txtstrm.h"
#if defined(__WXMAC__) || defined(__WXCOCOA__)
#ifdef __DARWIN__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <gl.h>
#include <glu.h>
#endif
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include "stdio.h"

#if defined(__WXMAC__) || defined(__WXCOCOA__)
	#ifdef __DARWIN__
		#include <OpenGL/gl.h>
		#include <OpenGL/glu.h>
	#else
		#include <gl.h>
		#include <glu.h>
	#endif
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif
#include "stdio.h"

#define ID_TIMER			10000
#define ID_SENDCAL_MENU		10001
#define ID_CLEAR_BUTTON		10002
#define ID_SENDCAL_BUTTON	10003
#define ID_PORTLIST			10004
#define ID_BAUDLIST			10005
#define ID_LINEENDINGLIST	10010
#define ID_MESSAGE_LOG		10020
#define ID_PAUSE_BUTTON		10030

#define X_ROW 0
#define Y_ROW 1
#define Z_ROW 2

#define ACCEL_COL 0
#define MAG_COL 2
#define GYRO_COL 1

#define YAW_COL 0
#define PITCH_COL 1
#define ROLL_COL 2
#define READING_ROW 0

class MyCanvas : public wxGLCanvas
{
public:
	MyCanvas(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		int *gl_attrib = NULL);

	virtual ~MyCanvas();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouseEvent(wxMouseEvent& event);

	void LoadSurface(const wxString& filename);
	void InitMaterials();
	void InitGL();

private:
	wxGLContext *m_glRC;
	wxDECLARE_NO_COPY_CLASS(MyCanvas);
	wxDECLARE_EVENT_TABLE();
};


class MyFrame: public wxFrame
{
public:
	MyFrame(wxWindow *parent, wxWindowID id,
		const wxString &title,
		const wxPoint &pos = wxDefaultPosition,
		const wxSize &size = wxDefaultSize,
		long style = wxDEFAULT_FRAME_STYLE);
	~MyFrame(void);
private:
	wxStaticText *m_err_coverage;
	wxStaticText *m_err_variance;
	wxStaticText *m_err_wobble;
	wxStaticText *m_err_fit;

	wxStaticText *m_mag_offset[3];
	wxStaticText *m_mag_mapping[3][3];
	wxStaticText *m_mag_field;
	wxStaticText *m_accel[3];
	wxStaticText *m_gyro[3];
	
	wxTextCtrl *_messageLog;

	MyCanvas *m_canvas;
	wxTimer *m_timer;
	wxButton *m_button_clear;
	wxButton *m_button_sendcal;
	wxButton *m_button_pause;
	wxStaticBitmap *m_confirm_icon;
	wxMenu *m_port_menu;
	wxComboBox *m_port_list;
	wxComboBox *_baudList;
	wxComboBox *_lineEndingList;
	
	wxMenu *m_sendcal_menu;
	
	wxGrid *_rawDataGrid;
	wxGrid *_orientationGrid;
	
	bool _drawingData;
	bool _paused = true;
		
	void OnSendCal(wxCommandEvent &event);
	void OnClear(wxCommandEvent &event);
	void OnPause(wxCommandEvent &event);
	void OnShowMenu(wxMenuEvent &event);
	void OnShowPortList(wxCommandEvent &event);
	void OnPortList(wxCommandEvent& event);
	void OnPortMenu(wxCommandEvent &event);
	void OnTimer(wxTimerEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnQuit(wxCommandEvent &event);
	void DebugPrint(const char *name, const unsigned char *data, int len);
	
	void OnShowBaudList(wxCommandEvent& event);
	void OnBaudList(wxCommandEvent& event);
	void PopulateBaudList();
	
	void OnShowLineEndingList(wxCommandEvent& event);
	void OnLineEndingList(wxCommandEvent& event);
	void PopulateLineEndingList();
	
	void SetMinimumWidthFromContents(wxComboBox *control, unsigned int additional);
	void showOpenPortError(const char *name, const char *baudRate, const char *lineEnding, int errorCode);
	void showOpenPortOK(const char *name, const char *baudRate, const char *lineEnding);
	void showMessage(const char *message);
	void showMessageInLog(const char *message);
	void LogImuData(ImuData imuData);
	void LogOrientationData(YawPitchRoll orientation);
		
		
	// Build UI components
	void BuildMenu();
	wxBoxSizer* BuildLeftPanel(wxPanel *panel);
	wxBoxSizer* BuildRightPanel(wxPanel *panel);
	void BuildTopLeftPanel(wxBoxSizer *parentPanel, wxPanel *panel);
	void BuildRawDataGrid(wxPanel *panel, wxSizer *parent, wxPoint rawDataGridLocation);
	void BuildOrientationGrid(wxPanel *panel, wxSizer *parent, wxPoint orientationGridLocation);
	void BuildBufferDisplayCallBack();
	void ResetConnectionParameters();
	
	wxSizer* BuildConnectionPanel(wxPanel *panel);
	wxSizer* BuildActionsPanel(wxPanel *parent);
	wxSizer* BuildDataPanel(wxPanel *parent);
	void BuildStatusPanel(wxPanel *parent, wxBoxSizer *panel);
	wxBoxSizer* BuildMagnetomerPanel(wxPanel *panel, wxSizer *parent);
	void SetPausable(bool pausable);
	

	// Callback handlers from serial data processor
	void UpdateImuData(ImuData imuData);
	static void StaticUpdateImuData(ImuData imuData);
	
	void UpdateOrientationData(YawPitchRoll orientation);	
	static void StaticUpdateOrientationData(YawPitchRoll orientation);
	
	void UpdateGrid(const unsigned char *serialBufferMessage, int bytesRead);
	static void StaticUpdateGrid(const unsigned char* buffer, int size);

	void UnknownMessageReceived(const unsigned char *serialBufferMessage, int bytesRead);
	static void StaticUnknownMessageReceived(const unsigned char* buffer, int size);

	void SoftIronCalibrationDataReceived(SoftIronCalibrationData softIron);
	static void StaticSoftIronCalibrationDataReceived(SoftIronCalibrationData softIron);

	void OffsetCalibrationDataReceived(OffsetsCalibrationData offsets);
	static void StaticOffsetCalibrationDataReceived(OffsetsCalibrationData offsets);

	void ProcessImuDataFromCallback(ImuData imuData);


	// Update UI	

    static MyFrame* instance; // Pointer to the current instance
	
	void UpdateRawDataGrid(char *token);
	void UpdateOrientationGrid(char *token);
	wxArrayString DeDuplicateList(wxArrayString originalList);
	wxArrayString GetUniquePortList();
	
	const int _labelWidth = 80;
	
	
	DECLARE_EVENT_TABLE()
};


class MyApp: public wxApp
{
public:
	MyApp();
	virtual bool OnInit();
	virtual int OnExit();
private:
        //wxSingleInstanceChecker *m_instance;
};

// portlist.cpp
wxArrayString serial_port_list();

// images.cpp
wxBitmap MyBitmap(const char *name);

// sample port name, for initial sizing of left panel
#if defined(LINUX)
#define SAMPLE_PORT_NAME "/dev/ttyACM5."
#elif defined(WINDOWS)
#define SAMPLE_PORT_NAME "COM22:."
#elif defined(MACOSX)
#define SAMPLE_PORT_NAME "/dev/cu.usbmodem2457891..."
#endif


#endif