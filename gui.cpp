#include "gui.h"

#include <string.h>
#define BUFFER_SIZE 512
wxString port_name;
wxString _baudRate;
wxString _lineEnding;
static bool show_calibration_confirmed = false;
MyFrame* MyFrame::instance = nullptr;

wxBEGIN_EVENT_TABLE(MyCanvas, wxGLCanvas)
	EVT_SIZE(MyCanvas::OnSize)
	EVT_PAINT(MyCanvas::OnPaint)
wxEND_EVENT_TABLE()

MyCanvas::MyCanvas(wxWindow *parent, wxWindowID id, int* gl_attrib)
	: wxGLCanvas(parent, id, gl_attrib)
{
	// Explicitly create a new rendering context instance for this canvas.
	m_glRC = new wxGLContext(this);
}

MyCanvas::~MyCanvas()
{
	delete m_glRC;
}

void MyCanvas::OnSize(wxSizeEvent& event)
{
	if (!IsShownOnScreen()) return;
	SetCurrent(*m_glRC);
	resize_callback(event.GetSize().x, event.GetSize().y);
}

void MyCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	wxPaintDC dc(this);
	SetCurrent(*m_glRC);
	display_callback();
	SwapBuffers();
}

void MyCanvas::InitGL()
{
	SetCurrent(*m_glRC);
	visualize_init();
	wxSizeEvent e = wxSizeEvent(GetSize());
	OnSize(e);
}

/*****************************************************************************/

BEGIN_EVENT_TABLE(MyFrame,wxFrame)
	EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
	EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
	EVT_MENU(ID_SENDCAL_MENU, MyFrame::OnSendCal)
	EVT_BUTTON(ID_CLEAR_BUTTON, MyFrame::OnClear)
	EVT_BUTTON(ID_PAUSE_BUTTON, MyFrame::OnPause)	
	EVT_BUTTON(ID_SENDCAL_BUTTON, MyFrame::OnSendCal)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
	EVT_MENU_RANGE(9000, 9999, MyFrame::OnPortMenu)
	EVT_MENU_RANGE(ID_BAUDRATE_MENU, ID_BAUDRATE_MENU + 10, MyFrame::OnBaudRateMenu)
	EVT_MENU_OPEN(MyFrame::OnShowMenu)
	EVT_COMBOBOX(ID_PORTLIST, MyFrame::OnPortList)
	EVT_COMBOBOX_DROPDOWN(ID_PORTLIST, MyFrame::OnShowPortList)
	
	EVT_COMBOBOX(ID_BAUDLIST, MyFrame::OnBaudList)
	EVT_COMBOBOX_DROPDOWN(ID_BAUDLIST, MyFrame::OnShowBaudList)
	
	EVT_COMBOBOX(ID_LINEENDINGLIST, MyFrame::OnLineEndingList)
	EVT_COMBOBOX_DROPDOWN(ID_LINEENDINGLIST, MyFrame::OnShowLineEndingList)
END_EVENT_TABLE()


MyFrame::MyFrame(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxFrame( parent, id, title, position, size, style )
{
	// This can't go in the message panel, we haven't constructed the UI yet.
	logMessage("******************************************");
	logMessage("MotionCal.app - start instance");
	logMessage("******************************************");

	BuildMenu();
	BuildBufferDisplayCallBack();		

	wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);
	wxPanel *panel = new wxPanel(this);
	
	wxBoxSizer *leftsizer = BuildLeftPanel(panel);	
	wxBoxSizer *rightsizer = BuildRightPanel(panel);

	topsizer->Add(leftsizer, 0,  wxALL | wxEXPAND | wxALIGN_TOP, 5);
	topsizer->Add(rightsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);

	panel->SetSizer(topsizer);
	topsizer->SetSizeHints(panel);
	Fit();
	Show(true);
	Raise();

	m_canvas->InitGL();
	raw_data_reset();

	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(14, wxTIMER_CONTINUOUS);
	
	ShowInMessagesPanel("MotionCal started.", false);
}

wxBoxSizer* MyFrame::BuildLeftPanel(wxPanel *panel)
{
	wxBoxSizer *parent = new wxStaticBoxSizer(wxVERTICAL, panel, "Processing");
	wxStaticBoxSizer *topLeftSizer;
	wxBoxSizer *bottomLeftSizer;
	wxBoxSizer *commsSizer;
	wxSizer *middlesizer;
	
	topLeftSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "");
	bottomLeftSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Messsages");
			
	commsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
	middlesizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
	
	BuildMagnetomerPanel(panel, middlesizer);
	BuildTopLeftPanel(commsSizer, panel);
	BuildStatusPanel(panel, bottomLeftSizer);
						
	topLeftSizer->Add(commsSizer, 4,  wxLEFT | wxTOP | wxBOTTOM | wxEXPAND | wxALIGN_TOP, -10);
	topLeftSizer->AddSpacer(10);
	topLeftSizer->Add(middlesizer, 5,  wxRIGHT | wxTOP | wxBOTTOM |  wxEXPAND, -10);	
	
	wxColour bg = topLeftSizer->GetStaticBox()->GetBackgroundColour();
	topLeftSizer->GetStaticBox()->SetBackgroundColour(bg);
	topLeftSizer->GetStaticBox()->SetForegroundColour(bg);
	
	parent->Add(topLeftSizer, 3, wxALL | wxEXPAND, 0);	
	parent->Add(bottomLeftSizer, 1, wxALL | wxEXPAND, 0);
	
	return parent;
}

bool MyFrame::OffsetsCalibrationDataChanged(OffsetsCalibrationData newOffsetsCalibrationData)
{
	if (_lastOffsetsCalibrationData.calMag != newOffsetsCalibrationData.calMag)
		return true;
	for(int i = 0; i < 9; i++)
		if (_lastOffsetsCalibrationData.offsetData[i] != newOffsetsCalibrationData.offsetData[i])
			return true;
	
	return false;
}

