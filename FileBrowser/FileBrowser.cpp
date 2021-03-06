//
// Create a tree and list control in the client area of the window.
//

#include "stdafx.h"
#include "FileBrowser.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// splitter position
RECT rect;
bool dragging;

// controls
HIMAGELIST hMgaIcon;
HWND hTree;
HWND hList;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void OnFillListControl(HWND hWnd, WPARAM wParam, LPARAM lParam);

void GetParentPathname(wchar_t* path, HWND hTree, HTREEITEM hItem);

void OnItemExpanding(LPARAM lParam);
void OnItemExpanded(LPARAM lParam);
void OnSelChanged(HWND hWnd, LPARAM lParam);

void OnLButtonDown(HWND hWnd, int x, int y);
void OnLButtonUp(HWND hWnd, int x, int y);
void OnMouseMove(HWND hWnd, int x, int y);

void OnPaint(HWND hWnd);
void OnCreate(HWND hWnd);
void OnDestroy(HWND hWnd);
void OnSize(HWND hWnd, int width, int height);

void OnFileExit(HWND hWnd);

// main window
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FILEBROWSER, szWindowClass, MAX_LOADSTRING);

    //  Registers the window class.
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FILEBROWSER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FILEBROWSER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);

    // Store instance handle in our global variable.
    hInst = hInstance; 

    // Create the main program window.
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    // Display the main program window.
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILEBROWSER));

    MSG msg;

    // Main message loop
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_FILL_LIST_CONTROL: OnFillListControl(hWnd, wParam, lParam);	break;
	case WM_NOTIFY:  
		switch(((LPNMHDR) lParam)->code)
		{
		case TVN_ITEMEXPANDING: OnItemExpanding(lParam); break;
		case TVN_ITEMEXPANDED:  OnItemExpanded(lParam);  break;
		case TVN_SELCHANGED:    OnSelChanged(hWnd, lParam);	break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_LBUTTONDOWN:	OnLButtonDown(hWnd, LOWORD(lParam), HIWORD(lParam));	break;
	case WM_LBUTTONUP:		OnLButtonUp(hWnd, LOWORD(lParam), HIWORD(lParam));		break;
	case WM_MOUSEMOVE:		OnMouseMove(hWnd, LOWORD(lParam), HIWORD(lParam));		break;
    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_EXIT:  OnFileExit(hWnd);							break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
        break;
	case WM_PAINT:   OnPaint(hWnd);									break;
	case WM_CREATE:  OnCreate(hWnd);								break;
	case WM_DESTROY: OnDestroy(hWnd);								break;
	case WM_SIZE:    OnSize(hWnd, LOWORD (lParam), HIWORD (lParam));break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// fill list control
void OnFillListControl(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    WIN32_FIND_DATA fData;
    HANDLE h;
	LV_ITEM lvI;
	int index;
	wchar_t path[MAX_PATH], name1[MAX_PATH], name2[14], name3[21];
	unsigned __int64 size;

	wchar_t* p = (wchar_t*)lParam;

	// add pathname to window title
	wchar_t szText[MAX_PATH];
    wcscpy_s(szText, MAX_PATH, szTitle);
    wcscat_s(szText, MAX_PATH, L" - ");
    wcscat_s(szText, MAX_PATH, p);
    SetWindowText(hWnd, szText);

	// empty list control
	ListView_DeleteAllItems(hList);

	// get list of files in this pathname
	wcscpy_s(path, MAX_PATH, p);
	wcscat_s(path, MAX_PATH, L"*.*");

	h = FindFirstFile(path, &fData);
	if (h == (HANDLE) 0xFFFFFFFF) return;  // can't find directory

	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvI.state = 0;
	lvI.stateMask = 0;
	lvI.iImage = 4;

	index = 0;

	// populate list control with filenames
	do
	{
		if (!wcscmp(fData.cFileName, L"..") || !wcscmp(fData.cFileName, L".") ) continue;
		if (!(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			wcscpy_s(name1, MAX_PATH, fData.cFileName);
			wcscpy_s(name2, 14, fData.cAlternateFileName);

			size = (unsigned __int64)fData.nFileSizeHigh * ((unsigned __int64)MAXDWORD+1) + (unsigned __int64)fData.nFileSizeLow;
			_ui64tow_s(size, name3, 21, 10);

			lvI.iItem = index;
			lvI.iSubItem = 0;
			lvI.pszText = name1;
			ListView_InsertItem(hList, &lvI);                 // column 1
			ListView_SetItemText(hList, index, 1, name2);     // column 2
			ListView_SetItemText(hList, index, 2, name3);     // column 3

			++index;
		}
	}while (FindNextFile(h, &fData));

	// close handle
	FindClose(h);

}

//  get the full pathname of the parent folder.
void GetParentPathname(wchar_t* path, HWND hTree, HTREEITEM hItem)
{
	TV_ITEM tvi;
	HTREEITEM hParent;
	wchar_t szText[MAX_PATH], szTemp[MAX_PATH], szPath[MAX_PATH];

	wcscpy_s(szPath, MAX_PATH, L"");

	hParent = hItem;

	// initialize variable fn to zero
	ZeroMemory(&tvi,sizeof(TV_ITEM));

	// concatenates folder name in reverse order
	// starting with immediate parent folder up to root folder.
	while(hParent != NULL)
	{
		tvi.mask           = TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem          = hParent;
		tvi.pszText        = szText;
		tvi.cchTextMax     = MAX_PATH;

		TreeView_GetItem(hTree,&tvi);

		wcscpy_s(szTemp, MAX_PATH, L"");
		wcscat_s(szTemp, MAX_PATH, szText);
		wcscat_s(szTemp, MAX_PATH, L"\\");

		wcscat_s(szTemp, MAX_PATH, szPath);
		wcscpy_s(szPath, MAX_PATH, szTemp);

		// get next parent
		hParent = TreeView_GetParent(hTree, hParent);
	}

	wcscpy_s(path, MAX_PATH, szPath);
}

//
void OnItemExpanding(LPARAM lParam)
{
	LPNM_TREEVIEW lpNMTreeView;
	HTREEITEM hItem;
	TVINSERTSTRUCT tvinsert;
	wchar_t path[MAX_PATH];

	lpNMTreeView = (LPNM_TREEVIEW)lParam;
	hItem        = lpNMTreeView->itemNew.hItem;

	// get full path name
	GetParentPathname(path, hTree, hItem);
	wcscat_s(path, MAX_PATH, L"*.*");

	// Just before the parent item gets EXPANDED,
	// add the children.
    WIN32_FIND_DATA fData;
    HANDLE h;

	if (lpNMTreeView->action == TVE_EXPAND)
	{               
		// Fill in the TV_ITEM struct
		tvinsert.hParent             = hItem;
		tvinsert.hInsertAfter        = TVI_LAST;
		tvinsert.item.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
		tvinsert.item.hItem          = NULL; 
		tvinsert.item.state          = 0;
		tvinsert.item.stateMask      = 0;
		tvinsert.item.iImage         = 2;
		tvinsert.item.iSelectedImage = 3;
		tvinsert.item.cChildren      = 1;

		// and call TreeView_InsertItem() for each item
		h = FindFirstFile(path, &fData);
		if (h == (HANDLE) 0xFFFFFFFF) return;  // can't find directory

		do
		{
			if (!wcscmp(fData.cFileName, L"..") || !wcscmp(fData.cFileName, L".") ) continue;
			if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				tvinsert.item.pszText = fData.cFileName;
				tvinsert.item.cchTextMax = wcslen(fData.cFileName);

				TreeView_InsertItem(hTree, &tvinsert);
			}
		}while (FindNextFile(h, &fData));

		// close handle
		FindClose(h);
	}
}

//
void OnItemExpanded(LPARAM lParam)
{
	LPNM_TREEVIEW lpNMTreeView;
	TV_ITEM tvi2;

	// Just before the parent item is COLLAPSED,
	// remove the children.       
	lpNMTreeView = (LPNM_TREEVIEW)lParam;
	tvi2 = lpNMTreeView->itemNew;

	// Do a TVE_COLLAPSERESET on the parent to minimize memory use.           
	if (lpNMTreeView->action == TVE_COLLAPSE)
	{
		TreeView_Expand (hTree, tvi2.hItem,TVE_COLLAPSE | TVE_COLLAPSERESET);
	}
}

// call when item is clicked in tree control
void OnSelChanged(HWND hWnd, LPARAM lParam)
{
	LPNM_TREEVIEW lpNMTreeView;
	HTREEITEM hItem;
	static wchar_t str[MAX_PATH];

	lpNMTreeView = (LPNM_TREEVIEW)lParam;
	hItem      = lpNMTreeView->itemNew.hItem;

	// get full pathname
	GetParentPathname(str, hTree, hItem);

	// send message to window to populate list control
	SendMessage(hWnd, WM_FILL_LIST_CONTROL, 0, (LPARAM)str);
}

// keep the double-arrow mouse cursor when dragging
void OnLButtonDown(HWND hWnd, int x, int y)
{
	if (rect.left < x && x < rect.right && rect.top < y && y < rect.bottom)
	{
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		SetCapture(hWnd);
		dragging = true;
	}
}

// restore the mouse cursor to previous one
void OnLButtonUp(HWND hWnd, int x, int y)
{
	ReleaseCapture();
	dragging = false;
}

// call when hovering or dragging splitter
void OnMouseMove(HWND hWnd, int x, int y)
{
	RECT rc;

	if (dragging)
	{
		// set the limit
		GetClientRect(hWnd,&rc);
		if(x < 96 || (rc.right - 96) < x) return;

		// change splitter position
		rect.left   = x;
		rect.right  = rect.left + 8;

		// resize tree control
		MoveWindow(hTree, 0, 0, rect.left, rect.bottom, TRUE);

		// reposition list control
		MoveWindow(hList, rect.left + 8, 0, rc.right - (rect.left + 8), rect.bottom, TRUE);
	}
	else
	{
		// change mouse cursor into double-pointed arrow when hover over the splitter
		if(rect.left < x && x < rect.right && rect.top < y && y < rect.bottom) SetCursor(LoadCursor(NULL, IDC_SIZEWE));
	}
}

//
void OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;

	hDC = BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
}

