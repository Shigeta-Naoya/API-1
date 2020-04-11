//定数宣言
#define DEF_APP_NAME	TEXT("Waveform Test")
#define DEF_MUTEX_NAME	DEF_APP_NAME			//ミューテックス名
#define DEF_DATAPERS 1000	//1秒間に何データ入出力するか

static COLORREF	color, colorPen;	//色
static COLORREF colorR = RGB(255, 0, 0); //波の色(赤)
static HWND hwnd1;		//子ウィンドウハンドル
static HWND hwnd2;

//関数宣言
BOOL CALLBACK MainDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );	//メインダイアログプロシージャ
BOOL WinInitialize( HINSTANCE hInst, HWND hPaWnd, HMENU chID, char *cWinName, HWND PaintArea, WNDPROC WndProc ,HWND *hDC);//子ウィンドウを生成
UINT WINAPI TFunc( LPVOID thParam );	
HRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );		//子ウィンドウプロシージャ