bool MyFrame::SoftIronCalibrationDataChanged(SoftIronCalibrationData newSoftIronCalibrationData)
{
	for(int i = 0; i < 9; i++)
		if (_lastSoftIronCalibrationData.softIronData[i] != newSoftIronCalibrationData.softIronData[i])
			return true;
			
	return false;
}

void MyFrame::UpdateOffsetsCalibrationData(OffsetsCalibrationData newOffsetsCalibrationData)
{
	_lastOffsetsCalibrationData.calMag = newOffsetsCalibrationData.calMag;
	for(int i = 0; i < 9; i++)
		_lastOffsetsCalibrationData.offsetData[i] = newOffsetsCalibrationData.offsetData[i];
}

void MyFrame::UpdateSoftIronCalibrationData(SoftIronCalibrationData newSoftIronCalibrationData)
{
	for(int i = 0; i < 9; i++)
		_lastSoftIronCalibrationData.softIronData[i] = newSoftIronCalibrationData.softIronData[i];
}

wxBoxSizer* MyFrame::BuildRightPanel(wxPanel *panel)
{
	wxSizer *vsizer, *calsizer;
	wxStaticText *text;
	int i, j;
	
	wxStaticBoxSizer *rightsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Calibration");
	vsizer = new wxBoxSizer(wxVERTICAL);

	calsizer = new wxBoxSizer(wxVERTICAL);
	rightsizer->Add(calsizer, 0, wxALL, 8);
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Offset");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_mag_offset[i] = new wxStaticText(panel, wxID_ANY, "0.00");
		vsizer->Add(m_mag_offset[i], 1);
	}
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Mapping");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(3, 0, 12);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		for (j=0; j < 3; j++) {
			m_mag_mapping[i][j] = new wxStaticText(panel, wxID_ANY,
				((i == j) ? "+1.000" : "+0.000"));
			vsizer->Add(m_mag_mapping[i][j], 1);
		}
	}
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Field");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_mag_field = new wxStaticText(panel, wxID_ANY, "0.00");
	calsizer->Add(m_mag_field, 0, wxLEFT, 20);
	text = new wxStaticText(panel, wxID_ANY, "Accelerometer");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_accel[i] = new wxStaticText(panel, wxID_ANY, "0.000");
		vsizer->Add(m_accel[i], 1);
	}
	text = new wxStaticText(panel, wxID_ANY, "Gyroscope");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_gyro[i] = new wxStaticText(panel, wxID_ANY, "0.000");
		vsizer->Add(m_gyro[i], 1);
	}

	calsizer->AddSpacer(8);
	text = new wxStaticText(panel, wxID_ANY, "");
	text->SetLabelMarkup("<small>Calibration should be performed\n<b>after</b> final installation.  Presence\nof magnets and ferrous metals\ncan alter magnetic calibration.\nMechanical stress during\nassembly can alter accelerometer\nand gyroscope calibration.</small>");
	calsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);
	
	return rightsizer;
}

wxBoxSizer* MyFrame::BuildMagnetomerPanel(wxPanel *panel, wxSizer *parent)
{
	wxSizer *hsizer;
	wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
	parent->Add(vsizer, 1, wxEXPAND | wxALL, 8);

	wxStaticText *text = new wxStaticText(panel, wxID_ANY, "");
	text->SetLabelMarkup("<small><i>Ideal calibration is a perfectly centered sphere</i></small>");
	vsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	int gl_attrib[20] = { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1, WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1, WX_GL_DOUBLEBUFFER, 0};
	m_canvas = new MyCanvas(panel, wxID_ANY, gl_attrib);
	m_canvas->SetMinSize(wxSize(480,480));
	vsizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);
	
	hsizer = new wxGridSizer(4, 0, 10);
	
	parent->Add(hsizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	
	text = new wxStaticText(panel, wxID_ANY, "Gaps");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_coverage = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_coverage, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	
	text = new wxStaticText(panel, wxID_ANY, "Variance");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_variance = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_variance, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	
	text = new wxStaticText(panel, wxID_ANY, "Wobble");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_wobble = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_wobble, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	
	text = new wxStaticText(panel, wxID_ANY, "Fit Error");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_fit = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_fit, 1, wxALIGN_CENTER_HORIZONTAL);
	
	return vsizer;
}

