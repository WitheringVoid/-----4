#include "Objects.h"
#include <Windows.h>
#include <cwchar>   // swprintf / wcslen (без этого заголовка MSVC не видит swprintf)

// ------------------------------ Location / Point ----------------------------

Location::Location(int init_x, int init_y) {
	_x = init_x;
	_y = init_y;
}

int Location::getX() { return _x; }
int Location::getY() { return _y; }

bool Point::moveTo(int new_x, int new_y) {
	_x = new_x;
	_y = new_y;
	return true;
}

void Point::toggleVisibility() {
	_visible = !_visible;
}

// ----------------------------------- Bus ------------------------------------

Bus::Bus(int init_X, int init_Y) : Point(init_X, init_Y) {
	speed = 10;
	_visible = true;
	length = 200, height = 100;
}

void Bus::setSpeed(int new_speed) { speed = new_speed; }
int Bus::getSpeed() { return speed; }

int Bus::getLength() { return length; }
int Bus::getHeight() { return height; }

int Bus::getPassengerCapacity() { return passengerCapacity; }
int Bus::getcurrentPassengers() { return currentPassengers; }

bool Bus::addPassenger() {
	if (currentPassengers >= passengerCapacity) return false;
	currentPassengers++;
	return true;
}

bool Bus::removePassenger() {
	if (currentPassengers < 1) return false;
	currentPassengers--;
	return true;
}

objectType Bus::getType() const {
	return OBJ_BUS;
}

movementDirection Bus::getDirection(WPARAM wParam) {
	switch (wParam) {
	case VK_DOWN:   return MOVE_DOWN;
	case VK_UP:     return MOVE_UP;
	case VK_LEFT:   return MOVE_LEFT;
	case VK_RIGHT:  return MOVE_RIGHT;
	default:        return NO_DIRECTION;
	}
}

bool Bus::driveTowards(movementDirection direction) {
	switch (direction) {
	case MOVE_RIGHT: _x += speed; break;
	case MOVE_LEFT:  _x -= speed; break;
	case MOVE_DOWN:  _y += speed; break;
	case MOVE_UP:    _y -= speed; break;
	default: return false;
	}
	return true;
}

// Проверка коллизии автобуса с будущим положением.
// ИСПРАВЛЕНИЕ КРИТИЧЕСКОГО БАГА: столкновение учитывается только для конусов.
// С остановкой (OBJ_BUSSTOP) наложение разрешено — иначе автобус физически
// не смог бы въехать в остановку, а взаимодействие (посадка/высадка) требует
// именно пересечения с ней.
bool Bus::willCollide(obstacle* obs, movementDirection dir, objectType type) {
	if (!obs) return false;
	if (type != OBJ_TRAFFICCONE) return false;   // через остановку «проезжаем»

	int future_x = _x, future_y = _y;
	switch (dir) {
	case MOVE_LEFT:   future_x -= speed; break;
	case MOVE_RIGHT:  future_x += speed; break;
	case MOVE_UP:     future_y -= speed; break;
	case MOVE_DOWN:   future_y += speed; break;
	default: break;
	}

	return obs->checkCollisionAt(this, future_x, future_y);
}

// Взаимодействие с остановкой.
// ИСПРАВЛЕНИЕ КРИТИЧЕСКОГО БАГА («транзакционная» посадка): сначала проверяются
// ВСЕ условия (есть место в автобусе И есть люди на остановке), и только при
// успехе обоих изменяется состояние. Раньше пассажир мог «исчезнуть» с остановки,
// если автобус уже был полон.
interactionType Bus::interactWithStop(busStop* stop, WPARAM wParam) {
	interactionType result = NO_INTERACTION;

	if (!stop) return result;

	bool isPlusKey  = (wParam == VK_OEM_PLUS || wParam == VK_ADD);
	bool isMinusKey = (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT);

	if (isPlusKey) {
		// Посадка: место в автобусе проверяем ДО изменения счётчика остановки
		if (this->getcurrentPassengers() < this->getPassengerCapacity() &&
			stop->getPeopleCount() > 0) {
			this->addPassenger();
			stop->changePeopleCount(-1);
			result = PASSENGER_BOARD;
		}
	}
	else if (isMinusKey) {
		// Высадка: аналогично — сначала условия, потом изменение состояния
		if (this->getcurrentPassengers() > 0) {
			this->removePassenger();
			stop->changePeopleCount(1);
			result = PASSENGER_DROP_OFF;
		}
	}

	return result;
}

