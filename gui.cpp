#include "gui.h"
#include "imuread.h"
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
	//EVT_CHAR(MyCanvas::OnChar)
	//EVT_MOUSE_EVENTS(MyCanvas::OnMouseEvent)
wxEND_EVENT_TABLE()

MyCanvas::MyCanvas(wxWindow *parent, wxWindowID id, int* gl_attrib)
	: wxGLCanvas(parent, id, gl_attrib)
{
	//m_xrot = 0;
	//m_yrot = 0;
	//m_numverts = 0;
	// Explicitly create a new rendering context instance for this canvas.
	m_glRC = new wxGLContext(this);
}

MyCanvas::~MyCanvas()
{
	delete m_glRC;
}

void MyCanvas::OnSize(wxSizeEvent& event)
{
	//printf("OnSize\n");
	if (!IsShownOnScreen()) return;
	SetCurrent(*m_glRC);
	resize_callback(event.GetSize().x, event.GetSize().y);
}

void MyCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	//printf("OnPaint\n");
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
	EVT_BUTTON(ID_SENDCAL_BUTTON, MyFrame::OnSendCal)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
	EVT_MENU_RANGE(9000, 9999, MyFrame::OnPortMenu)
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
	logMessage("******************************************");
	logMessage("MotionCal.app - start instance");
	
	logMessage("******************************************");
	wxPanel *panel;
	

	wxSizer *topsizer;
	wxBoxSizer *leftsizer;
	wxSizer *middlesizer, *rightsizer;
	wxSizer *hsizer, *vsizer, *calsizer;
	wxStaticText *text;
	int i, j;

	BuildMenu();
	BuildBufferDisplayCallBack();		

	topsizer = new wxBoxSizer(wxHORIZONTAL);
	panel = new wxPanel(this);
	
	leftsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Communication");
	middlesizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Magnetometer");
	rightsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Calibration");

	topsizer->Add(leftsizer, 0,  wxALL | wxEXPAND | wxALIGN_TOP, 5);
	topsizer->Add(middlesizer, 1, wxALL | wxEXPAND, 5);
	topsizer->Add(rightsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);

	BuildLeftPanel(leftsizer, panel);

	vsizer = new wxBoxSizer(wxVERTICAL);
	middlesizer->Add(vsizer, 1, wxEXPAND | wxALL, 8);

	text = new wxStaticText(panel, wxID_ANY, "");
	text->SetLabelMarkup("<small><i>Ideal calibration is a perfectly centered sphere</i></small>");
	vsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	int gl_attrib[20] = { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
		WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1, WX_GL_DOUBLEBUFFER, 0};
	m_canvas = new MyCanvas(panel, wxID_ANY, gl_attrib);
	m_canvas->SetMinSize(wxSize(480,480));
	vsizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);

	hsizer = new wxGridSizer(4, 0, 15);
	middlesizer->Add(hsizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
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
	//text->Wrap(200);
	//calsizer->Add(text, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);
	calsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	panel->SetSizer(topsizer);
	topsizer->SetSizeHints(panel);
	Fit();
	Show(true);
	Raise();

	m_canvas->InitGL();
	raw_data_reset();
	//open_port(PORT);
	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(14, wxTIMER_CONTINUOUS);
}

void MyFrame::StaticUpdateGrid(unsigned char* buffer, int size) {
    if (instance) {
        instance->UpdateGrid(buffer, size);
    }
    else
    {
    	logMessage("no instance set");
    }
}

// Set a callback function for when there's grid data to display.
// Doing this enables us to decouple events. It also allows us to 
// reduce the chance of race conditions.
void MyFrame::BuildBufferDisplayCallBack()
{
	_drawingData = false;
	MyFrame::instance = this;//&frameInstance;
	setDisplayBufferCallback(MyFrame::StaticUpdateGrid);
}

