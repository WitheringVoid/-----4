#pragma once
#ifndef OBJECTS_H
#define OBJECTS_H

#include "interactive.h"
#include <Windows.h>

// Идентификаторы пунктов меню «Выбор» (тип автобуса)
#define IDM_BUS            40001
#define IDM_ADVERTISINGBUS 40002
#define IDM_MINIBUS        40003
#define IDM_DOUBLEDECKER   40004

// Forward declarations
class obstacle;
class busStop;

// ============================================================================
// ДЕМОНСТРАЦИЯ 4 ПРИНЦИПОВ ООП:
//
// 1. НАСЛЕДОВАНИЕ:
//    Location -> Point -> Bus -> miniBus / doubleDecker / specialBus -> advertisingBus
//    Location -> Point -> obstacle -> trafficCone / busStop
//    Повторное использование кода и расширение поведения классов-потомков.
//
// 2. АБСТРАКТНЫЕ КЛАССЫ:
//    Point — абстрактный класс: содержит чисто виртуальный метод draw(),
//    поэтому экземпляр Point создать нельзя, только наследников.
//    obstacle — тоже абстрактный: getType() оставлен чисто виртуальным.
//
// 3. ПОЛИМОРФИЗМ:
//    В WndProc все объекты хранятся как указатели на базу (obstacle*, Bus*),
//    а вызов obj->draw(hdc) / obj->getType() выполняет нужную реализацию
//    за счёт таблицы виртуальных функций (виртуальный dispatch).
//
// 4. ИНКАПСУЛЯЦИЯ:
//    Все поля (_x, _y, currentPassengers, peopleCount и т.д.) закрытые/защищённые,
//    доступ к ним только через открытые методы (getX(), addPassenger() и пр.).
//
// ЧЕТЫРЕ ЭТАПА РАЗРАБОТКИ (как программа развивалась до текущего вида):
//    Этап 1. Наследование: описана база Location -> Point -> Bus/obstacle,
//            движение и геометрия вынесены в общие методы предков.
//    Этап 2. Полиморфизм: draw()/getType()/interactWith() объявлены virtual,
//            каждый тип автобуса и препятствия переопределяет их под себя.
//    Этап 3. Интерфейсы: выделен чистый интерфейс interactive (только
//            чисто виртуальные методы + виртуальный деструктор), от него
//            наследуются Bus и obstacle; матрица взаимодействий — в interactive.
//    Этап 4. Абстрактные классы: Point и obstacle сделаны абстрактными
//            (чисто виртуальные draw/getType), конкретная реализация — только
//            в «листьях» иерархии; добавлена RAII-обёртка ScopedBrush для GDI.
// ============================================================================

// RAII-обёртка над GDI-кистью: создаёт и выбирает кисть в конструкторе,
// возвращает старую кисть и удаляет свою — в деструкторе. Убирает повторяющиеся
// CreateSolidBrush/SelectObject/DeleteObject из каждого метода draw().
struct ScopedBrush {
	HBRUSH _brush = nullptr;
	HBRUSH _old = nullptr;
	HDC    _dc = nullptr;

	explicit ScopedBrush(HDC hdc, COLORREF color) : _dc(hdc) {
		_brush = CreateSolidBrush(color);
		if (_brush) _old = (HBRUSH)SelectObject(_dc, _brush);
	}
	ScopedBrush(const ScopedBrush&) = delete;
	ScopedBrush& operator=(const ScopedBrush&) = delete;
	~ScopedBrush() {
		if (_brush) {
			if (_old) SelectObject(_dc, _old);
			DeleteObject(_brush);
		}
	}
};

// Инкапсуляция: координаты доступны только через геттеры
class Location {
protected:
	int _x, _y;
public:
	Location(int init_x, int init_y);
	int getX();
	int getY();
	~Location() = default;
};

// АБСТРАКТНЫЙ КЛАСС (этап 4): чисто виртуальный draw() запрещает
// создавать объекты типа Point напрямую.
class Point : public Location {
protected:
	bool _visible;
public:
	Point(int init_x = 0, int init_y = 0) : Location(init_x, init_y), _visible(false) {}
	bool moveTo(int new_x, int new_y);
	void toggleVisibility();
	virtual bool draw(HDC hdc) = 0;   // абстрактный метод — основа полиморфизма (этап 2)
	~Point() = default;
};

