#include <windows.h>		//Windows環境
#include <windowsx.h>		//Windows環境
#include <stdio.h>			//入出力用
#include <process.h>		//スレッド用
#include <stdlib.h>

#pragma comment(lib,"winmm.lib")//高精度タイマ

#include "resource.h"		//リソースファイル
#include "Header.h"		//リソースファイル

//構造体
typedef struct {
	HWND	hwnd;
	HWND	hEdit;
}SEND_POINTER_STRUCT;

//メイン関数(ダイアログバージョン)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HANDLE hMutex;

	//多重起動判定
	hMutex = CreateMutex(NULL, TRUE, DEF_MUTEX_NAME);		//ミューテックスオブジェクトの生成
	if (GetLastError() == ERROR_ALREADY_EXISTS) {				//2重起動の有無を確認
		MessageBox(NULL, TEXT("既に起動されています．"), NULL, MB_OK | MB_ICONERROR);
		return 0;											//終了
	}

	//ダイアログ起動
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainDlgProc);

	return FALSE;			//終了
}

//メインプロシージャ（ダイアログ）
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hPict1;		//ウィンドウハンドル（PictureBox）
	static HWND hPict2;
	static HFONT hFont;				//フォント
	static HANDLE hThread;
	static UINT thID;
	static SEND_POINTER_STRUCT Sps;
	static HWND hEdit;

	switch (uMsg) {
	case WM_INITDIALOG:		//ダイアログ初期化
		//最初は停止ボタン無効		//EnableWindowで入力を無効または有効にする。
		EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);

		 Sps.hwnd = hDlg;
		hEdit = GetDlgItem(hDlg, IDC_EDIT1);
		Sps.hEdit = hEdit;

		hPict1 = GetDlgItem(hDlg, IDC_STATIC1);
		WinInitialize(NULL, hDlg, (HMENU)110, "TEST1", hPict1, WndProc, &hwnd1); //初期化
		hPict2 = GetDlgItem(hDlg, IDC_STATIC2);
		WinInitialize(NULL, hDlg, (HMENU)110, "TEST2", hPict2, WndProc, &hwnd2); //初期化
		
		return TRUE;

	case WM_COMMAND:		//ボタンが押された時
		switch (LOWORD(wParam)) {
		case ID_START:			//開始ボタン

								//データ読み込みスレッド起動
			hThread = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps, 0, &thID);   //_beginthreadex→スレッドを立ち上げる関数	
			EnableWindow(GetDlgItem(hDlg, ID_START), FALSE);						//開始ボタン無効化
			EnableWindow(GetDlgItem(hDlg, ID_STOP), TRUE);							//停止ボタン有効化
			return TRUE;

		case ID_STOP:	//停止・再開ボタン

			/*　サスペンドカウンタ　**************************
			実行を許可するまでスレッドを動かさない。
			ResumeThread：　サスペンドカウンタを1減らす
			SuspendThread：　サスペンドカウンタを1増やす

			0のときは実行。それ以外は待機する。
			**************************************************/

			if (ResumeThread(hThread) == 0) {					//停止中かを調べる(サスペンドカウントを１減らす)
				SetDlgItemText(hDlg, ID_STOP, TEXT("再開"));	//再開に変更　　　　　　　　　　　　　　　　　　　//SetDlgItemTextでダイアログ内のテキストなどを変更することができる
				SuspendThread(hThread);						//スレッドの実行を停止(サスペンドカウントを１増やす)
			}
			else
				SetDlgItemText(hDlg, ID_STOP, TEXT("停止"));	//停止に変更

			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//ダイアログ終了
		return TRUE;
	}

	return FALSE;
}


HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	HDC			hdc;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps;					//(構造体)クライアント領域描画するための情報	
	HBRUSH		hBrush, hOldBrush;	//ブラシ
	HPEN		hPen, hOldPen;		//ペン

	switch (uMsg) {
	case WM_CREATE:
		colorPen = RGB(255, 255, 255);	//色指定
		break;

	case WM_PAINT:

		/********************************

		PictureControlに描画するためには，HDC型のハンドルを別に取得する
		必要があります．

		例：hdc = BeginPaint(hWnd, &ps);
		hdc:デバイスコンテキストのハンドル
		hWnd:PictureControlのハンドル
		ps：(構造体)クライアント領域描画するための情報

		********************************/

		hdc = BeginPaint(hWnd, &ps);//デバイスコンテキストのハンドル取得

		/********************************

		PictureControlに描画するためには，線を引きたいときはペン，
		塗りつぶす際にはブラシが必要です．

		********************************/
		hBrush = CreateSolidBrush(color);				//ブラシ生成
		hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);	//ブラシ設定
		hPen = CreatePen(PS_SOLID, 2, colorPen);		//ペン生成
		hOldPen = (HPEN)SelectObject(hdc, hPen);		//ペン設定
		RECT rect;
		//ここから背景描画
		GetClientRect(hWnd, &rect);						//座標取得

		/********************************

		図形を描画するためには以下の関数を用います．
		長方形：Rectangle(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);
		円：Ellipse(HDC hdc ,int nLeftRect , int nTopRect ,int nRightRect , int nBottomRect);

		nLiftRect：長方形の左上X座標
		nTopRect：左上Y座標
		nRightRect：右下X座標
		nBottomRect：右下のY座標

		線を引くには以下の関数を用います．

	　　線の始点設定：MoveToEx(HDC hdc , int X , int Y , NULL);
		X,Y：線の始点の座標
		線；LineTo(HDC hdc , int nXEnd , int nYEnd);
		nXEnd, nYEnd：線の終点の設定


		以上を参考に図形を描画する関数を以下に記述しましょう
		********************************/

		Rectangle(hdc, -1, -1, rect.right+2, rect.bottom+2);	//四角

		MoveToEx(hdc,40 ,rect.bottom/2, NULL);			//カレントポジション
		LineTo(hdc,rect.right - 50 , rect.bottom/2);	//X軸
		MoveToEx(hdc, 40, rect.top+10, NULL);			//カレントポジション
		LineTo(hdc, 40, rect.bottom-10);				//Y軸
		//横軸のタイトル
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, rect.right * 0.42, rect.bottom * 0.8, TEXT("Time(s)"), 7);
		
		//ここまで

		/********************************

		使い終わったペンとブラシは破棄する必要があります．

		********************************/

		SelectObject(hdc, hOldBrush);
		DeleteObject(hBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);

		//デバイスコンテキストのハンドル破棄
		EndPaint(hWnd, &ps);
		break;
	}

	return TRUE;
}



