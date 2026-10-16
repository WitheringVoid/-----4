#pragma once
#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "enums.h"
#include <Windows.h>

interactionType getNextInteractionType(interactionType current);

class interactive {
private:
    static interactionType interactionMatrix[OBJ_LENGTH_COUNTER][INTER_LENGTH_COUNTER];
public:
    virtual objectType getType() const = 0;
    virtual interactionType interactWith(interactive* other, WPARAM wParam) = 0;
    static interactionType getInteractionType(objectType object1, objectType object2);
    virtual ~interactive() = default;
};

#endif // !INTERACTIVE_H