//
void OnCreate(HWND hWnd)
{
	// splitter initial position
	rect.left   = 300;
	rect.top    = 0;
	rect.right  = rect.left + 8;
	rect.bottom = 0;

	// not dragging
	dragging = false;

	// Create the image lists.
	hMgaIcon = ImageList_LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1), 16, 0, RGB(192,192,192));

	TVINSERTSTRUCT tvinsert;
	wchar_t drives[100], drive[5];
	wchar_t* p;
	int n;

	// Create tree control.
	hTree = CreateWindowEx(0L, WC_TREEVIEW, L"", WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                          0, 0, 100, 100, hWnd, (HMENU) IDC_TREE1, hInst, NULL);

	// Assigns an image list to a tree control.
	TreeView_SetImageList(hTree, hMgaIcon, TVSIL_NORMAL);

	// Add root folder name.
	tvinsert.hParent             = NULL;
	tvinsert.hInsertAfter        = TVI_ROOT;
	tvinsert.item.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
	tvinsert.item.hItem          = NULL; 
	tvinsert.item.state          = 0;
	tvinsert.item.stateMask      = 0;
	tvinsert.item.cchTextMax     = 5;
	tvinsert.item.iImage         = 0;
	tvinsert.item.iSelectedImage = 1;
	tvinsert.item.cChildren      = 1;
	tvinsert.item.lParam         = NULL;

	// get drives in the system
	if (GetLogicalDriveStrings(100, drives))
	{
		p = drives;
		while (*p) {

			// add to tree control
			wcscpy_s(drive, 5, p);
			n = wcslen(drive);
			drive[n - 1] = '\0';

			tvinsert.item.pszText = drive;
			TreeView_InsertItem(hTree, &tvinsert);

			// get the next drive
			p += wcslen(p) + 1;
		}
	}

	LV_COLUMN lvC;
	wchar_t str[100];

	// Create list control.
	hList = CreateWindowEx(0L, WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT,
                          0, 0, 100, 100, hWnd, (HMENU) IDC_LIST1, hInst, NULL);

	// Assigns an image list to a list control.
	ListView_SetImageList(hList,hMgaIcon,LVSIL_SMALL);

	// Create three columns.
	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;
	lvC.cx = 200;
	lvC.pszText = str;

	// column 1
	lvC.iSubItem = 0;
	wcscpy_s(str, 100, L"Name");
	if(ListView_InsertColumn(hList, 0, &lvC) == -1) return;

	// column 2
	lvC.iSubItem = 1;
	wcscpy_s(str, 100, L"DOS Name");
	if(ListView_InsertColumn(hList, 1, &lvC) == -1) return;

	// column 3
	lvC.iSubItem = 2;
	wcscpy_s(str, 100, L"Size");
	if(ListView_InsertColumn(hList, 2, &lvC) == -1) return;
}

//
void OnDestroy(HWND hWnd)
{
	// Destroys an image list.
	ImageList_Destroy(hMgaIcon);

	PostQuitMessage(0);
}

//
void OnSize(HWND hWnd, int width, int height)
{
	// resize splitter
	rect.bottom = height;

	// resize tree control
	MoveWindow(hTree, 0, 0, rect.left, height, TRUE);

	// resize list control
	MoveWindow(hList, rect.left + 8, 0, width - (rect.left + 8), height, TRUE);
}

//
void OnFileExit(HWND hWnd)
{
	DestroyWindow(hWnd);
}

