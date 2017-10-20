//By Quaternions

#include "stdafx.h"
#include <math.h>
#include <fstream>
#include <iostream>
#include <windows.h>

HHOOK Mouse;
POINT LastCursor;

struct boundary {
	//line info
	int diffx;//rx-lx
	int diffy;//ry-ly
	float diff2;
	float diffm;
	int lDot;
	int rDot;
	int dDot;
	//last state information
	int pDot;
	int cDot;
	bool mouseState = true;//false=behind true=infront
	bool mouseAlignment = false;//is the mouse between the left and right points
	//which boundary will the mouse teleport to when this boundary is crossed
	int teleportTarget;
};

//state variables
bool debounce;
bool pointParity;
bool boundaryParity;
bool creatingBoundaries;
int boundaryIndex;
//boundary list info
int boundaryListLength;
boundary * boundaryList;

/*
vec2 turn(vec2 v){
return vec2(y,-x);
}
*/
template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

LRESULT __stdcall MouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	bool cursorTeleported = false;
	if (nCode >= 0) {
		switch (wParam) {
		case WM_MOUSEMOVE:
			if (!creatingBoundaries) {
				POINT Cursor;
				GetCursorPos(&Cursor);
				int x = Cursor.x;//GET_X_LPARAM(lParam)
				int y = Cursor.y;//GET_Y_LPARAM(lParam)
				//std::cout << "x" << x << " y" << y << std::endl;
				for (int i = 0; i < boundaryListLength; ++i) {
					int pDot = x*boundaryList[i].diffy - y*boundaryList[i].diffx;
					int cDot = x*boundaryList[i].diffx + y*boundaryList[i].diffy;
					bool mouseState = pDot >= boundaryList[i].dDot;//dot(Cursor,turn(diff))>=dDot
					bool mouseAlignment = boundaryList[i].lDot <= cDot&&cDot <= boundaryList[i].rDot;//lDot<dot(Cursor,diff)<rDot
					bool condition = mouseState && !boundaryList[i].mouseState&&boundaryList[i].mouseAlignment;
					boundaryList[i].mouseState = mouseState;
					boundaryList[i].mouseAlignment = mouseAlignment;
					if (condition&&debounce) {//Mouse has just crossed the line
						//use saved data from last frame because windows can mess up the input values if it crossed a monitor border
						int j = boundaryList[i].teleportTarget;
						float tcDot = boundaryList[j].rDot + float(boundaryList[j].lDot - boundaryList[j].rDot)*float(boundaryList[i].cDot - boundaryList[i].lDot) / float(boundaryList[i].rDot - boundaryList[i].lDot);
						float tpDot = boundaryList[j].dDot - max(0.0f, boundaryList[j].diffm*float(boundaryList[i].pDot - boundaryList[i].dDot)) / boundaryList[i].diffm;
						int nx = int((tcDot*boundaryList[j].diffx + tpDot*boundaryList[j].diffy) / boundaryList[j].diff2) - sgn(boundaryList[j].diffy);
						int ny = int((tcDot*boundaryList[j].diffy - tpDot*boundaryList[j].diffx) / boundaryList[j].diff2) + sgn(boundaryList[j].diffx);
						ClipCursor(NULL);
						SetCursorPos(nx, ny);//This is the only place the cursor pos is set
						cursorTeleported = true;
						debounce = false;
						std::cout << "Teleported cursor from " << x << "," << y << " to " << nx << "," << ny << std::endl;
						break;
					}
					boundaryList[i].pDot = pDot;
					boundaryList[i].cDot = cDot;
				}
				LastCursor = Cursor;
				if (!(debounce || cursorTeleported)) {
					debounce = true;
				}
			}
			break;
		case WM_LBUTTONDOWN:
			if (creatingBoundaries && 0 <= boundaryIndex&&boundaryIndex<boundaryListLength) {
				POINT Cursor;
				GetCursorPos(&Cursor);
				std::cout << "Creating point at x=" << Cursor.x << " y=" << Cursor.y << std::endl;
				if (pointParity) {
					boundary * b = &boundaryList[boundaryIndex];
					//read left point from temp
					int lx = b->diffx;
					int ly = b->diffy;
					int rx = Cursor.x;//GET_X_LPARAM(lParam)
					int ry = Cursor.y;//GET_Y_LPARAM(lParam)
					//compute precalculated values
					b->diffx = rx - lx;
					b->diffy = ry - ly;
					b->diff2 = float(b->diffx*b->diffx + b->diffy*b->diffy);
					b->diffm = sqrt(b->diff2);
					b->lDot = b->diffx*lx + b->diffy*ly;
					b->rDot = b->diffx*rx + b->diffy*ry;
					b->dDot = (b->diffy*(lx + rx) - b->diffx*(ly + ry)) / 2;
					std::cout << "diffx" << b->diffx << std::endl;
					std::cout << "diffy" << b->diffy << std::endl;
					std::cout << "diffm" << b->diffm << std::endl;
					std::cout << "lDot" << b->lDot << std::endl;
					std::cout << "rDot" << b->rDot << std::endl;
					std::cout << "dDot" << b->dDot << std::endl;
					if (boundaryParity) {
						boundaryList[boundaryIndex - 1].teleportTarget = boundaryIndex;
						boundaryList[boundaryIndex].teleportTarget = boundaryIndex - 1;
					}
					boundaryIndex++;
					if (boundaryIndex == boundaryListLength) {
						std::cout << "Boundaries successfully created.  Saving to file cursorcorrector.boundaries.  Delete the file to make a new config." << std::endl;
						creatingBoundaries = false;
						std::ofstream file;
						file.open("cursorcorrector.boundaries", std::ios::out | std::ios::binary | std::ios::trunc);
						if (file.is_open()) {
							file.write((char*)&boundaryListLength, sizeof(int));
							file.write((char*)boundaryList, boundaryListLength * sizeof(boundary));
							file.close();
						}
						else {
							std::cout << "File failed to open!" << std::endl;
						}
					}
					else {
						std::cout << "Created boundary " << boundaryIndex << "/" << boundaryListLength << std::endl;
						boundaryParity = !boundaryParity;
					}
				}
				else {
					//write cursor pos to diff temporarily
					boundaryList[boundaryIndex].diffx = Cursor.x;//GET_X_LPARAM(lParam)
					boundaryList[boundaryIndex].diffy = Cursor.y;//GET_Y_LPARAM(lParam)
				}
				pointParity = !pointParity;
			}
			break;
		}
	}
	if (cursorTeleported) {
		return 1;
	}
	else {
		return CallNextHookEx(Mouse, nCode, wParam, lParam);
	}
}

