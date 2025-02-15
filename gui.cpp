#include "gui.h"
#include "imuread.h"
#include <string.h>


wxString port_name;
static bool show_calibration_confirmed = false;


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
END_EVENT_TABLE()


MyFrame::MyFrame(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxFrame( parent, id, title, position, size, style )
{
	wxPanel *panel;
	wxMenuBar *menuBar;
	wxMenu *menu;
	wxSizer *topsizer;
	wxSizer *leftsizer, *middlesizer, *rightsizer;
	wxSizer *hsizer, *vsizer, *calsizer;
	wxStaticText *text;
	int i, j;

	topsizer = new wxBoxSizer(wxHORIZONTAL);
	panel = new wxPanel(this);

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

	leftsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Communication");
	middlesizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Magnetometer");
	rightsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Calibration");

	topsizer->Add(leftsizer, 1, wxALL | wxEXPAND | wxALIGN_TOP, 5);
	topsizer->Add(middlesizer, 1, wxALL | wxEXPAND, 5);
	topsizer->Add(rightsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);

	buildLeftPanel(leftsizer, panel);

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


void MyFrame::showMessage(const char *message)
{
	wxMessageDialog dialog(this,message,
        " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
}

void MyFrame::buildLeftPanel(wxSizer *parentPanel, wxPanel *panel)
{
	wxSizer  *vsizer;
	wxStaticText *text;
	
	
	const wxPoint rawDataGridLocation = wxPoint(30,480);
	const wxSize rawDataGridSize = wxSize(450,95);
	const wxPoint messagesLocation = wxPoint(100,450);
	const wxPoint bitmapLocation = wxPoint(10,300);
	const wxSize messagesSize = wxSize(450,100);
	int colWidth = 100;
	
	vsizer = new wxBoxSizer(wxVERTICAL);
	parentPanel->Add(vsizer, 0, wxALL| wxEXPAND , 8);
	_portLabel = new wxStaticText(panel, wxID_ANY, "Port");
	
	vsizer->Add(_portLabel, 0, wxTOP|wxBOTTOM, 4);
	m_port_list = new wxComboBox(panel, ID_PORTLIST, "",
		wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	m_port_list->Append("(none )");
	m_port_list->Append(SAMPLE_PORT_NAME); // never seen, only for initial size
	m_port_list->SetSelection(0);
	vsizer->Add(m_port_list, 1, wxALL |wxEXPAND, 0);

	vsizer->AddSpacer(8);
	text = new wxStaticText(panel, wxID_ANY, "Actions");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_button_clear = new wxButton(panel, ID_CLEAR_BUTTON, "Clear");
	m_button_clear->Enable(false);
	vsizer->Add(m_button_clear, 1, wxEXPAND, 0);
	m_button_sendcal = new wxButton(panel, ID_SENDCAL_BUTTON, "Send Cal");
	vsizer->Add(m_button_sendcal, 1, wxEXPAND, 0);
	m_button_sendcal->Enable(false);
	vsizer->AddSpacer(16);
	text = new wxStaticText(panel, wxID_ANY, "Status");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);


	_rawDataGrid = new wxGrid(panel, wxID_ANY, rawDataGridLocation, rawDataGridSize, wxWANTS_CHARS);
	_rawDataGrid->CreateGrid(3,3);
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

	_statusMessage = new wxStaticText(panel, wxID_ANY, "Messages", messagesLocation, messagesSize, 0,wxStaticTextNameStr);
	_statusMessage->Wrap(300);
	vsizer->Add(_statusMessage, 0, wxTOP|wxBOTTOM, 4);	

	wxImage::AddHandler(new wxPNGHandler);
	m_confirm_icon = new wxStaticBitmap(panel, wxID_ANY, MyBitmap("checkemptygray.png"), bitmapLocation);
	vsizer->Add(m_confirm_icon, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);

	unsigned char *serialBufferMessage = (unsigned char *)"Raw: 0.624527,2.325824,9.525827,0.000000,0.000000,0.000000,20.000000,-36.300001,-144.699997";
	UpdateGrid(serialBufferMessage, 92);


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

void MyFrame::UpdateGrid(unsigned char *serialBufferMessage, int bytesRead)
{
	char messageBuffer[256];	
	
	//snprintf(messageBuffer,256,"%d bytes read", bytesRead);
	//showMessage(messageBuffer);	

	if (bytesRead < 40)
	{
		return;
	}
	
	snprintf(messageBuffer, 256,"%s",serialBufferMessage);
	//showMessage(messageBuffer);	
	
	//return;
	
	if (strcmp(messageBuffer,"reset check") == 0)
	{
		printf("reset check received\n");
		return;
	}

	char *token = strtok(messageBuffer,":");
	if (token == NULL) return;
	if (strcmp(token,"Raw") == 0)
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
	else
	{
		char errorMessage[128];
		snprintf(errorMessage, 128, "unknown start token '%s'", token);
		_statusMessage->SetLabelText(errorMessage);
	}

}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	static int firstrun=1;
	float gaps, variance, wobble, fiterror;
	char buf[32];
	int i, j;
	unsigned char *serialBufferMessage;	
	char messageBuffer[256];
		
	if (port_is_open()) {
		int bytesRead = read_serial_data();
		
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
		}	
		
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
			m_port_list->Clear();
			m_port_list->Append("(none)");
			m_port_list->SetSelection(0);
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
	//printf("OnClear\n");
	raw_data_reset();
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	printf("OnSendCal\n");
	printf("Magnetic Calibration:   (%.1f%% fit error)\n", magcal.FitError);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[0], magcal.invW[0][0], magcal.invW[0][1], magcal.invW[0][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[1], magcal.invW[1][0], magcal.invW[1][1], magcal.invW[1][2]);
	printf("   %7.2f   %6.3f %6.3f %6.3f\n",
		magcal.V[2], magcal.invW[2][0], magcal.invW[2][1], magcal.invW[2][2]);

	m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));
  int bytesWritten = send_calibration();
	printf("no. bytes written: %d\n", bytesWritten);

}

void calibration_confirmed(void)
{
	show_calibration_confirmed = true;
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
        wxArrayString list = serial_port_list();
        int num = list.GetCount();
        for (int i=0; i < num; i++) {
                menu->AppendRadioItem(9001 + i, list[i]);
                if (isopen && port_name.IsSameAs(list[i])) {
                        menu->Check(9001 + i, true);
                }
        }
	menu->UpdateUI();
}

void MyFrame::OnShowPortList(wxCommandEvent& event)
{
	//printf("OnShowPortList\n");
	m_port_list->Clear();
	m_port_list->Append("(none)");
	wxArrayString list = serial_port_list();
	int num = list.GetCount();
	for (int i=0; i < num; i++) {
		m_port_list->Append(list[i]);
	}
	SetMinimumWidthFromContents(m_port_list, 50);
}


void MyFrame::OnPortMenu(wxCommandEvent &event)
{
        int id = event.GetId();
        wxString name = m_port_menu->FindItem(id)->GetItemLabelText();

	close_port();
        //printf("OnPortMenu, id = %d, name = %s\n", id, (const char *)name);
	port_name = name;
	m_port_list->Clear();
	m_port_list->Append(port_name);
	SetMinimumWidthFromContents(m_port_list, 50);
	m_port_list->SetSelection(0);
        if (id == 9000) return;
	raw_data_reset();
	int openPortResult = open_port((const char *)name);
	if (openPortResult == 0)
	{
		showOpenPortError((const char *)name);
	}
	else
	{
		showOpenPortOK((const char *)name);
	}
	m_button_clear->Enable(true);
}

void MyFrame::OnPortList(wxCommandEvent& event)
{
	int selected = m_port_list->GetSelection();
	if (selected == wxNOT_FOUND) return;
	wxString name = m_port_list->GetString(selected);
	//printf("OnPortList, %s\n", (const char *)name);
	close_port();
	port_name = name;
	if (name == "(none)") return;
	raw_data_reset();
	int openPortResult = open_port((const char *)name);
	if (openPortResult == 0)
	{
		showOpenPortError((const char *)name);
	}
	else
	{
		showOpenPortOK((const char *)name);
	}
	m_button_clear->Enable(true);
}

void MyFrame::showOpenPortError(const char *name)
{
	char buffer[64];
	snprintf(buffer, 64, "port %s failed to open", name);
	
	wxMessageDialog dialog(this,buffer,
        " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();
}

void MyFrame::showOpenPortOK(const char *name)
{
	/*char buffer[64];
	sprintf(buffer,"port %s opened OK", name);
	
	wxMessageDialog dialog(this,buffer,
        " MotionCal", wxOK|wxICON_INFORMATION|wxCENTER);
    dialog.ShowModal();*/
    
    _statusMessage->SetLabelText("port open");
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
