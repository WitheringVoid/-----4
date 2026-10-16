#include "interactive.h"

// Матрица взаимодействий: строка — инициатор, столбец — цель.
// Используется статическим методом interactive::getInteractionType.
interactionType interactive::interactionMatrix[OBJ_LENGTH_COUNTER][INTER_LENGTH_COUNTER] = {
    // BUS,               MINIBUS,            DOUBLEDECKER,         ADVERTISINGBUS,       TRAFFICCONE,          BUSSTOP
    {NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, COLLISION_DETECTED, PASSENGER_BOARD},        // BUS
    {NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, COLLISION_DETECTED, PASSENGER_BOARD},        // MINIBUS
    {NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, COLLISION_DETECTED, PASSENGER_BOARD},        // DOUBLEDECKER
    {NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, NO_INTERACTION, COLLISION_DETECTED, ADVERTISEMENT_ADD},      // ADVERTISINGBUS
    {COLLISION_DETECTED, COLLISION_DETECTED, COLLISION_DETECTED, COLLISION_DETECTED, NO_INTERACTION, NO_INTERACTION}, // TRAFFICCONE
    {PASSENGER_DROP_OFF, PASSENGER_DROP_OFF, PASSENGER_DROP_OFF, ADVERTISEMENT_SUBTRACT, NO_INTERACTION, NO_INTERACTION} // BUSSTOP
};

interactionType interactive::getInteractionType(objectType source, objectType target) {
    // Исправление: оба индекса — тип объекта (objectType), поэтому верхняя
    // граница для обоих — OBJ_LENGTH_COUNTER (раньше для target ошибочно
    // использовался размер INTER_LENGTH_COUNTER).
    if (source < 0 || source >= OBJ_LENGTH_COUNTER) return INTERACTION_ERROR;
    if (target < 0 || target >= OBJ_LENGTH_COUNTER) return INTERACTION_ERROR;
    return interactionMatrix[source][target];
}
