// ПРОГ 4.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "ПРОГ 4.h"

#include <Windows.h>
#include <ctime>
#include "Objects.h"

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY4, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY4));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY4));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY4);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) return FALSE;

   HMENU hMenu = CreateMenu();
   HMENU hSelectMenu = CreatePopupMenu();
   HMENU hFileMenu = CreatePopupMenu();
   HMENU hHelpMenu = CreatePopupMenu();

   AppendMenu(hSelectMenu, MF_STRING, IDM_BUS, L"Обычный автобус");
   AppendMenu(hSelectMenu, MF_STRING, IDM_MINIBUS, L"Микроавтобус");
   AppendMenu(hSelectMenu, MF_STRING, IDM_DOUBLEDECKER, L"Двухэтажный автобус");
   AppendMenu(hSelectMenu, MF_STRING, IDM_ADVERTISINGBUS, L"Рекламный автобус");
   AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSelectMenu, L"Выбор");

   AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"Выход");
   AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");

   AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, L"О программе");
   AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Справка");

   SetMenu(hWnd, hMenu);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    static PAINTSTRUCT ps;
    static RECT rect;

    static bool objectsInitialized = false;
    static objectType currentBusType = OBJ_LENGTH_COUNTER;
    static Bus *currentBus = nullptr;
    static obstacle *pObstacles[3] = {nullptr};
    movementDirection direction = NO_DIRECTION;

    switch (message) {
    case WM_CREATE:
        GetClientRect(hWnd, &rect);
        break;
    case WM_SIZE:
        GetClientRect(hWnd, &rect);
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_BUS:
            if (currentBusType == OBJ_BUS)
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
            if (currentBus) delete currentBus;
            currentBus = new Bus(5, 5);
            currentBusType = OBJ_BUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_ADVERTISINGBUS:
            if (currentBusType == OBJ_ADVERTISINGBUS)
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
            if (currentBus) delete currentBus;
            currentBus = new advertisingBus(5, 5);
            currentBusType = OBJ_ADVERTISINGBUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_MINIBUS:
            if (currentBusType == OBJ_MINIBUS)
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
            if (currentBus != nullptr) delete currentBus;
            currentBus = new miniBus(5, 5);
            currentBusType = OBJ_MINIBUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_DOUBLEDECKER:
            if (currentBusType == OBJ_DOUBLEDECKER)
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
            if (currentBus != nullptr) delete currentBus;
            currentBus = new doubleDecker(5, 5);
            currentBusType = OBJ_DOUBLEDECKER;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if (!objectsInitialized) {
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            pObstacles[0] = new trafficCone(50 + rand() % (width - 100), 50 + rand() % (height - 100));
            pObstacles[1] = new trafficCone(50 + rand() % (width - 100), 50 + rand() % (height - 100));
            pObstacles[2] = new busStop(50 + rand() % (width - 150), 50 + rand() % (height - 200));

            objectsInitialized = true;
        }
        if (currentBus) currentBus->draw(hdc);
        for (obstacle* obj : pObstacles)
            if (obj) obj->draw(hdc);
        EndPaint(hWnd, &ps);
        break;
    case WM_KEYDOWN:
    {
        hdc = GetDC(hWnd);

        if (!currentBus) {
            MessageBox(hWnd, L"Ошибка инициализации объектов", L"Информация", MB_OK | MB_ICONERROR);
            ReleaseDC(hWnd, hdc);
            break;
        }

        interactive* interactiveBus = dynamic_cast<interactive*>(currentBus);
        if (!interactiveBus) {
            MessageBox(hWnd, L"Ошибка: автобус не поддерживает взаимодействие", L"Информация", MB_OK | MB_ICONERROR);
            ReleaseDC(hWnd, hdc);
            break;
        }

        // Определяем направление движения
        movementDirection dir = currentBus->getDirection(wParam);

        // Если нажата клавиша движения - проверяем коллизии и двигаем
        if (dir != NO_DIRECTION) {
            // Проверяем коллизию со всеми объектами
            bool hasCollision = false;
            for (obstacle* obj : pObstacles) {
                if (obj && currentBus->willCollide(static_cast<void*>(obj), dir)) {
                    hasCollision = true;
                    break;  // Столкновение с любым объектом блокирует движение
                }
            }

            if (!hasCollision) {
                // Нет коллизий - закрашиваем текущую позицию белым и двигаем
                currentBus->clearCurrent(hdc);
                currentBus->driveTowards(dir);
            }
            // Если есть коллизия - не двигаемся (автобус останавливается)
        }
        else {
            // Обработка взаимодействия (+/-)
            // Поддерживаем разные коды клавиш: VK_OEM_PLUS, VK_ADD, VK_OEM_MINUS, VK_SUBTRACT
            bool isInteractionKey = (wParam == VK_OEM_PLUS || wParam == VK_ADD || 
                                     wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT);
            if (isInteractionKey) {
                for (obstacle* obj : pObstacles) {
                    if (obj) {
                        interactiveBus->interactWith(dynamic_cast<interactive*>(obj), wParam);
                    }
                }
            }
        }

        ReleaseDC(hWnd, hdc);
        
        // Перерисовываем окно
        InvalidateRect(hWnd, NULL, FALSE);
        
        break;
    }
	case WM_DESTROY:
        DestroyMenu(GetMenu(hWnd));
		for (auto& obj : pObstacles) {
			if (obj) {
				delete obj;
				obj = nullptr;
			}
		}
		if (currentBus) {
			delete currentBus;
			currentBus = nullptr;
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