int main()
{
	if (Mouse = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, NULL, 0))
	{
		std::cout << "Mouse Hooking Successful!" << std::endl;
	}
	else
	{
		std::cout << "Mouse Hooking failed!" << std::endl;
	}
	debounce = true;
	pointParity = false;
	boundaryParity = false;
	creatingBoundaries = true;
	boundaryIndex = -1;
	std::ifstream file;
	file.open("cursorcorrector.boundaries", std::ios::in | std::ios::binary);
	if (file.is_open()) {
		std::cout << "Loading config from file." << std::endl;
		//interpret int
		file.read((char*)&boundaryListLength, sizeof(int));
		//interpret everything else
		boundaryList = new boundary[boundaryListLength];
		file.read((char*)boundaryList, boundaryListLength * sizeof(boundary));
		file.close();
		boundaryIndex = boundaryListLength;
		creatingBoundaries = false;
	}
	else {
		std::cout << "How many screen boundaries do you have?  For a MxN grid of screens there are M*(N-1)+(M-1)*N or 2*M*N-M-N boundaries." << std::endl;
		std::cin >> boundaryListLength;
		if (boundaryListLength > 24) {
			std::cout << "Bruh you should just get the source code and write the boundaries in.  Do you seriously have over 16 monitors?  Who are you, Linus Sebastian?" << std::endl;
			return 0;
		}
		else if (boundaryListLength <= 0) {
			std::cout << "No boundaries huh... exactly why are you using this program?" << std::endl;
			creatingBoundaries = false;
		}
		else {
			boundaryIndex = 0;
			boundaryListLength *= 2;
			boundaryList = new boundary[boundaryListLength];
			std::cout << "Click the corners of the screens on each boundary in the order depicted in the guide image (guide can be rotated 90 degrees for the order of vertical boundaries)" << std::endl;
		}
	}
	std::cout << "Press Enter to exit the program." << std::endl;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{

	}
	return 0;
}

