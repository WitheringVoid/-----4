#include "framework.h"
#include "ПРОГ 4.h"

#include <Windows.h>
#include <ctime>
#include <cstdlib>
#include "Objects.h"

#define MAX_LOADSTRING 100

// Минимально допустимые размеры клиентской области для корректной
// генерации препятствий (иначе rand() % отрицательное число — UB)
#define MIN_CLIENT_WIDTH  320
#define MIN_CLIENT_HEIGHT 320

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

   AppendMenu(hHelpMenu, MF_STRING, IDM_KEYSHELP, L"Клавиши управления");
   AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, L"О программе");
   AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Справка");

   SetMenu(hWnd, hMenu);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// AABB-пересечение прямоугольников (лев, верх, прав, низ)
static bool rectsOverlap(int l1, int t1, int r1, int b1,
                         int l2, int t2, int r2, int b2) {
    return (l1 < r2 && r1 > l2 && t1 < b2 && b1 > t2);
}

// Безопасная генерация позиции препятствия.
// ИСПРАВЛЕНИЕ КРИТИЧЕСКОГО БАГА: раньше конусы и остановка могли появиться
// друг на друге или поверх автобуса, а при маленьком окне выражение вида
// rand() % (width - 100) давало UB (отрицательный модуль).
// Здесь: проверка минимального размера окна, до MAX_ATTEMPTS попыток подбор
// случайных координат с проверкой пересечений (AABB) с автобусом и уже
// созданными препятствиями; если безопасная позиция не найдена — возвращаем
// запасную позицию в левом нижнем углу клиента.
static void findSafePosition(RECT client, Bus* bus, obstacle* existing[], int existingCount,
                              int objWidth, int objHeight, int& outX, int& outY) {
    const int MAX_ATTEMPTS = 300;

    int width  = client.right - client.left;
    int height = client.bottom - client.top;

    // Проверка минимального размера окна: при слишком маленьком окне
    // генерация невозможна — кладём объект в угол.
    if (width < MIN_CLIENT_WIDTH || height < MIN_CLIENT_HEIGHT) {
        outX = 5;
        outY = 5;
        return;
    }

    const int margin = 10;
    int maxX = width  - objWidth  - margin;
    int maxY = height - objHeight - margin;
    if (maxX < margin)   maxX = margin;
    if (maxY < margin)   maxY = margin;
    int spanX = maxX - margin + 1;
    int spanY = maxY - margin + 1;
    if (spanX <= 0) spanX = 1;
    if (spanY <= 0) spanY = 1;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        int x = margin + rand() % spanX;
        int y = margin + rand() % spanY;

        bool safe = true;

        // Не накладываем на текущий автобус
        if (bus && rectsOverlap(x, y, x + objWidth, y + objHeight,
                               bus->getX(), bus->getY(),
                               bus->getX() + bus->getLength(),
                               bus->getY() + bus->getHeight())) {
            safe = false;
        }

        // Не накладываем на уже созданные препятствия
        for (int i = 0; safe && i < existingCount; ++i) {
            obstacle* o = existing[i];
            if (o && rectsOverlap(x, y, x + objWidth, y + objHeight,
                                  o->getX(), o->getY(),
                                  o->getX() + o->getLength(),
                                  o->getY() + o->getHeight())) {
                safe = false;
            }
        }

        if (safe) {
            outX = x;
            outY = y;
            return;
        }
    }

    // Запасной вариант: вписываем объект в правый нижний угол по мере возможности
    outX = (maxX > margin) ? maxX : margin;
    outY = (maxY > margin) ? maxY : margin;
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
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    static PAINTSTRUCT ps;
    static RECT rect;

    static bool objectsInitialized = false;
    static objectType currentBusType = OBJ_LENGTH_COUNTER;
    static Bus *currentBus = nullptr;
    static obstacle *pObstacles[3] = {nullptr};

    switch (message) {
    case WM_CREATE:
        GetClientRect(hWnd, &rect);
        srand((unsigned int)time(nullptr));
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
        case IDM_KEYSHELP:
            MessageBox(hWnd,
                L"Управление симулятором:\n\n"
                L"Стрелки ← ↑ → ↓ — движение автобуса\n"
                L"+  — посадка пассажира (на остановке)\n"
                L"-  — высадка пассажира (на остановке)\n\n"
                L"Тип автобуса выбирается в меню «Выбор».\n"
                L"Для посадки/высадки подъедьте автобус к остановке.",
                L"Клавиши управления", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_BUS:
            if (currentBusType == OBJ_BUS) {
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
                break;
            }
            delete currentBus;
            currentBus = new Bus(5, 5);
            currentBusType = OBJ_BUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_ADVERTISINGBUS:
            if (currentBusType == OBJ_ADVERTISINGBUS) {
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
                break;
            }
            delete currentBus;
            currentBus = new advertisingBus(5, 5);
            currentBusType = OBJ_ADVERTISINGBUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_MINIBUS:
            if (currentBusType == OBJ_MINIBUS) {
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
                break;
            }
            delete currentBus;
            currentBus = new miniBus(5, 5);
            currentBusType = OBJ_MINIBUS;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case IDM_DOUBLEDECKER:
            if (currentBusType == OBJ_DOUBLEDECKER) {
                MessageBox(hWnd, L"Вы уже выбрали этот тип автобуса", L"Информация", MB_OK | MB_ICONINFORMATION);
                break;
            }
            delete currentBus;
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
            int x = 0, y = 0;

            // Безопасная генерация: без пересечений с автобусом и друг с другом
            findSafePosition(rect, currentBus, pObstacles, 0, 30, 50, x, y);
            pObstacles[0] = new trafficCone(x, y);

            findSafePosition(rect, currentBus, pObstacles, 1, 30, 50, x, y);
            pObstacles[1] = new trafficCone(x, y);

            findSafePosition(rect, currentBus, pObstacles, 2, 100, 150, x, y);
            pObstacles[2] = new busStop(x, y);

            objectsInitialized = true;
        }
        // ПОЛНАЯ ПЕРЕРИСОВКА: фон стирает Windows (bErase=TRUE в InvalidateRect),
        // здесь рисуем все объекты заново — сначала автобус, затем препятствия
        // (полиморфизм: draw() вызывает реализацию конкретного класса).
        if (currentBus) currentBus->draw(hdc);
        for (obstacle* obj : pObstacles)
            if (obj) obj->draw(hdc);
        EndPaint(hWnd, &ps);
        break;
    case WM_KEYDOWN:
    {
        if (!currentBus) {
            MessageBox(hWnd, L"Сначала выберите тип автобуса в меню «Выбор»", L"Информация", MB_OK | MB_ICONINFORMATION);
            break;
        }

        interactive* interactiveBus = dynamic_cast<interactive*>(currentBus);
        if (!interactiveBus) {
            MessageBox(hWnd, L"Ошибка: автобус не поддерживает взаимодействие", L"Информация", MB_OK | MB_ICONERROR);
            break;
        }

        // Определяем направление движения
        movementDirection dir = currentBus->getDirection(wParam);

        if (dir != NO_DIRECTION) {
            // ДВИЖЕНИЕ: проверяем столкновение ТОЛЬКО с конусами.
            // Через остановку автобус может «проехать» (наложение разрешено) —
            // иначе посадка/высадка были бы невозможны.
            bool hasCollision = false;
            for (obstacle* obj : pObstacles) {
                if (obj && obj->getType() == OBJ_TRAFFICCONE &&
                    currentBus->willCollide(obj, dir, obj->getType())) {
                    hasCollision = true;
                    break;
                }
            }

            if (!hasCollision) {
                currentBus->driveTowards(dir);
            }
            // Если есть коллизия с конусом — не двигаемся (автобус останавливается)
        }
        else {
            // Обработка взаимодействия (+/-)
            bool isInteractionKey = (wParam == VK_OEM_PLUS || wParam == VK_ADD ||
                                     wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT);
            if (isInteractionKey) {
                for (obstacle* obj : pObstacles) {
                    if (obj) {
                        // Делегирование: остановка сама вызывает interactWith автобуса
                        obj->interactWith(interactiveBus, wParam);
                    }
                }
            }
        }

        // ИСПРАВЛЕНИЕ: вместо закрашивания старой позиции белым цветом
        // (clearCurrent стирал и другие объекты) — полная перерисовка окна.
        // TRUE заставит Windows стереть фон, затем WM_PAINT перерисует всё.
        InvalidateRect(hWnd, NULL, TRUE);
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
