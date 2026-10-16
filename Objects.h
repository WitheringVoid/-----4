#pragma once
#ifndef OBJECTS_H
#define OBJECTS_H

#include "interactive.h"
#include <conio.h>
#include <Windows.h>

#define IDM_BUS            40001
#define IDM_ADVERTISINGBUS 40002
#define IDM_MINIBUS        40003
#define IDM_DOUBLEDECKER   40004

// Forward declaration
class obstacle;
class busStop;

class Location {
protected:
	int _x, _y;
public:
	Location(int init_x, int init_y);
	int getX();
	int getY();
	~Location() = default;
};

class Point : public Location {
protected: 
	bool _visible;
public:
	Point(int init_x = 0, int init_y = 0) : Location(init_x, init_y), _visible {false} {};
	bool moveTo(int new_x, int new_y);
	void toggleVisibility();
	virtual bool draw(HDC hdc) = 0;
	~Point() = default;
};

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
	void clearCurrent(HDC hdc);  // закрасить текущую позицию белым

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
	
	// Проверка коллизии с будущим положением
	bool willCollide(void* obs, movementDirection dir);
	
protected:
	// Вспомогательный метод для взаимодействия с остановкой (виртуальный для переопределения)
	virtual interactionType interactWithStop(busStop* stop, WPARAM wParam);
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

class obstacle : public Point, public interactive {
protected:
	int _length, _height;
public:
	obstacle(int init_X, int init_Y, int init_length, int init_height);
	virtual bool checkCollision(Bus *vehicle);  // проверка текущей коллизии
	virtual bool checkCollisionAt(Bus* vehicle, int x, int y);  // проверка коллизии в точке
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override = 0;
	virtual ~obstacle() = default;
};

class trafficCone : public obstacle {
private:
	COLORREF topColor = RGB(255, 255, 0);
	COLORREF bottomColor = RGB(255, 165, 0);
public:
	trafficCone(int init_X, int init_Y);
	bool draw(HDC hdc) override;
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
	interactionType interactWith(interactive* other, WPARAM wParam) override;
	objectType getType() const override;
};

#endif // !OBJECTS_H