interactionType Bus::interactWith(interactive* other, WPARAM wParam) {
	if (!other) return INTERACTION_ERROR;

	objectType otherType = other->getType();
	obstacle* obs = dynamic_cast<obstacle*>(other);
	if (!obs) return NO_INTERACTION;

	// Взаимодействие возможно только с остановкой и только при пересечении
	if (otherType != OBJ_BUSSTOP || !obs->checkCollision(this)) {
		return NO_INTERACTION;
	}

	busStop* stop = dynamic_cast<busStop*>(other);
	return interactWithStop(stop, wParam);
}

// Общий вывод табло «пассажиры/места» для всех типов автобусов.
void Bus::drawPassengerSign(HDC hdc) {
SetTextColor(hdc, RGB(255, 255, 255));
SetBkMode(hdc, TRANSPARENT);
wchar_t passengersText[50];
swprintf(passengersText, 50, L"%d/%d", currentPassengers, passengerCapacity);
TextOut(hdc, _x + length - 40, _y + 5, passengersText, (int)wcslen(passengersText));
}

bool Bus::draw(HDC hdc) {
	if (!_visible) return false;

	{	// Кузов
		ScopedBrush body(hdc, bodyColor);
		Rectangle(hdc, _x, _y, _x + length, _y + height);
	}

	{	// Стёкла
		ScopedBrush win(hdc, windowColor);
		Rectangle(hdc, _x + 20, _y + 20, _x + 65, _y + 70);
		Rectangle(hdc, _x + 75, _y + 20, _x + 125, _y + 70);
		Rectangle(hdc, _x + 135, _y + 20, _x + 180, _y + 70);
	}

	{	// Колёса
		ScopedBrush tire(hdc, tireColor);
		Ellipse(hdc, _x + 30, _y + 85, _x + 60, _y + 115);
		Ellipse(hdc, _x + 130, _y + 85, _x + 160, _y + 115);
	}

	{	// Табло пассажиров
		ScopedBrush bg(hdc, tireColor);
		Rectangle(hdc, _x + length - 45, _y + 5, _x + length - 5, _y + 20);
	}

	drawPassengerSign(hdc);
	return true;
}

// -------------------------------- miniBus -----------------------------------

miniBus::miniBus(int init_X, int init_Y) : Bus(init_X, init_Y) {
	bodyColor = RGB(255, 156, 0);
	passengerCapacity = 10;
	length = 120, height = 60;
}

objectType miniBus::getType() const {
	return OBJ_MINIBUS;
}

bool miniBus::draw(HDC hdc) {
	if (!_visible) return false;

	{
		ScopedBrush body(hdc, bodyColor);
		Rectangle(hdc, _x, _y, _x + length, _y + height);
	}
	{
		ScopedBrush win(hdc, windowColor);
		Rectangle(hdc, _x + 10, _y + 10, _x + 40, _y + 40);
		Rectangle(hdc, _x + 45, _y + 10, _x + 75, _y + 40);
		Rectangle(hdc, _x + 80, _y + 10, _x + 110, _y + 40);
	}
	{
		ScopedBrush tire(hdc, tireColor);
		Ellipse(hdc, _x + 20, _y + 50, _x + 40, _y + 70);
		Ellipse(hdc, _x + 80, _y + 50, _x + 100, _y + 70);
	}
	{
		ScopedBrush bg(hdc, tireColor);
		Rectangle(hdc, _x + length - 45, _y + 5, _x + length - 5, _y + 20);
	}

	drawPassengerSign(hdc);
	return true;
}

// ------------------------------- doubleDecker -------------------------------

doubleDecker::doubleDecker(int init_X, int init_Y) : Bus(init_X, init_Y) {
	bodyColor = RGB(75, 156, 75);
	passengerCapacity = 30;
	length = 200, height = 100;
}

objectType doubleDecker::getType() const {
	return OBJ_DOUBLEDECKER;
}