wxSizer* MyFrame::BuildConnectionPanel(wxPanel *parent)
{
	wxSizer *connectionPanel = new wxStaticBoxSizer(wxVERTICAL, parent, "Connection");
	
	wxBoxSizer* portRow = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* portLabel = new wxStaticText(parent, wxID_ANY, "Port");
	portLabel->SetMinSize(wxSize(_labelWidth, -1));
	portRow->Add(portLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);	
	m_port_list = new wxComboBox(parent, ID_PORTLIST, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	m_port_list->Append("(none)");
	m_port_list->Append(SAMPLE_PORT_NAME); // never seen, only for initial size
	m_port_list->SetSelection(0);
	portRow->Add(m_port_list, 1, wxEXPAND);
	connectionPanel->Add(portRow,0,wxEXPAND | wxALL, 5);
	
	wxBoxSizer* baudRow = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* baudLabel = new wxStaticText(parent, wxID_ANY, "Baud Rate");
	baudLabel->SetMinSize(wxSize(_labelWidth, -1));
	baudRow->Add(baudLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);	
	_baudList = new wxComboBox(parent, ID_BAUDLIST, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	baudRow->Add(_baudList, 1, wxEXPAND);
	connectionPanel->Add(baudRow,0,wxEXPAND | wxALL, 5);
	PopulateBaudList();
	
	wxBoxSizer* lineEndingsRow = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* lineEndingsRowLabel = new wxStaticText(parent, wxID_ANY, "Line Endings");
	lineEndingsRowLabel->SetMinSize(wxSize(_labelWidth, -1));
	lineEndingsRow->Add(lineEndingsRowLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);	
	_lineEndingList = new wxComboBox(parent, ID_LINEENDINGLIST, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	PopulateLineEndingList();
	lineEndingsRow->Add(_lineEndingList, 1, wxEXPAND);
	connectionPanel->Add(lineEndingsRow,0,wxEXPAND | wxALL, 5);
	
	connectionPanel->SetMinSize(wxSize(-1, 60)); 	
	return connectionPanel;
}

wxSizer* MyFrame::BuildActionsPanel(wxPanel *parent)
{
	wxSizer *actionsPanel = new wxStaticBoxSizer(wxHORIZONTAL, parent, "Actions");	
	actionsPanel->SetMinSize(wxSize(-1, 60)); 

	// Create the row
	wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
	
	// Add the status icon
	wxImage::AddHandler(new wxPNGHandler);
	const wxPoint bitmapLocation = wxPoint(5,5);
	m_confirm_icon = new wxStaticBitmap(parent, wxID_ANY, MyBitmap("checkemptygray.png"), bitmapLocation);
	wxSizerItem* icon = row->Add(m_confirm_icon, 0, wxALL , 1);
	icon->SetMinSize(wxSize(240, -1)); 
	
	// Add the commands column
	wxBoxSizer* commandsColumn = new wxBoxSizer(wxVERTICAL);
	
	// Add the 'Pause' command
	m_button_pause = new wxButton(parent, ID_PAUSE_BUTTON, "Pause");
	m_button_pause->Enable(false);
	commandsColumn->Add(m_button_pause, 0, wxALL, 1);
	m_button_pause->SetMinSize(wxSize(120, -1)); 
	
	// Add the 'Clear' command
	m_button_clear = new wxButton(parent, ID_CLEAR_BUTTON, "Clear");
	m_button_clear->Enable(false);
	commandsColumn->Add(m_button_clear, 0, wxALL, 1);
	m_button_clear->SetMinSize(wxSize(120, -1)); 
	
	// Add the 'Send Calibration' command
	m_button_sendcal = new wxButton(parent, ID_SENDCAL_BUTTON, "Send Calibration");
	m_button_sendcal->Enable(false);
	commandsColumn->Add(m_button_sendcal, 0, wxALL, 1);
	m_button_sendcal->SetMinSize(wxSize(120, -1)); 
		
	_paused = false;
	
	// Add command column to row
	row->Add(commandsColumn,0, wxALL, 5);

	// Add row to panel
	actionsPanel->Add(row, 1, wxEXPAND, 0);
	
	return actionsPanel;
}

wxSizer* MyFrame::BuildDataPanel(wxPanel *parent)
{
	const wxPoint rawDataGridLocation = wxPoint(30,200);
	const wxPoint orientationGridLocation = wxPoint(30,300);
	
	wxSizer *dataPanel = new wxStaticBoxSizer(wxVERTICAL, parent, "Received Data");
	
	dataPanel->SetMinSize(wxSize(-1, 180)); 
	wxStaticText* rawLabel = new wxStaticText(parent, wxID_ANY, "Raw:");
	rawLabel->SetMinSize(wxSize(_labelWidth, -1));
	dataPanel->Add(rawLabel, 0, wxALIGN_LEFT | wxRIGHT, 5);	
	BuildRawDataGrid(parent, dataPanel, rawDataGridLocation);
	dataPanel->AddSpacer(20);
	
	wxStaticText* orientationLabel = new wxStaticText(parent, wxID_ANY, "Orientation:");
	orientationLabel->SetMinSize(wxSize(_labelWidth, -1));
	dataPanel->Add(orientationLabel, 0, wxALIGN_LEFT | wxRIGHT, 5);	
	BuildOrientationGrid(parent, dataPanel, orientationGridLocation);
	
	return dataPanel;
}

void MyFrame::BuildStatusPanel(wxPanel *parent, wxBoxSizer *bottomLeftSizer)
{	
	const wxPoint messagesLocation = wxPoint(0,0);	
	const wxSize messagesSize = wxSize(1000,100);
	
	_messageLog = new wxTextCtrl(parent, ID_MESSAGE_LOG, "",messagesLocation, messagesSize,
		wxTE_MULTILINE | wxTE_DONTWRAP| wxTE_MULTILINE  | wxTE_LEFT,
		wxDefaultValidator, "messageLog"); 
	
	wxTextAttr currentStyle;
	wxFont font(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
	_messageLog->SetFont(font);
	
	bottomLeftSizer->Add(_messageLog, 0, wxALL|wxEXPAND, 4);		
}

void MyFrame::LogImuData(ImuData imuData)
{
	// Leave as a placeholder
	//char buffer[120];
	//snprintf(buffer, 120, "ImuData: a(%f,%f,%f), g(%f,%f,%f), m(%f,%f,%f)", 
	//	imuData.accelerometer.x, imuData.accelerometer.y, imuData.accelerometer.z,
	//	imuData.gyroscope.x, imuData.gyroscope.y, imuData.gyroscope.z,
	//	imuData.magnetometer.x, imuData.magnetometer.y, imuData.magnetometer.z);

	//ShowInMessagesPanel(buffer, false);
}

void MyFrame::LogOrientationData(YawPitchRoll orientation)
{
	char buffer[120];
	snprintf(buffer, 120, "Ori: yaw: %f, pitch %f, roll %f", 
		orientation.yaw, orientation.pitch, orientation.roll);

	ShowInMessagesPanel(buffer, true);
}

void MyFrame::StaticUpdateImuData(ImuData imuData) {
    if (instance) 
        instance->UpdateImuData(imuData);
}

void MyFrame::StaticUpdateOrientationData(YawPitchRoll orientation) {
    if (instance)
        instance->UpdateOrientationData(orientation);
}

void MyFrame::StaticUnknownMessageReceived(const unsigned char* buffer, int size) {
    if (instance) 
        instance->UnknownMessageReceived(buffer, size);
}

void MyFrame::StaticSoftIronCalibrationDataReceived(SoftIronCalibrationData softIron) {
    if (instance) 
        instance->SoftIronCalibrationDataReceived(softIron);
}

void MyFrame::StaticOffsetCalibrationDataReceived(OffsetsCalibrationData offsets) {
    if (instance) 
        instance->OffsetCalibrationDataReceived(offsets);    
}

void MyFrame::UpdateImuData(ImuData imuData)
{
	char buffer[20];

	snprintf(buffer,20, "%f", imuData.accelerometer.x);
	_rawDataGrid->SetCellValue(X_ROW, ACCEL_COL,buffer);
	snprintf(buffer,20,"%f", imuData.accelerometer.y);	
	_rawDataGrid->SetCellValue(Y_ROW, ACCEL_COL,buffer);
	snprintf(buffer,20,"%f", imuData.accelerometer.z);	
	_rawDataGrid->SetCellValue(Z_ROW, ACCEL_COL,buffer);

	snprintf(buffer,20,"%f", imuData.gyroscope.x);
	_rawDataGrid->SetCellValue(X_ROW, GYRO_COL,buffer);
	snprintf(buffer,20,"%f", imuData.gyroscope.y);	
	_rawDataGrid->SetCellValue(Y_ROW, GYRO_COL,buffer);
	snprintf(buffer,20,"%f", imuData.gyroscope.z);	
	_rawDataGrid->SetCellValue(Z_ROW, GYRO_COL,buffer);
	
	snprintf(buffer,20,"%f", imuData.magnetometer.x);
	_rawDataGrid->SetCellValue(X_ROW, MAG_COL,buffer);
	snprintf(buffer,20,"%f", imuData.magnetometer.y);	
	_rawDataGrid->SetCellValue(Y_ROW, MAG_COL,buffer);
	snprintf(buffer,20,"%f", imuData.magnetometer.z);	
	_rawDataGrid->SetCellValue(Z_ROW, MAG_COL,buffer);
	
	ProcessImuDataFromCallback(imuData);
	LogImuData(imuData);
}

void MyFrame::UpdateOrientationData(YawPitchRoll orientation)
{
	char buffer[20];
	snprintf(buffer,20,"%f", orientation.yaw);	
	_orientationGrid->SetCellValue(READING_ROW, YAW_COL,buffer);
	snprintf(buffer,20,"%f", orientation.pitch);
	_orientationGrid->SetCellValue(READING_ROW, PITCH_COL,buffer);
	snprintf(buffer,20,"%f", orientation.roll);	
	_orientationGrid->SetCellValue(READING_ROW, ROLL_COL,buffer);
	LogOrientationData(orientation);
	SensorOrientation = orientation;
}

void MyFrame::UnknownMessageReceived(const unsigned char *serialBufferMessage, int bytesRead)
{
	char buffer[255];
	snprintf(buffer, 255,"Unknown Message: %s", serialBufferMessage);
	ShowInMessagesPanel(buffer, true);
}

void MyFrame::SoftIronCalibrationDataReceived(SoftIronCalibrationData softIron)
{
	if(SoftIronCalibrationDataChanged(softIron))
	{
		char buffer[255];
		snprintf(buffer, 255,"SoftIron: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", 
		softIron.softIronData[0], softIron.softIronData[1], softIron.softIronData[2],
		softIron.softIronData[3], softIron.softIronData[4], softIron.softIronData[5],
		softIron.softIronData[6], softIron.softIronData[7], softIron.softIronData[8],
		softIron.softIronData[9]);
		ShowInMessagesPanel(buffer, true);
		
		UpdateSoftIronCalibrationData(softIron);
	}
}
 
void MyFrame::OffsetCalibrationDataReceived(OffsetsCalibrationData offsets)
{
	if (OffsetsCalibrationDataChanged(offsets))
	{
		char buffer[255];
		
		snprintf(buffer, 255,"Offsets: %f, %f, %f, %f, %f, %f, %f, %f, %f, calMag: %f", 
		offsets.offsetData[0], offsets.offsetData[1], offsets.offsetData[2],
		offsets.offsetData[3], offsets.offsetData[4], offsets.offsetData[5],
		offsets.offsetData[6], offsets.offsetData[7], offsets.offsetData[8],
		offsets.calMag);
		ShowInMessagesPanel(buffer, true);
		
		UpdateOffsetsCalibrationData(offsets);
	}	
}

// Set a callback function for when there's grid data to display.
// Doing this enables us to decouple events. It also allows us to 
// reduce the chance of race conditions.
void MyFrame::BuildBufferDisplayCallBack()
{
	MyFrame::instance = this;
	setImuDataCallback(MyFrame::StaticUpdateImuData);
	setOrientationDataCallback(MyFrame::StaticUpdateOrientationData);
	setUnknownMessageCallback(MyFrame::StaticUnknownMessageReceived);
	setOffsetsCalibrationCallback(MyFrame::StaticOffsetCalibrationDataReceived);
	setSoftIronCalibrationCallback(MyFrame::StaticSoftIronCalibrationDataReceived);
}

void MyFrame::ShowMessagePopup(const char *message)
{	
	wxMessageDialog dialog(this,message, " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
}

void MyFrame::ShowInMessagesPanel(const char *message, bool echoToLogFile)
{
	_messageLog->AppendText(message);
	_messageLog->AppendText('\n');
	
	int lineCount = _messageLog->GetNumberOfLines();
	if (lineCount > 999)
	{
		long firstLineLength = _messageLog->GetLineLength(0);
		_messageLog->Remove(0, firstLineLength + 1);  
	}
		
	_messageLog->SetInsertionPointEnd();
	_messageLog->ShowPosition(_messageLog->GetLastPosition()); 
	if (strcmp(message,"Calibration verified in memory.\n") == 0 ||
		strcmp(message,"Calibration write failed or data is empty.\n") == 0)
	{
		ShowMessagePopup(message);
	}
}

void MyFrame::BuildTopLeftPanel(wxBoxSizer *parentPanel, wxPanel *panel)
{	
	wxSizer *topmostPanel = BuildConnectionPanel(panel); 
	parentPanel->Add(topmostPanel,1,wxEXPAND | wxALL,5);	
	
	wxSizer* topPanel = BuildActionsPanel(panel);
	parentPanel->Add(topPanel,1,wxEXPAND | wxALL,5);	
	
	wxSizer* midPanel = BuildDataPanel(panel);	
	parentPanel->Add(midPanel,2,wxEXPAND | wxALL,5);
	
}

void MyFrame::BuildRawDataGrid(wxPanel *panel, wxSizer *parent, wxPoint rawDataGridLocation)
{
	int colWidth = 100;
	const wxSize rawDataGridSize = wxSize(383,90);

	_rawDataGrid = new wxGrid(panel, wxID_ANY, rawDataGridLocation, rawDataGridSize, wxWANTS_CHARS);
	_rawDataGrid->CreateGrid(3,3);
	parent->Add(_rawDataGrid, 2, wxEXPAND, 0);
	
	wxFont gridLabelFont = _rawDataGrid->GetLabelFont();
	gridLabelFont.MakeBold();
	
	_rawDataGrid->SetLabelFont(gridLabelFont);
	_rawDataGrid->SetColLabelValue(ACCEL_COL,"Accel");
	_rawDataGrid->SetColLabelValue(MAG_COL,"Mag");
	_rawDataGrid->SetColLabelValue(GYRO_COL,"Gyro");
	
	_rawDataGrid->SetColSize(ACCEL_COL, colWidth);
	_rawDataGrid->SetColSize(MAG_COL, colWidth);
	_rawDataGrid->SetColSize(GYRO_COL, colWidth);
		
	_rawDataGrid->SetRowLabelValue(X_ROW,"X");
	_rawDataGrid->SetRowLabelValue(Y_ROW,"Y");
	_rawDataGrid->SetRowLabelValue(Z_ROW,"Z");
	
	_rawDataGrid->SetCellAlignment(X_ROW, ACCEL_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Y_ROW, ACCEL_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Z_ROW, ACCEL_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);

	_rawDataGrid->SetCellAlignment(X_ROW, MAG_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Y_ROW, MAG_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Z_ROW, MAG_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);

	_rawDataGrid->SetCellAlignment(X_ROW, GYRO_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Y_ROW, GYRO_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_rawDataGrid->SetCellAlignment(Z_ROW, GYRO_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
}

void MyFrame::BuildOrientationGrid(wxPanel *panel, wxSizer *parent, wxPoint orientationGridLocation)
{
	int colWidth = 100;
	const wxSize orientationGridSize = wxSize(383,53);

	_orientationGrid = new wxGrid(panel, wxID_ANY, orientationGridLocation, orientationGridSize, wxWANTS_CHARS);
	_orientationGrid->CreateGrid(1,3);
	parent->Add(_orientationGrid, 1, wxEXPAND , 0);
	wxFont gridLabelFont = _rawDataGrid->GetLabelFont();
	gridLabelFont.MakeBold();
		
	_orientationGrid->SetLabelFont(gridLabelFont);
	_orientationGrid->SetColLabelValue(YAW_COL,"Yaw");
	_orientationGrid->SetColLabelValue(PITCH_COL,"Pitch");
	_orientationGrid->SetColLabelValue(ROLL_COL,"Roll");	
	_orientationGrid->SetColSize(YAW_COL, colWidth);
	_orientationGrid->SetColSize(PITCH_COL, colWidth);
	_orientationGrid->SetColSize(ROLL_COL, colWidth);	
	_orientationGrid->SetRowLabelValue(READING_ROW,"Reading");
	_orientationGrid->SetCellAlignment(READING_ROW, YAW_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_orientationGrid->SetCellAlignment(READING_ROW, PITCH_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);
	_orientationGrid->SetCellAlignment(READING_ROW, ROLL_COL, wxALIGN_RIGHT, wxALIGN_CENTRE);	
}

void MyFrame::SetMinimumWidthFromContents(wxComboBox *control, unsigned int additional)
{
	unsigned int i;
	int maxWidth(0), width;
	for (i = 0; i < control->GetCount(); i++)
	{
		control->GetTextExtent(control->GetString(i), &width, NULL);
		if (width > maxWidth)
			maxWidth = width;
	}
	
	control->SetMinSize(wxSize(300, -1));
}

void MyFrame::BuildMenu()
{
	wxMenuBar *menuBar;
	wxMenu *menu;
	
	menuBar = new wxMenuBar;
	menu = new wxMenu;
	menu->Append(ID_SENDCAL_MENU, wxT("Send Calibration"));
	m_sendcal_menu = menu;
	m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
	menu->Append(wxID_EXIT, wxT("Quit"));
	menuBar->Append(menu, wxT("&File"));

	menu = new wxMenu;
	menuBar->Append(menu, "Port");
	m_port_menu = menu;
	
	menu = new wxMenu;
	menuBar->Append(menu, "Baud Rate");
	m_baudRateMenu = menu;
	
	menu = new wxMenu;
	menu->Append(wxID_ABOUT, wxT("About"));
	menuBar->Append(menu, wxT("&Help"));
	SetMenuBar(menuBar);
}

void MyFrame::DebugPrint(const char *name, const unsigned char *data, int len)
{
	int i;
    char message[60];
    
	snprintf(message, 60, "log data : '%s', %d", name, len);
	logMessage(message);

	for (i=0; i < len; i++) {
		snprintf(message, 60, "    %02X [%c]", data[i], data[i]);
	    logMessage(message);
	}
	snprintf(message, 60, "done %d", len);
	logMessage(message);
}

void MyFrame::UpdateRawDataGrid(char *token)
{
	token = strtok(NULL, ",");
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(X_ROW, ACCEL_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Y_ROW, ACCEL_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Z_ROW, ACCEL_COL,token);
		
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(X_ROW,GYRO_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Y_ROW,GYRO_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Z_ROW,GYRO_COL,token);
		
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(X_ROW,MAG_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Y_ROW,MAG_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_rawDataGrid->SetCellValue(Z_ROW,MAG_COL,token);
}

void MyFrame::UpdateOrientationGrid(char *token)
{
	token = strtok(NULL, ",");
	if (token == NULL) return;
	_orientationGrid->SetCellValue(READING_ROW, YAW_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_orientationGrid->SetCellValue(READING_ROW, PITCH_COL,token);
	token = strtok(NULL,",");			
	if (token == NULL) return;
	_orientationGrid->SetCellValue(READING_ROW, ROLL_COL,token);
}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	if (_paused)
		return;
		
	if (port_is_open()) {
		read_serial_data();
	} else {
		if (!port_name.IsEmpty()) {
			ShowInMessagesPanel("port has closed, updating stuff",false);
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_clear->Enable(false);
			m_button_sendcal->Enable(false);
			m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
			port_name = "";
		}
	}
	if (show_calibration_confirmed) {
		m_confirm_icon->SetBitmap(MyBitmap("checkgreen.png"));
		show_calibration_confirmed = false;
	}
}

void MyFrame::ProcessImuDataFromCallback(ImuData imuData)
{
	char messageBuffer[256];
	static int firstrun=1;
	float gaps, variance, wobble, fiterror;
	char buf[32];
	int i, j;
		
	if (firstrun && m_canvas->IsShown()) {
		firstrun = 0;
	}
	
	m_canvas->Refresh();
	gaps = quality_surface_gap_error();
	variance = quality_magnitude_variance_error();
	wobble = quality_wobble_error();
	fiterror = quality_spherical_fit_error();

	// don't spam the messages panel, only send a new gaps message if the data has changed.
	if (_lastGaps != gaps || _lastVariance != variance || _lastWobble != wobble || _lastFitError != fiterror)
	{
		snprintf(messageBuffer, sizeof(messageBuffer),"Gaps %.2f var. %.2f, wobble %.2f fitError %.2f",gaps, variance, wobble, fiterror);
		ShowInMessagesPanel(messageBuffer, true);
	}
	_lastGaps = gaps;
	_lastVariance = variance;
	_lastWobble = wobble;
	_lastFitError = fiterror;
	
	m_canvas->Refresh();
		
	if (gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f) {
		if (!m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || !m_button_sendcal->IsEnabled()) {
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, true);
			m_button_sendcal->Enable(true);
			m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));
			ShowInMessagesPanel("Can save calibration data.", false);
			CanSave = true;
		}
	} else if (gaps > 20.0f && variance > 5.0f && wobble > 5.0f && fiterror > 6.0f) {
		if (m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || m_button_sendcal->IsEnabled()) {
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_sendcal->Enable(false);
			m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
			CanSave = false;
		}
	}
	float qualitySurfaceGapError = quality_surface_gap_error();
	float qualityMagnitudeVarianceError = quality_magnitude_variance_error();
	float qualityWobbleError = quality_wobble_error();
	float qualitySphericalFitError = quality_spherical_fit_error();
	
	snprintf(buf, sizeof(buf), "%.1f%%", qualitySurfaceGapError);
	m_err_coverage->SetLabelText(buf);
	snprintf(buf, sizeof(buf), "%.1f%%", qualityMagnitudeVarianceError);
	m_err_variance->SetLabelText(buf);
	snprintf(buf, sizeof(buf), "%.1f%%", qualityWobbleError);
	m_err_wobble->SetLabelText(buf);
	snprintf(buf, sizeof(buf), "%.1f%%", qualitySphericalFitError);
	m_err_fit->SetLabelText(buf);
	

	for (i=0; i < 3; i++) {
		snprintf(buf, sizeof(buf), "%.2f", magcal.V[i]);
		m_mag_offset[i]->SetLabelText(buf);
	}
	for (i=0; i < 3; i++) {
		for (j=0; j < 3; j++) {
			snprintf(buf, sizeof(buf), "%+.3f", magcal.invW[i][j]);
			m_mag_mapping[i][j]->SetLabelText(buf);
		}
	}
	snprintf(buf, sizeof(buf), "%.2f", magcal.B);
	m_mag_field->SetLabelText(buf);
	for (i=0; i < 3; i++) {
		snprintf(buf, sizeof(buf), "%.3f", 0.0f);
		m_accel[i]->SetLabelText(buf);
	}
	for (i=0; i < 3; i++) {
		snprintf(buf, sizeof(buf), "%.3f", 0.0f);
		m_gyro[i]->SetLabelText(buf);
	}
}

void MyFrame::OnClear(wxCommandEvent &event)
{
	raw_data_reset();
}

void MyFrame::OnPause(wxCommandEvent &event)
{
	_paused = !_paused;
	if (_paused)
	{
		m_button_pause->SetLabel("Capture");
		ShowInMessagesPanel("Capturing paused", false);
	}
	else
	{
		m_button_pause->SetLabel("Pause");
		ShowInMessagesPanel("Capturing enabled", false);
	}
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	char messageBuffer[255];
	
	snprintf(messageBuffer, 255, "Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	ShowInMessagesPanel(messageBuffer, true);
		
	snprintf(messageBuffer, 255, "   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	ShowInMessagesPanel(messageBuffer, true);	
	snprintf(messageBuffer, 255, "   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	ShowInMessagesPanel(messageBuffer, true);	
	snprintf(messageBuffer, 255, "   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);
	ShowInMessagesPanel(messageBuffer, true);	
	
	m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));	
	int bytesWritten = send_calibration();
	printf("No. bytes written: %d\n", bytesWritten);
	
	snprintf(messageBuffer, 255,
		"Magnetic Calibration:   (%.1f%% fit error)\n   %7.2f   %6.3f %6.3f %6.3f\n   %7.2f   %6.3f %6.3f %6.3f\n   %7.2f   %6.3f %6.3f %6.3f\nNo. bytes written: %d\n",
		magcal.FitError,
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2],
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2],
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2],
		bytesWritten);
	ShowInMessagesPanel(messageBuffer, true);			
	ShowMessagePopup(messageBuffer);	
}

void calibration_confirmed(void)
{
	show_calibration_confirmed = true;
}

// Brute force de-duplication of ports list. This wouldn't
// scale much, but I doubt that being an issue as we're
// looking at a set of serial ports, so it's probably
// only going to have single digit number of items.
wxArrayString MyFrame::DeDuplicateList(wxArrayString originalList)
{
	wxArrayString uniqueList;

	int num = originalList.GetCount();
	uniqueList.Add(originalList[0]);
	for (int originaListIndex=1; originaListIndex < num; originaListIndex++) {
		bool isDuplicate = false;
		for(int newListIndex = 0; newListIndex < uniqueList.GetCount(); newListIndex++)
		{
			if (uniqueList[newListIndex] == originalList[originaListIndex])
			{ 
				isDuplicate = true;
				break;
			}
		}
		if (!isDuplicate)
		{
			uniqueList.Add(originalList[originaListIndex]);
		}		
	}	
	return uniqueList;
}

wxArrayString MyFrame::GetUniquePortList()
{
	wxArrayString list = serial_port_list();
	wxArrayString uniqueList = DeDuplicateList(list);
	return uniqueList;
}

void MyFrame::OnShowMenu(wxMenuEvent &event)
{
    wxMenu *menu = event.GetMenu();
    if (menu == m_port_menu){
		while (menu->GetMenuItemCount() > 0) {
			menu->Delete(menu->GetMenuItems()[0]);
		}
		
		menu->AppendRadioItem(9000, " (none)");
		bool isopen = port_is_open();
		if (!isopen) menu->Check(9000, true);
			wxArrayString list =  GetUniquePortList();  
			int num = list.GetCount();
			for (int i=0; i < num; i++) {
					menu->AppendRadioItem(9001 + i, list[i]);
					if (isopen && port_name.IsSameAs(list[i])) {
							menu->Check(9001 + i, true);
					}
			}
		menu->UpdateUI();
	}
	else if (menu == m_baudRateMenu)
	{
		while (menu->GetMenuItemCount() > 0) {
			menu->Delete(menu->GetMenuItems()[0]);
		}
		
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU, "300");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+1,"1200");		
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+2,"2400");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+3,"4800");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+4,"9600");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+5,"19200");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+6,"38400");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+7, "57600");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+8,"115200");	
		m_baudRateMenu->AppendRadioItem(ID_BAUDRATE_MENU+9,"230400");
		
		int selectedBaudRate = _baudList->GetSelection();		
		menu->Check(ID_BAUDRATE_MENU + selectedBaudRate, true);
		
		menu->UpdateUI();		
	}
}