void MyFrame::showMessage(const char *message)
{
	
	char log[512];
	snprintf(log, 512, "showMessage: '%s'\n", message);\
	logMessage(log);
	wxMessageDialog dialog(this,message, " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
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
	
	m_button_clear = new wxButton(parent, ID_CLEAR_BUTTON, "Clear");
	m_button_clear->Enable(false);
	actionsPanel->Add(m_button_clear, 1, wxEXPAND, 0);
		
	m_button_sendcal = new wxButton(parent, ID_SENDCAL_BUTTON, "Send Cal");
	m_button_sendcal->Enable(false);
	actionsPanel->Add(m_button_sendcal, 1, wxEXPAND, 0);
	
	return actionsPanel;
}

wxSizer* MyFrame::BuildDataPanel(wxPanel *parent)
{
	const wxPoint rawDataGridLocation = wxPoint(30,200);
	const wxPoint orientationGridLocation = wxPoint(30,300);
	
	wxSizer *dataPanel = new wxStaticBoxSizer(wxVERTICAL, parent, "Data");
	dataPanel->SetMinSize(wxSize(-1, 180)); 
	
	BuildRawDataGrid(parent, dataPanel, rawDataGridLocation);
	dataPanel->AddSpacer(8);
	BuildOrientationGrid(parent, dataPanel, orientationGridLocation);
	
	return dataPanel;
}

wxSizer* MyFrame::BuildStatusPanel(wxPanel *parent)
{	
	const wxPoint messagesLocation = wxPoint(30,375);	
	const wxPoint bitmapLocation = wxPoint(30,100);
	const wxSize messagesSize = wxSize(350,100);
	
	wxSizer *statusPanel = new wxStaticBoxSizer(wxHORIZONTAL, parent, "Status");
	_statusMessage = new wxStaticText(parent, wxID_ANY, "Messages", messagesLocation, messagesSize, 0,wxStaticTextNameStr);
	_statusMessage->Wrap(300);
	statusPanel->Add(_statusMessage, 0, wxTOP|wxBOTTOM, 4);	

	wxImage::AddHandler(new wxPNGHandler);
	m_confirm_icon = new wxStaticBitmap(parent, wxID_ANY, MyBitmap("checkemptygray.png"), bitmapLocation);
	statusPanel->Add(m_confirm_icon, 0, wxALL , 0);
	
	return statusPanel;
}

void MyFrame::BuildLeftPanel(wxBoxSizer *parentPanel, wxPanel *panel)
{	
	wxSizer *topmostPanel = BuildConnectionPanel(panel); 
	parentPanel->Add(topmostPanel,1,wxEXPAND | wxALL,5);	
	
	wxSizer* topPanel = BuildActionsPanel(panel);
	parentPanel->Add(topPanel,1,wxEXPAND | wxALL,5);	
	
	wxSizer* midPanel = BuildDataPanel(panel);	
	parentPanel->Add(midPanel,2,wxEXPAND | wxALL,5);
	
	wxSizer* lowerPanel = BuildStatusPanel(panel);
	parentPanel->Add(lowerPanel,2,wxEXPAND | wxALL,5);
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



void MyFrame::UpdateGrid(unsigned char *serialBufferMessage, int bytesRead)
{
	char messageBuffer[BUFFER_SIZE];		
	snprintf(messageBuffer,256,"UpdateGrid called: %d bytes read", bytesRead);
	DebugPrint("UpdateGrid", serialBufferMessage, bytesRead);
	logMessage(messageBuffer);	

	if (_drawingData)
	{
		logMessage("    already updating grid");
		return;
	}
		
	_drawingData = true;

	logMessage((char*)serialBufferMessage);	
	if (bytesRead < 40)
	{
		logMessage("    too few bytes read");
		_drawingData = false;
		logMessage("    exiting UpdateGrid");
		return;
	}
	
	if (strcmp(messageBuffer,"reset check") == 0)
	{
		logMessage("    doing reset check");
		_drawingData = false;
		return;
	}

	char *token = strtok((char*)serialBufferMessage,":");
	if (token == NULL)
	{
	  char errorMessage[640];
	  snprintf(errorMessage, 640, "    no colon in message '%s', %d", serialBufferMessage, bytesRead);
	  logMessage(errorMessage);
	}
    else
    {
    	char commandMessage[640];
	    snprintf(commandMessage, 640, "command: '%s'", token);
    	logMessage((char*)commandMessage);
    	
    	if (strcmp(token,"Raw") == 0)
		{
		    logMessage("read Raw");
		    _statusMessage->SetLabelText("read Raw2");
			//UpdateRawDataGrid((char*)serialBufferMessage);
		}
		else if (strcmp(token,"orientationOutput") == 0)
		{
			logMessage("read orientation");
			_statusMessage->SetLabelText(token);
			//UpdateOrientationGrid(token);
		}
		else
		{
			char errorMessage[640];
			snprintf(errorMessage, 640, "**Error '%s'", token);
			logMessage(errorMessage);		
			_statusMessage->SetLabelText(errorMessage);
		}
	}
	_drawingData = false;
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
	static int firstrun=1;
	float gaps, variance, wobble, fiterror;
	char buf[32];
	int i, j;
		
	if (port_is_open()) {
		//int bytesRead = read_serial_data();
		read_serial_data();
		/*
		
		
		if (bytesRead > 0)
		{
			serialBufferMessage	 = getSerialBuffer();	
			if (serialBufferMessage != NULL)
			{
				UpdateGrid(serialBufferMessage, bytesRead);	
				//_statusMessage->SetLabelText(serialBufferMessage);
			}
			else
			{
				_statusMessage->SetLabel("null buffer");
			}
		}	*/
		
		if (firstrun && m_canvas->IsShown()) {
			//int h, w;
			//m_canvas->GetSize(&w, &h);
			//printf("Canvas initial size = %d, %d\n", w, h);
			firstrun = 0;
		}

		
		m_canvas->Refresh();
		gaps = quality_surface_gap_error();
		variance = quality_magnitude_variance_error();
		wobble = quality_wobble_error();
		fiterror = quality_spherical_fit_error();
		
		//snprintf(messageBuffer, sizeof(messageBuffer),"gaps %.2f var. %.2f, wobble %.2f fitError %.2f",gaps, variance, wobble, fiterror);
		//_statusMessage->SetLabelText(messageBuffer);
		m_canvas->Refresh();
			
		if (gaps < 15.0f && variance < 4.5f && wobble < 4.0f && fiterror < 5.0f) {
			if (!m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || !m_button_sendcal->IsEnabled()) {
				m_sendcal_menu->Enable(ID_SENDCAL_MENU, true);
				m_button_sendcal->Enable(true);
				m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));
			}
		} else if (gaps > 20.0f && variance > 5.0f && wobble > 5.0f && fiterror > 6.0f) {
			if (m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || m_button_sendcal->IsEnabled()) {
				m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
				m_button_sendcal->Enable(false);
				m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
			}
		}
		snprintf(buf, sizeof(buf), "%.1f%%", quality_surface_gap_error());
		m_err_coverage->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_magnitude_variance_error());
		m_err_variance->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_wobble_error());
		m_err_wobble->SetLabelText(buf);
		snprintf(buf, sizeof(buf), "%.1f%%", quality_spherical_fit_error());
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
			snprintf(buf, sizeof(buf), "%.3f", 0.0f); // TODO...
			m_accel[i]->SetLabelText(buf);
		}
		for (i=0; i < 3; i++) {
			snprintf(buf, sizeof(buf), "%.3f", 0.0f); // TODO...
			m_gyro[i]->SetLabelText(buf);
		}
	} else {
		if (!port_name.IsEmpty()) {
			_statusMessage->SetLabelText("port has closed, updating stuff");
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_clear->Enable(false);
			m_button_sendcal->Enable(false);
			m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
			//m_port_list->Clear();
			//m_port_list->Append("(none)");
			//m_port_list->SetSelection(0);
			port_name = "";
		}
	}
	if (show_calibration_confirmed) {
		m_confirm_icon->SetBitmap(MyBitmap("checkgreen.png"));
		show_calibration_confirmed = false;
	}
}