bool doubleDecker::draw(HDC hdc) {
	if (!_visible) return false;

	{
		ScopedBrush body(hdc, bodyColor);
		Rectangle(hdc, _x, _y, _x + length, _y + height);
	}
	{
		ScopedBrush win(hdc, windowColor);
		// Верхний ряд окон
		Rectangle(hdc, _x + 20, _y + 10, _x + 65, _y + 40);
		Rectangle(hdc, _x + 75, _y + 10, _x + 125, _y + 40);
		Rectangle(hdc, _x + 135, _y + 10, _x + 180, _y + 40);
		// Нижний ряд окон
		Rectangle(hdc, _x + 75, _y + 50, _x + 125, _y + 80);
		Rectangle(hdc, _x + 135, _y + 50, _x + 180, _y + 80);
	}
	{
		ScopedBrush tire(hdc, tireColor);
		Ellipse(hdc, _x + 30, _y + 85, _x + 60, _y + 115);
		Ellipse(hdc, _x + 130, _y + 85, _x + 160, _y + 115);
	}
	{
		ScopedBrush bg(hdc, tireColor);
		Rectangle(hdc, _x + length - 45, _y + 5, _x + length - 5, _y + 20);
	}

	drawPassengerSign(hdc);
	return true;
}

// ------------------------------- specialBus ---------------------------------

specialBus::specialBus(int init_X, int init_Y) : Bus(init_X, init_Y) {}

// ------------------------------ advertisingBus ------------------------------

advertisingBus::advertisingBus(int init_X, int init_Y) : specialBus(init_X, init_Y) {
	bodyColor = RGB(255, 255, 0);
	passengerCapacity = 0;   // рекламный автобус не перевозит пассажиров
	length = 200, height = 100;
}

interactionType advertisingBus::interactWith(interactive* other, WPARAM wParam) {
	if (!other) return INTERACTION_ERROR;

	objectType otherType = other->getType();
	obstacle* obs = dynamic_cast<obstacle*>(other);
	if (!obs) return NO_INTERACTION;

	if (otherType != OBJ_BUSSTOP || !obs->checkCollision(this)) {
		return NO_INTERACTION;
	}

	busStop* stop = dynamic_cast<busStop*>(other);
	return interactWithStop(stop, wParam);
}

// Рекламный автобус «обменивает» людей на остановке на рекламу:
// плюс — люди смотрят рекламу и уходят с остановки, минус — возвращаются.
interactionType advertisingBus::interactWithStop(busStop* stop, WPARAM wParam) {
	interactionType result = NO_INTERACTION;

	if (!stop) return result;

	bool isPlusKey  = (wParam == VK_OEM_PLUS || wParam == VK_ADD);
	bool isMinusKey = (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT);

	if (isPlusKey && stop->getPeopleCount() > 0) {
		if (stop->changePeopleCount(-1)) {
			result = ADVERTISEMENT_ADD;
		}
	}
	else if (isMinusKey) {
		if (stop->changePeopleCount(1)) {
			result = ADVERTISEMENT_SUBTRACT;
		}
	}

	return result;
}

objectType advertisingBus::getType() const {
	return OBJ_ADVERTISINGBUS;
}

bool advertisingBus::draw(HDC hdc) {
	if (!_visible) return false;

	{
		ScopedBrush body(hdc, bodyColor);
		Rectangle(hdc, _x, _y, _x + length, _y + height);
	}
	{
		ScopedBrush win(hdc, windowColor);
		Rectangle(hdc, _x + 135, _y + 20, _x + 180, _y + 70);
	}
	{
		ScopedBrush tire(hdc, tireColor);
		Ellipse(hdc, _x + 30, _y + 85, _x + 60, _y + 115);
		Ellipse(hdc, _x + 130, _y + 85, _x + 160, _y + 115);
	}
	{	// Рекламный щит
		ScopedBrush ad(hdc, RGB(255, 255, 255));
		Rectangle(hdc, _x + 5, _y + 5, _x + 125, _y + 80);
	}

	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, TRANSPARENT);
	const wchar_t* lines[] = { L"СМОТРИ", L"  ЖИВЕ", L"   ЯРЧЕ" };
	int lineHeight = 15;
	int startY = _y + 15;
	for (int i = 0; i < 3; i++)
		TextOut(hdc, _x + 15, startY + (i * lineHeight), lines[i], (int)wcslen(lines[i]));
	return true;
}

// -------------------------------- obstacle ----------------------------------

