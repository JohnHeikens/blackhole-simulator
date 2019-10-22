#include "main.h"
#include <time.h>
#include <list>

GraphicsObject* graphics = new GraphicsObject();

// Global Windows/Drawing variables
HBITMAP hbmp = NULL;
HWND hwnd = NULL;
HDC hdcMem = NULL;
// The window's DC
HDC wndDC = NULL;
HBITMAP hbmOld = NULL;

POINT MousePos = POINT();

std::vector<atom*> atoms = std::vector<atom*>(startatomcount);
vec2 camera = vec2();
int Run()
{
	atoms[0] = new atom(vec2(), vec2());
	for (int i = 1; i < startatomcount; i++) 
	{
		atoms[i] = new atom(
			vec2::getrotatedvector(RANDFP * math::PI2) * RANDFP * maxdistance,
			vec2::getrotatedvector(RANDFP * math::PI2) * maxspeed
		);
	}
	while (DoEvents())//next frame
	{
		Sleep(30);
		ProcessInput();//process events from user
		// Do stuff with graphics->colors
		Draw();
		// Draw graphics->colors to window
		BitBlt(wndDC, 0, 0, graphics->width, graphics->height, hdcMem, 0, 0, SRCCOPY);
	}
	return 0;
}
void Draw() 
{
	std::vector< std::vector<gravitytile*>*> tiles = std::vector< std::vector<gravitytile*>*>();//is this what einstein meant?
	std::vector<gravitytile*> tiles1d = std::vector<gravitytile*>();
	typedef std::vector<gravitytile*> gravitytileyrow;
	color atomcolor = colorpalette::red;
	graphics->ClearColor(colorpalette::black);
	mat3x3 screentransform = mat3x3();
	screentransform = mat3x3::translate2d(graphics->GetSize() * 0.5 - camera);
	for (int i = 0; i< atoms.size();i++)
	{
		atom* currentatom = atoms[i];
		gravitytileyrow* currentrow = NULL;
		gravitytile* currenttile = NULL;
		const vec2 gravitytilepos = currentatom->pos / gravitytilesize;
		cint gravitytilex = (int)floor(gravitytilepos.x);
		cint gravitytiley = (int)floor(gravitytilepos.y);
		bool calculated = false;
		for (gravitytileyrow* row : tiles)
		{
			if ((*row)[0]->y == gravitytiley) //row exists
			{
				currentrow = row;
				for (gravitytile* tile : *row) 
				{
					if (tile->x == gravitytilex) 
					{
						//already calculated
						currenttile = tile;
						currenttile->weight++;//divide weight over gravitytiles
						calculated = true;
						break;
					}
				}
				break;
			}
		}
		if (!calculated) 
		{
			//gravity needs to be calculated
			//add a row
			if (currentrow == NULL)
			{
				currentrow = new gravitytileyrow();
				tiles.push_back(currentrow);
			}
			//add a tile
			currenttile = new gravitytile();
			currenttile->x = gravitytilex;
			currenttile->y = gravitytiley;
			currenttile->additionalspeed = vec2();
			if (i == 0) 
			{

				currenttile->weight = 10;
			}
			else {
				currenttile->weight = 1;
			}
			currentrow->push_back(currenttile);
			tiles1d.push_back(currenttile);
		}
		currentatom->currenttile = currenttile;
		
	}
	for (int i = 0; i< tiles1d.size();i++)
	{
		gravitytile* tile = tiles1d[i];
		//calculate additional speed for all atoms
		vec2 gravitytilepos = vec2(tile->x, tile->y) * gravitytilesize;
		for (int j = i + 1; j < tiles1d.size(); j++)
		{
			gravitytile* tile2 = tiles1d[j];
			const vec2 diff = vec2(tile2->x, tile2->y) - gravitytilepos;
			const fp distance2 = diff.lengthsquared();
			const fp distance = sqrt(distance2);
			const vec2 addition1to2 = ((diff / distance) * CalculateGravity1(distance2, GravityConstant));
			
			tile->additionalspeed = AddRelativisticVelocities(tile->additionalspeed, addition1to2,lightspeed);//+= addition1to2;
			tile2->additionalspeed = AddRelativisticVelocities(tile2->additionalspeed, -addition1to2, lightspeed);//-= addition1to2;
		}
		//draw the tile ( will always be behind atoms
		vec2 tilescreenpos = screentransform.multPointMatrix(vec2(tile->x * gravitytilesize, tile->y * gravitytilesize));
		graphics->FillRectangle(tilescreenpos.x, tilescreenpos.y, gravitytilesize, gravitytilesize, color((byte)(((tile->additionalspeed.x / lightspeed) * 128) + 128), (byte)(((tile->additionalspeed.y / lightspeed) * 128) + 128), (byte)0));

	}
	for (int i = 0; i < atoms.size(); i++)
	{
		atom* currentatom = atoms[i];

		const vec2 screenpos = screentransform.multPointMatrix(currentatom->pos);
		//graphics->FillPixel(screenpos, atomcolor);
		if (i == 0) {
			graphics->FillCircle(screenpos.x - 5, screenpos.y - 5, 10, 10, colorpalette::blue);
		}
		else 
		{
			//color((byte)(((tile->additionalspeed.x / lightspeed) * 128) + 128), (byte)(((tile->additionalspeed.y / lightspeed) * 128) + 128), (byte)0)
			graphics->FillCircle(screenpos.x - 5, screenpos.y - 5, 10, 10, 
				color((byte)(((currentatom->speed.x / lightspeed) * 128) + 128), (byte)(((currentatom->speed.y / lightspeed) * 128) + 128), (byte)128)
				//atomcolor
			);
		}
		currentatom->speed = AddRelativisticVelocities(currentatom->speed, currentatom->currenttile->additionalspeed, lightspeed);//+= currentatom->currenttile->additionalspeed;
		currentatom->pos += currentatom->speed;
	}
}