void MyFrame::OnClear(wxCommandEvent &event)
{
	raw_data_reset();
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n", magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);
	
	
	m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));	
	int bytesWritten = send_calibration();
	printf("No. bytes written: %d\n", bytesWritten);
	
	char messageBuffer[255];
	snprintf(messageBuffer, 255,
		"Magnetic Calibration:   (%.1f%% fit error)\n   %7.2f   %6.3f %6.3f %6.3f\n   %7.2f   %6.3f %6.3f %6.3f\n   %7.2f   %6.3f %6.3f %6.3f\nNo. bytes written: %d\n",
		magcal.FitError,
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2],
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2],
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2],
		bytesWritten);
		
	showMessage(messageBuffer);	
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
    if (menu != m_port_menu) return;
        //printf("OnShow Port Menu, %s\n", (const char *)menu->GetTitle());
	
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


void MyFrame::PopulateBaudList()
{
	logMessage("Into PopulateBaudList");
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
	logMessage("Into ResetConnectionParameters");
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
		showOpenPortError((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding, openPortResult);
	}
	else
	{
		showOpenPortOK((const char *)port_name, (const char *)_baudRate, (const char *)_lineEnding);
	}
	m_button_clear->Enable(true);
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
	logMessage("Into OnLineEndingList");
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
		showOpenPortError((const char *)name, (const char *)_baudRate, (const char*) _lineEnding, openPortResult);
	}
	else
	{
		showOpenPortOK((const char *)name, (const char *)_baudRate, (const char*) _lineEnding);
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
		
	char portMessage[50];
	snprintf(portMessage, 50, "PortCount: %zu", uniqueList.GetCount());
	logMessage(portMessage);
	
	int num = uniqueList.GetCount();
	
	for (int i=0; i < num; i++) {
		
		m_port_list->Append(uniqueList[i]);
	}
	SetMinimumWidthFromContents(m_port_list, 50);
}


void MyFrame::showOpenPortError(const char *name, const char *baudRate, const char *lineEnding, int errorCode)
{
	const int messageBufferSize = 128;
	char errorMessage[messageBufferSize];
	if (errorCode == -4) snprintf(errorMessage, messageBufferSize, "couldn't get terminal settings(2)");
	if (errorCode == -3) snprintf(errorMessage, messageBufferSize, "tcsetattr failed");
	if (errorCode == -2) snprintf(errorMessage, messageBufferSize, "'open_port' failed");
	if (errorCode == -1) snprintf(errorMessage, messageBufferSize, "couldn't get terminal settings(1)");
	
	char buffer[messageBufferSize];
	snprintf(buffer, messageBufferSize, "port %s failed to open at %s bps: %s,line ending: %s", name, baudRate, lineEnding, errorMessage);
	logMessage(buffer);
	
	wxMessageDialog dialog(this,buffer, " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
}

void MyFrame::showOpenPortOK(const char *name, const char *baudRate, const char *lineEnding)
{
   	char commandMessage[640];
    snprintf(commandMessage, 640, "Port opened: '%s', %s bps, %s line ending", name, baudRate, lineEnding);
   	logMessage((char*)commandMessage);

    _statusMessage->SetLabelText(commandMessage);   
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