obstacle::obstacle(int init_X, int init_Y, int init_length, int init_height)
	: Point(init_X, init_Y), _length(init_length), _height(init_height) {}

bool obstacle::checkCollision(Bus* vehicle) {
	return checkCollisionAt(vehicle, vehicle->getX(), vehicle->getY());
}

// AABB (Axis-Aligned Bounding Box) — пересечение прямоугольников
bool obstacle::checkCollisionAt(Bus* vehicle, int x, int y) {
	return (x < _x + _length && x + vehicle->getLength() > _x &&
		y < _y + _height && y + vehicle->getHeight() > _y);
}

int obstacle::getLength() const { return _length; }
int obstacle::getHeight() const { return _height; }

interactionType obstacle::interactWith(interactive* other, WPARAM wParam) {
	(void)other; (void)wParam;
	return NO_INTERACTION;
}

// ------------------------------- trafficCone --------------------------------

trafficCone::trafficCone(int init_x, int init_y) : obstacle(init_x, init_y, 30, 50) {
	_visible = true;
	_length = 30, _height = 50;
}

objectType trafficCone::getType() const {
	return OBJ_TRAFFICCONE;
}

// Осмысленная реализация интерфейса interactive: конус — физическое
// препятствие. Если его касается интерактивный объект (автобус),
// возвращаем COLLISION_DETECTED вместо бессмысленного NO_INTERACTION.
interactionType trafficCone::interactWith(interactive* other, WPARAM wParam) {
	(void)wParam;
	if (!other) return INTERACTION_ERROR;

	Bus* vehicle = dynamic_cast<Bus*>(other);
	if (vehicle && checkCollision(vehicle)) {
		return COLLISION_DETECTED;
	}
	return NO_INTERACTION;
}

bool trafficCone::draw(HDC hdc) {
	if (!_visible) return false;

	{	// Основание конуса
		ScopedBrush base(hdc, bottomColor);
		Rectangle(hdc, _x, _y + _height - 5, _x + _length, _y + _height);
	}

	{	// Вершина конуса
		POINT conePoints[3] = {
			{ _x + 15, _y },
			{ _x + 2,  _y + _height - 5 },
			{ _x + _length - 2, _y + _height - 5 }
		};
		ScopedBrush top(hdc, topColor);
		Polygon(hdc, conePoints, 3);
	}
	return true;
}

// --------------------------------- busStop ----------------------------------

busStop::busStop(int init_x, int init_y) : obstacle(init_x, init_y, 100, 150) {
	_visible = true;
	_length = 100, _height = 150;
}

int busStop::getPeopleCount() { return peopleCount; }

bool busStop::changePeopleCount(int difference) {
	if (peopleCount + difference < 0) return false;
	peopleCount += difference;
	return true;
}

objectType busStop::getType() const {
	return OBJ_BUSSTOP;
}

// Остановка делегирует обработку взаимодействия автобусу: она сама не знает,
// как сажать/высаживать пассажиров. Если автобус пересекает остановку,
// вызываем interactWith() самого автобуса (полиморфизм: для каждого типа
// автобуса сработает своя реализация).
interactionType busStop::interactWith(interactive* other, WPARAM wParam) {
	if (!other) return INTERACTION_ERROR;

	Bus* bus = dynamic_cast<Bus*>(other);
	if (bus && checkCollision(bus)) {
		return bus->interactWith(this, wParam);
	}
	return NO_INTERACTION;
}

bool busStop::draw(HDC hdc) {
	if (!_visible) return false;

	{	// Крыша
		ScopedBrush roof(hdc, topColor);
		Rectangle(hdc, _x, _y, _x + _length, _y + _height / 15);
	}
	{	// Стены
		ScopedBrush walls(hdc, bottomColor);
		Rectangle(hdc, _x + _length / 20, _y + _height / 15,
			_x + _length - (_length / 20), _y + _height);
	}
	{	// Стекло
		ScopedBrush glass(hdc, glassColor);
		Rectangle(hdc, _x + 20, _y + 20, _x + 80, _y + 100);
	}

	// Надписи
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, _x + 15, _y + 110, L"Остановка", 9);

	wchar_t peopleText[50];
	swprintf(peopleText, 50, L"Люди: %d", peopleCount);
	TextOut(hdc, _x + 15, _y + 130, peopleText, (int)wcslen(peopleText));
	return true;
}
