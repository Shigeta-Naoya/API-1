#include <windows.h>		//Windows環境
#include <windowsx.h>		//Windows環境
#include <stdio.h>			//入出力用
#include <process.h>		//スレッド用
#include <stdlib.h>
#include<iostream>
#include<fstream>
#include<string>
using namespace std;

#pragma comment(lib,"winmm.lib")//高精度タイマ

//定数宣言
#define DEF_APP_NAME	TEXT("Waveform Test")
#define DEF_MUTEX_NAME	DEF_APP_NAME			//ミューテックス名

#include "resource.h"		//リソースファイル
#include "Header.h"		//リソースファイル
		

//構造体
typedef struct {
	HWND	hwnd;
	HWND	hEdit;
}SEND_POINTER_STRUCT;

HWND name[2];
streampos pos[2];

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
	static HFONT hFont;				//フォント
	static HANDLE hThread[2];
	static UINT thID[2];
	static SEND_POINTER_STRUCT Sps[2];
	static HWND hPict[2];		//ウィンドウハンドル（PictureBox）
	static HWND hWnd[2];		//子ウィンドウハンドル


	switch (uMsg) {
	case WM_INITDIALOG:		//ダイアログ初期化

		//最初は終了ボタン無効		//EnableWindowで入力を無効または有効にする。
		EnableWindow(GetDlgItem(hDlg, ID_STOP), FALSE);
		return TRUE;

	case WM_COMMAND:		//ボタンが押された時
		switch (LOWORD(wParam)) {
		case ID_START:			//開始ボタン
								//データ読み込みスレッド起動
			//終了ボタン有効化		//開始ボタン無効化　　　　
			hPict[0] = GetDlgItem(hDlg, IDC_PICTURE_BOX1);
			hPict[1] = GetDlgItem(hDlg, IDC_PICTURE_BOX2);

			WinInitialize(NULL, hDlg, (HMENU)110, "TEST", hPict[0], WndProc, &hWnd[0]); //初期化
			WinInitialize(NULL, hDlg, (HMENU)110, "TEST2", hPict[1], WndProc, &hWnd[1]); //初期化

			name[0] = hWnd[0];
			name[1] = hWnd[1];
			Sps[0].hwnd = hWnd[0];
			Sps[1].hwnd = hWnd[1];
			hThread[0] = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps[0], 0, &thID[0]);   //_beginthreadexでスレッドを立ち上げる	
			hThread[1] = (HANDLE)_beginthreadex(NULL, 0, TFunc, (PVOID)&Sps[1], 0, &thID[1]);

			//ここでデータを読み込んで描画をする用のスレッドを起動する
			//Timerを参考に…



			EnableWindow(GetDlgItem(hDlg, ID_START), FALSE);
			EnableWindow(GetDlgItem(hDlg, ID_STOP), TRUE);

			return TRUE;

		case ID_STOP:	//停止・再開ボタン

			/*　サスペンドカウンタ　**************************
			実行を許可するまでスレッドを動かさない。
			ResumeThread：　サスペンドカウンタを1減らす
			SuspendThread：　サスペンドカウンタを1増やす

			0のときは実行。それ以外は待機する。
			**************************************************/


			if (ResumeThread(hThread[0]) == 0 || ResumeThread(hThread[1]) == 0) {					//停止中かを調べる(サスペンドカウントを１減らす)
				SetDlgItemText(hDlg, ID_STOP, TEXT("再開"));	//再開に変更　　　　　　　　　　　　　　　　　　　//SetDlgItemTextでダイアログ内のテキストなどを変更することができる
				SuspendThread(hThread[0]);						//スレッドの実行を停止(サスペンドカウントを１増やす)
				SuspendThread(hThread[1]);
			}
			else
				SetDlgItemText(hDlg, ID_STOP, TEXT("終了"));	//終了に変更

			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);			//ダイアログ終了
		return TRUE;
	}

	//オーナー描画後に再描画
	if (uMsg == WM_PAINT) {
		InvalidateRect(hWnd[0], NULL, TRUE);	//再描画
		InvalidateRect(hWnd[1], NULL, TRUE);	//再描画
	}

	return FALSE;
}

HRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	HDC			hdc;				//デバイスコンテキストのハンドル
	PAINTSTRUCT ps;					//(構造体)クライアント領域描画するための情報
	RECT rect;
	HBRUSH		hBrush, hBrush2,hOldBrush;	//ブラシ
	HPEN		hPen, hOldPen,hPen2;		//ペン
	double data1, data2;
	static int width, height;


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

		//ペン，ブラシ生成
		hBrush = CreateSolidBrush(color);	
		hBrush2 = CreateSolidBrush(RGB(0, 0, 0));	//ブラシ生成
		hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);	//ブラシ設定
		hPen = CreatePen(PS_SOLID, 2, colorPen);
		hPen2 = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));		//ペン生成
		hOldPen = (HPEN)SelectObject(hdc, hPen);		//ペン設定

		//描画
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


		static int i = 0;
		if (i < 2)
		{
			GetClientRect(hWnd, &rect);//現在のクライアントのサイズを取得
			width = rect.right;//幅
			height = rect.bottom;//高さ

			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

			MoveToEx(hdc, 0, height / 2, NULL);//横軸
			LineTo(hdc, width, height / 2);

			MoveToEx(hdc, width / 8, 0, NULL);//縦軸
			LineTo(hdc, width / 8, height);
			//☆軸ラベルの背景を黒に設定する
			TextOut(hdc, width / 2, height * 6 / 8, TEXT("Time[s]"), 7);
			i++;
		}
		//☆軸ラベルの背景を黒に設定する
		TextOut(hdc, width / 2, height * 6 / 8, TEXT("Time[s]"), 7);
		//図形描画

		ifstream ifs("data.txt");
		string str;


		if (ifs.fail()) {
			cerr << "ファイルを読み込めませんでした\n";
			exit(0);
		}

		double data1, data2;
		static double old_position_y[2] = { height / 2,height / 2 };
		static double old_position_x[2] = { width / 8,width / 8 };


		//マルチスレッドでの処理なのでこれだとsin波と矩形波のタイミングがずれてしまうことがある
		//同一のスレッド内で同じタイミングでデータを読み込んでそれぞれ描画するように
		if (hWnd == name[0])
		{
			double tmp = 0;
			ifs.seekg(pos[0]);
			getline(ifs, str);

			//☆Timerを参考に1secに読み込むデータ数から待機時間を計算して待機するように
			sscanf(str.data(), "%lf %lf", &data1, &data2);
			pos[0] = ifs.tellg();


			tmp = -data1 * ((double)height / 2 - 2) + (double)(height - 1) / 2;


			colorPen = RGB(255, 255, 255);
			hOldPen = (HPEN)SelectObject(hdc, hPen);		//ペン設定

			if (old_position_x[0] + 1 > width)
			{
				hOldPen = (HPEN)SelectObject(hdc, hPen);		//ペン設定
				GetClientRect(hWnd, &rect);
				width = rect.right;
				height = rect.bottom;

				SetTextColor(hdc, RGB(255, 255, 255));
				colorPen = RGB(255, 255, 255);
				old_position_x[0] = width / 8;
				old_position_y[0] = tmp;

				Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

				MoveToEx(hdc, 0, height / 2, NULL);
				LineTo(hdc, width, height / 2);

				MoveToEx(hdc, width / 8, 0, NULL);
				LineTo(hdc, width / 8, height);

				//☆軸ラベルの背景を黒に設定するように
				TextOut(hdc, width / 2, height * 6 / 8, TEXT("Time[s]"), 7);
			}
			else
			{
				hOldPen = (HPEN)SelectObject(hdc, hPen2);		//ペン設定
				MoveToEx(hdc, old_position_x[0], old_position_y[0], NULL);
				LineTo(hdc, old_position_x[0] + 1, tmp);
				old_position_x[0] += 1;
				old_position_y[0] = tmp;
			}


		}
		else if (hWnd == name[1])
		{
			double tmp = 0;
			ifs.seekg(pos[1]);
			getline(ifs, str);


			sscanf(str.data(), "%lf %lf", &data1, &data2);
			pos[1] = ifs.tellg();

			SetTextColor(hdc, RGB(0, 0, 0));

			tmp = -data2 * ((double)height / 2 - 2) + (double)(height - 1) / 2;

			if (old_position_x[1] + 1 > width)
			{

				GetClientRect(hWnd, &rect);
				width = rect.right;
				height = rect.bottom;

				SetTextColor(hdc, RGB(255, 255, 255));
				colorPen = RGB(255, 255, 255);

				old_position_x[1] = width / 8;
				old_position_y[1] = tmp;

				Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

				MoveToEx(hdc, 0, height / 2, NULL);
				LineTo(hdc, width, height / 2);

				MoveToEx(hdc, width / 8, 0, NULL);
				LineTo(hdc, width / 8, height);
				//☆軸ラベルの背景を黒に設定するように
				TextOut(hdc, width / 2, height * 6 / 8, TEXT("Time[s]"), 7);
			}
			else
			{
				hOldPen = (HPEN)SelectObject(hdc, hPen2);		//ペン設定
				MoveToEx(hdc, old_position_x[1], old_position_y[1], NULL);
				LineTo(hdc, old_position_x[1] + 1, tmp);
				old_position_x[1] += 1;
				old_position_y[1] = tmp;
			}
		}


		//ペン，ブラシ廃棄
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

UINT WINAPI TFunc(LPVOID thParam) {

	//ここにはファイルデータの書き込みを行う
	SEND_POINTER_STRUCT* FU = (SEND_POINTER_STRUCT*)thParam;        //構造体のポインタ取得

	for (int i = 0; i < 3000; i++)
	{
		InvalidateRect(FU->hwnd, NULL, TRUE);	//再描画
		//Sleep(1000 / 61.5);
		Sleep(1000 / 1000);
	}



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