// Bus наследует геометрию у Point и «умение взаимодействовать» у интерфейса interactive
class Bus : public Point, public interactive {
protected:
	int length, height;
	int currentPassengers = 0, passengerCapacity = 20, speed = 0;

	COLORREF bodyColor = RGB(255, 0, 0);
	COLORREF windowColor = RGB(70, 70, 175);
	COLORREF tireColor = RGB(0, 0, 0);
public:
	Bus(int init_X, int init_Y);
	virtual ~Bus() = default;

	movementDirection getDirection(WPARAM);
	bool driveTowards(movementDirection direction);

	int getLength();
	int getHeight();
	int getPassengerCapacity();
	int getcurrentPassengers();
	virtual bool addPassenger();
	virtual bool removePassenger();
	void setSpeed(int new_speed);
	int getSpeed();

	interactionType interactWith(interactive* other, WPARAM wParam) override;
	bool draw(HDC hdc) override;
	objectType getType() const override;

	// Проверка коллизии с будущим положением.
	// type — тип препятствия: столкновение учитывается ТОЛЬКО с конусами
	// (OBJ_TRAFFICCONE); с остановкой наложение разрешено, иначе автобус
	// не смог бы «заехать» на остановку для посадки/высадки пассажиров.
	bool willCollide(obstacle* obs, movementDirection dir, objectType type);

protected:
	// Вспомогательный метод взаимодействия с остановкой (виртуальный для переопределения)
	virtual interactionType interactWithStop(busStop* stop, WPARAM wParam);

private:
	// Общий вывод табло «пассажиры/места» — уменьшает дублирование кода
	// в наследниках (наследование, этап 1).
	void drawPassengerSign(HDC hdc);
};

class miniBus : public Bus {
private:
	bool draw(HDC hdc) override;
public:
	miniBus(int init_X, int init_Y);
	objectType getType() const override;
};

class doubleDecker : public Bus {
private:
	bool draw(HDC hdc) override;
public:
	doubleDecker(int init_X, int init_Y);
	objectType getType() const override;
};

class specialBus : public Bus {
public:
	specialBus(int init_X, int init_Y);
};

class advertisingBus : public specialBus {
private:
	bool draw(HDC hdc) override;
public:
	advertisingBus(int init_X, int init_Y);
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override;

protected:
	interactionType interactWithStop(busStop* stop, WPARAM wParam) override;
};

// АБСТРАКТНЫЙ КЛАСС: getType() не реализован — создавать obstacle нельзя,
// только конкретные trafficCone / busStop.
class obstacle : public Point, public interactive {
protected:
	int _length, _height;
public:
	obstacle(int init_X, int init_Y, int init_length, int init_height);
	virtual bool checkCollision(Bus* vehicle);                 // проверка текущей коллизии
	virtual bool checkCollisionAt(Bus* vehicle, int x, int y); // проверка коллизии в точке
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override = 0;                   // абстрактный метод
	int getLength() const;
	int getHeight() const;
	virtual ~obstacle() = default;
};

class trafficCone : public obstacle {
private:
	COLORREF topColor = RGB(255, 255, 0);
	COLORREF bottomColor = RGB(255, 165, 0);
public:
	trafficCone(int init_X, int init_Y);
	bool draw(HDC hdc) override;
	// Осмысленная реализация интерфейса: конус — физическое препятствие,
	// при контакте с автобусом возвращает COLLISION_DETECTED.
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override;
};

class busStop : public obstacle {
private:
	COLORREF topColor = RGB(100, 100, 100);
	COLORREF bottomColor = RGB(200, 200, 200);
	COLORREF glassColor = RGB(100, 200, 255);
	int peopleCount = 5;
public:
	busStop(int init_X, int init_Y);
	int getPeopleCount();
	bool changePeopleCount(int difference);

	bool draw(HDC hdc) override;
	// Делегирует обработку взаимодействия автобусу: остановка сама не знает,
	// КАК сажать/высаживать пассажиров, — это ответственность Bus::interactWithStop.
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override;
};

#endif // !OBJECTS_H