void MyFrame::PopulateBaudList()
{
	if(_baudList == NULL)
		return;

	_baudList->Clear();
	_baudList->Append("300");	
	_baudList->Append("1200");		
	_baudList->Append("2400");	
	_baudList->Append("4800");	
	_baudList->Append("9600");	
	_baudList->Append("19200");	
	_baudList->Append("38400");	
	_baudList->Append("57600");	
	_baudList->Append("115200");	
	_baudList->Append("230400");	
	_baudList->SetSelection(5);
}

void MyFrame::OnShowBaudList(wxCommandEvent& event)
{
	PopulateBaudList();	
}

void MyFrame::ResetConnectionParameters()
{	
	int selectedBaudRate = _baudList->GetSelection();
	int selectedPort = m_port_list->GetSelection();
	int selectedLineEnding = _lineEndingList->GetSelection();
	
	if (selectedBaudRate == wxNOT_FOUND || selectedPort == wxNOT_FOUND || selectedLineEnding == wxNOT_FOUND) return;	

	_baudRate = _baudList->GetString(selectedBaudRate);
	port_name = m_port_list->GetString(selectedPort);
	_lineEnding = _lineEndingList->GetString(selectedLineEnding);
	
	if (strcmp(port_name,"(none)") == 0) return;

	close_port();
	raw_data_reset();
	
	int openPortResult = open_port((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding);
	if (openPortResult <= 0)
	{
		ShowOpenPortError((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding, openPortResult);
	}
	else
	{
		ShowOpenPortOK((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding);
	}
	SetPausable(openPortResult > 0);
	m_button_clear->Enable(true);
}

void MyFrame::SetPausable(bool pausable)
{
	m_button_pause->Enable(pausable);
}

void MyFrame::OnPortList(wxCommandEvent& event)
{
	ResetConnectionParameters();
}

void MyFrame::OnBaudList(wxCommandEvent& event)
{
	ResetConnectionParameters();
}

void MyFrame::OnLineEndingList(wxCommandEvent& event)
{	
	ResetConnectionParameters();
}

void MyFrame::OnPortMenu(wxCommandEvent &event)
{
    int id = event.GetId();
    wxString name = m_port_menu->FindItem(id)->GetItemLabelText();
	int selectedBaudRate = _baudList->GetSelection();
	if (selectedBaudRate == wxNOT_FOUND) return;
	_baudRate = _baudList->GetString(selectedBaudRate);
	int selectedLineEnding = _lineEndingList->GetSelection();
	_lineEnding = _lineEndingList->GetString(selectedLineEnding);
	close_port();
	port_name = name;
	m_port_list->Clear();
	m_port_list->Append(port_name);
	SetMinimumWidthFromContents(m_port_list, 50);
	m_port_list->SetSelection(0);
    if (id == 9000) return;
	raw_data_reset();
	int openPortResult = open_port((const char *)name, (const char *)_baudRate, (const char *)_lineEnding);
	if (openPortResult <= 0)
	{
		ShowOpenPortError((const char *)name, (const char *)_baudRate, (const char*) _lineEnding, openPortResult);
	}
	else
	{
		ShowOpenPortOK((const char *)name, (const char *)_baudRate, (const char*) _lineEnding);
	}
	SetPausable(openPortResult > 0);
	m_button_clear->Enable(true);
}

void MyFrame::OnBaudRateMenu(wxCommandEvent &event)
{
    int id = event.GetId();
    // get selected baud rate
    wxString baudRateName = m_baudRateMenu->FindItem(id)->GetItemLabelText();
    
    // set member variable
    _baudRate = baudRateName;
    
    // update baud rate drop down
    _baudList->SetStringSelection(_baudRate);
	
	int selectedLineEnding = _lineEndingList->GetSelection();
	_lineEnding = _lineEndingList->GetString(selectedLineEnding);
	
	// assume portname hasn't changed
	if (m_port_list->GetSelection() > 0)
	{
		close_port();
		raw_data_reset();
		int openPortResult = open_port((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding);
		if (openPortResult <= 0)
		{
			ShowOpenPortError((const char *)port_name, (const char *)_baudRate, (const char*) _lineEnding, openPortResult);
		}
		else
		{
			ShowOpenPortOK((const char *)port_name, (const char *)_baudRate, (const char*) _lineEnding);
		}
		
		SetPausable(openPortResult > 0);
	}
	m_button_clear->Enable(true);
}

void MyFrame::PopulateLineEndingList()
{
	_lineEndingList->Clear();
	_lineEndingList->Append("(none)");	
	_lineEndingList->Append("LF");		
	_lineEndingList->Append("CR");	
	_lineEndingList->Append("CRLF");	
	_lineEndingList->SetSelection(1);
}

void MyFrame::OnShowLineEndingList(wxCommandEvent& event)
{
	PopulateLineEndingList();
}

void MyFrame::OnShowPortList(wxCommandEvent& event)
{
	m_port_list->Clear();
	m_port_list->Append("(none)");
	wxArrayString uniqueList = GetUniquePortList();
			
	int num = uniqueList.GetCount();
	
	for (int i=0; i < num; i++) {
		
		m_port_list->Append(uniqueList[i]);
	}
	SetMinimumWidthFromContents(m_port_list, 50);
}

void MyFrame::ShowOpenPortError(const char *name, const char *baudRate, const char *lineEnding, int errorCode)
{
	const int messageBufferSize = 128;
	char errorMessage[messageBufferSize];
	if (errorCode == -4) snprintf(errorMessage, messageBufferSize, "couldn't get terminal settings(2)");
	if (errorCode == -3) snprintf(errorMessage, messageBufferSize, "tcsetattr failed");
	if (errorCode == -2) snprintf(errorMessage, messageBufferSize, "'open_port' failed");
	if (errorCode == -1) snprintf(errorMessage, messageBufferSize, "couldn't get terminal settings(1)");
	
	char buffer[messageBufferSize];
	snprintf(buffer, messageBufferSize, "port %s failed to open at %s bps: %s,line ending: %s", name, baudRate, lineEnding, errorMessage);
	ShowInMessagesPanel(buffer, true);
	
	wxMessageDialog dialog(this,buffer, " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
}

void MyFrame::ShowOpenPortOK(const char *name, const char *baudRate, const char *lineEnding)
{
   	char commandMessage[640];
    snprintf(commandMessage, 640, "Port opened: '%s', %s bps, %s line ending", name, baudRate, lineEnding);
    ShowInMessagesPanel(commandMessage, true);  
}

void MyFrame::OnAbout(wxCommandEvent &event)
{
        wxMessageDialog dialog(this,
                "MotionCal - Motion Sensor Calibration Tool\n\n"
		"Paul Stoffregen <paul@pjrc.com>\n"
		"http://www.pjrc.com/store/prop_shield.html\n"
		"https://github.com/PaulStoffregen/MotionCal\n\n"
		"Copyright 2018, PJRC.COM, LLC.",
                "About MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
        dialog.ShowModal();
}

void MyFrame::OnQuit( wxCommandEvent &event )
{
        Close(true);
}

MyFrame::~MyFrame(void)
{
	m_timer->Stop();
	close_port();
}

/*****************************************************************************/

IMPLEMENT_APP(MyApp)

MyApp::MyApp()
{
}

bool MyApp::OnInit()
{
	// make sure we exit properly on macosx
	SetExitOnFrameDelete(true);

	wxPoint pos(100, 100);

	MyFrame *frame = new MyFrame(NULL, -1, "Motion Sensor Calibration Tool",
		pos, wxSize(1120,760), wxDEFAULT_FRAME_STYLE);
#ifdef WINDOWS
	frame->SetIcon(wxIcon("MotionCal"));
#endif
	frame->Show( true );
	return true;
}

int MyApp::OnExit()
{
	return 0;
}