void ProcessInput()
{
	POINT p;
	if (GetCursorPos(&p))
	{
		//cursor position now in p.x and p.y
		if (ScreenToClient(hwnd, &p))
		{
			MousePos = p;
			//p.x and p.y are now relative to hwnd's client area
		}
	}
	if (pressed('A')) 
	{
		camera.x -= 10;
	}
	if (pressed('D'))
	{
		camera.x += 10;
	}
	if (pressed('W'))
	{
		camera.y -= 10;
	}
	if (pressed('S'))
	{
		camera.y += 10;
	}
	fp power = 0.2;
	if (pressed('F')) 
	{
		atoms[0]->speed = AddRelativisticVelocities(atoms[0]->speed, vec2(-power, 0), lightspeed);
	}
	if (pressed('H'))
	{
		atoms[0]->speed = AddRelativisticVelocities(atoms[0]->speed, vec2(power, 0), lightspeed);
	}
	if (pressed('T'))
	{
		atoms[0]->speed = AddRelativisticVelocities(atoms[0]->speed, vec2(0, -power), lightspeed);
	}
	if (pressed('G'))
	{
		atoms[0]->speed = AddRelativisticVelocities(atoms[0]->speed, vec2(0, power), lightspeed);
	}
}
void MakeSurface(HWND hwnd) {
	/* Use CreateDIBSection to make a HBITMAP which can be quickly
	 * blitted to a surface while giving 100% fast access to graphics->colors
	 * before blit.
	 */
	 // Desired bitmap properties
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);//sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = graphics->width;
	bmi.bmiHeader.biHeight = -graphics->height; // Order graphics->colors from top to bottom
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // last byte not used, 32 bit for alignment
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;// graphics->width* graphics->height * 4;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;
	HDC hdc = GetDC(hwnd);
	graphics->colors = nullptr;
	// Create DIB section to always give direct access to colors
	hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)& graphics->colors, NULL, 0);
	
	DeleteDC(hdc);
	// Give plenty of time for main thread to finish setting up
	Sleep(50);//time??? without sleep, it finishes
	ShowWindow(hwnd, SW_SHOW);
	// Retrieve the window's DC
	wndDC = GetDC(hwnd);
	// Create DC with shared colors to variable 'graphics->colors'
	hdcMem = CreateCompatibleDC(wndDC);//HDC must be wndDC!! :)
	hbmOld = (HBITMAP)SelectObject(hdcMem, hbmp);

	graphics->depthbuffer = (fp*)calloc(graphics->width * graphics->height, sizeof(fp));
	//set random seed
	srand(time(NULL));
}
LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
	{
		MakeSurface(hwnd);
	}
	break;
	case WM_MOUSEMOVE:
	{
		
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		// Draw graphics->colors to window when window needs repainting
		BitBlt(hdc, 0, 0, graphics->width, graphics->height, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
	}
	break;
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
	}
	break;
	case WM_DESTROY:
	{
		SelectObject(hdcMem, hbmOld);
		DeleteDC(wndDC);
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	//get global vars
	GetDesktopResolution(graphics->width, graphics->height);
	//graphics->width = 1280;
	//graphics->height = 640;
	
	WNDCLASSEX wc;
	//MSG msg;
	// Init wc
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = CreateSolidBrush(0);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"animation_class";
	wc.lpszMenuName = NULL;
	wc.style = 0;
	// Register wc
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Failed to register window class.", L"Error", MB_OK);
		return 0;
	}
	// Make window
	hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		L"animation_class",
		L"Animation",
		WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION,
		300, 200, graphics->width, graphics->height,
		NULL, NULL, hInstance, NULL);
	RECT rcClient, rcWindow;
	POINT ptDiff;
	// Get window and client sizes
	GetClientRect(hwnd, &rcClient);
	GetWindowRect(hwnd, &rcWindow);
	// Find offset between window size and client size
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	// Resize client
	MoveWindow(hwnd, rcWindow.left, rcWindow.top, graphics->width + ptDiff.x, graphics->height + ptDiff.y, false);
	UpdateWindow(hwnd);

	return Run();
}