#pragma once
#ifndef ENUMS_H
#define ENUMS_H

// Типы объектов сцены (используются, в частности, в матрице взаимодействий
// и при проверке коллизий: блокируются только конусы, остановка — «проезжаема»)
enum objectType {
    OBJ_BUS,
    OBJ_MINIBUS,
    OBJ_DOUBLEDECKER,
    OBJ_ADVERTISINGBUS,
    OBJ_TRAFFICCONE,
    OBJ_BUSSTOP,
    OBJ_LENGTH_COUNTER
};

// Результаты взаимодействия объектов
enum interactionType {
    NO_INTERACTION,
    COLLISION_DETECTED,
    PASSENGER_BOARD,
    PASSENGER_DROP_OFF,
    ADVERTISEMENT_ADD,
    ADVERTISEMENT_SUBTRACT,
    INTERACTION_ERROR,
    INTER_LENGTH_COUNTER
};

// Направления движения автобуса
enum movementDirection {
	MOVE_UP,
	MOVE_DOWN,
	MOVE_LEFT,
	MOVE_RIGHT,
	NO_DIRECTION
};

#endif // ENUMS_H