UINT WINAPI TFunc(LPVOID thParam)
{
	static SEND_POINTER_STRUCT *FU = (SEND_POINTER_STRUCT*)thParam;        //構造体のポインタ取得

	FILE *fp;			//ファイルポインタ
	BOOL Flag = TRUE;		//ループフラグ
	double data1;		//データ
	double data2;		//データ
	INT t = 0;
	INT x, y,y1,x1,x2, y2;
	HDC			hdc1,hdc2;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps;						//(構造体)クライアント領域描画するための情報	
	HBRUSH		hBrush, hOldBrush;	//ブラシ
	HPEN		hPen, hOldPen;		//ペン

	RECT rect;

		colorPen = RGB(255, 255, 255);
		hdc1 = GetDC(hwnd1);
		hdc2 = GetDC(hwnd2);

		hBrush = CreateSolidBrush(BLACK_BRUSH);				//ブラシ生成
		hOldBrush = (HBRUSH)SelectObject(hdc1, hBrush);	//ブラシ設定
		hOldBrush = (HBRUSH)SelectObject(hdc2, hBrush);	//ブラシ設定
		hPen = CreatePen(PS_SOLID, 2, colorR);		//ペン生成
		hOldPen = (HPEN)SelectObject(hdc1, hPen);		//ペン設定
		hOldPen = (HPEN)SelectObject(hdc2, hPen);		//ペン設定
		GetClientRect(hwnd1, &rect);						//座標取得
		GetClientRect(hwnd2, &rect);
		x = rect.left + 40;
		y = rect.bottom / 2;
		y2 = rect.bottom / 2;
		MoveToEx(hdc1, x, y, NULL);			//カレントポジション
		MoveToEx(hdc2, x, y, NULL);			//カレントポジション
		DWORD DNum = 0, beforeTime;
	int time = 0, min = 0;

	beforeTime = timeGetTime();						//現在の時刻計算（初期時間）

	if ((fp = fopen("data.txt", "r")) == NULL) {
		MessageBox(NULL, TEXT("Input File Open ERROR!"), NULL, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	//データ読み込み・表示
	while (Flag == TRUE) {
		DWORD nowTime, progress, idealTime;

		//時間の調整
		nowTime = timeGetTime();					//現在の時刻計算
		progress = nowTime - beforeTime;				//処理時間を計算
		idealTime = (DWORD)(DNum * (1000.0F / (double)DEF_DATAPERS));	//理想時間を計算
		if (idealTime > progress) Sleep(idealTime - progress);			//理想時間になるまで待機

		//データの読み込み
		if (fscanf(fp, "%lf   %lf", &data1,&data2) == EOF) {
			MessageBox(NULL, TEXT("終了"), TEXT("INFORMATION"), MB_OK | MB_ICONEXCLAMATION);
			EnableWindow(GetDlgItem(FU->hwnd, ID_START), TRUE);		//開始ボタン有効
			EnableWindow(GetDlgItem(FU->hwnd, ID_STOP), FALSE);		//停止ボタン無効
			Flag = FALSE;												//ループ終了フラグ
			return FALSE;
		}

		y1 = y - data1*y*0.8;
		y2 = y - data2*y*0.8;
		LineTo(hdc1, x, y1);//波形描画
		LineTo(hdc2, x, y2);//波形描画矩形	
		x++;

		DNum++;

		//一秒経過時
		if (progress >= 1000.0) {
			beforeTime = nowTime;
			DNum = 0;
		}
		if (x >= rect.right - 50) {
			hPen = CreatePen(PS_SOLID, 2, colorPen);		//ペン生成
			hOldPen = (HPEN)SelectObject(hdc1, hPen);		//ペン設定
			hOldPen = (HPEN)SelectObject(hdc2, hPen);		//ペン設定
			Rectangle(hdc1, -1, -1, rect.right + 2, rect.bottom + 2);	//四角
																		//colorPen = WHITE_PEN;							//白
			MoveToEx(hdc1, 40, rect.bottom / 2, NULL);			//カレントポジション
			LineTo(hdc1, rect.right - 50, rect.bottom / 2);	//X軸
			MoveToEx(hdc1, 40, rect.top + 10, NULL);			//カレントポジション
			LineTo(hdc1, 40, rect.bottom - 10);				//Y軸
			//横軸のタイトル
			SetTextColor(hdc1, RGB(255, 255, 255));
			SetBkColor(hdc1, RGB(0, 0, 0));
			TextOut(hdc1, rect.right * 0.42, rect.bottom * 0.8, TEXT("Time(s)"), 7);
			Rectangle(hdc2, -1, -1, rect.right + 2, rect.bottom + 2);	//四角
			MoveToEx(hdc2, 40, rect.bottom / 2, NULL);			//カレントポジション
			LineTo(hdc2, rect.right - 50, rect.bottom / 2);	//X軸
			MoveToEx(hdc2, 40, rect.top + 10, NULL);			//カレントポジション
			LineTo(hdc2, 40, rect.bottom - 10);				//Y軸
			//横軸のタイトル
			SetTextColor(hdc2, RGB(255, 255, 255));
			SetBkColor(hdc2, RGB(0, 0, 0));
			TextOut(hdc2, rect.right * 0.42, rect.bottom * 0.8, TEXT("Time(s)"), 7);
			x = rect.left + 40;
			MoveToEx(hdc1, x, y1, NULL);
			MoveToEx(hdc2, x, y2, NULL);

			SelectObject(hdc1, hOldPen);
			SelectObject(hdc2, hOldPen);
			DeleteObject(hPen);
			hPen = CreatePen(PS_SOLID, 2, colorR);		//ペン生成
			hOldPen = (HPEN)SelectObject(hdc1, hPen);		//ペン設定
			hOldPen = (HPEN)SelectObject(hdc2, hPen);		//ペン設定

		}

		
	}
		SelectObject(hdc1, hOldPen);
		SelectObject(hdc2, hOldPen);
		DeleteObject(hPen);
		
		EndPaint(hwnd1, &ps);
		EndPaint(hwnd2, &ps);
	return 0;
}

//-----------------------------------------------------------------------------
//子ウィンドウ初期化＆生成
//指定したウィンドウハンドルの領域に子ウィンドウを生成する．
//----------------------------------------------------------
// hInst	: 生成用インスタンスハンドル
// hPaWnd	: 親ウィンドウのハンドル
// chID		: 子ウィンドウのID
// cWinName	: 子ウィンドウ名
// PaintArea: 子ウィンドウを生成する領域のデバイスハンドル
// WndProc	: ウィンドウプロシージャ
// *hWnd	: 子ウィンドウのハンドル（ポインタ）
// 戻り値	: 成功時=true
//-----------------------------------------------------------------------------
BOOL WinInitialize(HINSTANCE hInst, HWND hPaWnd, HMENU chID, char *cWinName, HWND PaintArea, WNDPROC WndProc, HWND *hWnd)
{
	WNDCLASS wc;			//ウィンドウクラス
	WINDOWPLACEMENT	wplace;	//子ウィンドウ生成領域計算用（画面上のウィンドウの配置情報を格納する構造体）
	RECT WinRect;			//子ウィンドウ生成領域
	ATOM atom;				//アトム

							//ウィンドウクラス初期化
	wc.style = CS_HREDRAW ^ WS_MAXIMIZEBOX | CS_VREDRAW;	//ウインドウスタイル
	wc.lpfnWndProc = WndProc;									//ウインドウのメッセージを処理するコールバック関数へのポインタ
	wc.cbClsExtra = 0;											//
	wc.cbWndExtra = 0;
	wc.hCursor = NULL;										//プログラムのハンドル
	wc.hIcon = NULL;										//アイコンのハンドル
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);		//ウインドウ背景色
	wc.hInstance = hInst;										//ウインドウプロシージャがあるインスタンスハンドル
	wc.lpszMenuName = NULL;										//メニュー名
	wc.lpszClassName = (LPCTSTR)cWinName;									//ウインドウクラス名

	if (!(atom = RegisterClass(&wc))) {
		MessageBox(hPaWnd, TEXT("ウィンドウクラスの生成に失敗しました．"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	GetWindowPlacement(PaintArea, &wplace);	//描画領域ハンドルの情報を取得(ウィンドウの表示状態を取得)
	WinRect = wplace.rcNormalPosition;		//描画領域の設定

											//ウィンドウ生成
	*hWnd = CreateWindow(
		(LPCTSTR)atom,
		(LPCTSTR)cWinName,
		WS_CHILD | WS_VISIBLE,//| WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME |WS_VISIBLE, 
		WinRect.left, WinRect.top,
		WinRect.right - WinRect.left, WinRect.bottom - WinRect.top,
		hPaWnd, chID, hInst, NULL
		);

	if (*hWnd == NULL) {
		MessageBox(hPaWnd, TEXT("ウィンドウの生成に失敗しました．"), NULL, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}