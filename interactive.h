#pragma once
#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "enums.h"
#include <Windows.h>

// ============================================================================
// ИНТЕРФЕЙС (чистый виртуальный класс) — этап 3 разработки.
// Содержит ТОЛЬКО чисто виртуальные методы и виртуальный деструктор,
// поэтому не может иметь экземпляров. Классы Bus и obstacle наследуют
// этот интерфейс и обязательны к реализации его методов — полиморфизм:
// WndProc работает через interactive*, не зная конкретного типа объекта.
// ============================================================================
class interactive {
private:
    // Матрица «кто с кем как взаимодействует» (строка — инициатор, столбец — цель)
    static interactionType interactionMatrix[OBJ_LENGTH_COUNTER][INTER_LENGTH_COUNTER];
public:
    virtual objectType getType() const = 0;                          // абстрактный метод
    virtual interactionType interactWith(interactive* other, WPARAM wParam) = 0;  // абстрактный метод
    static interactionType getInteractionType(objectType object1, objectType object2);
    virtual ~interactive() = default;                                // виртуальный деструктор для корректного delete через указатель на базу
};

#endif // !INTERACTIVE_H
