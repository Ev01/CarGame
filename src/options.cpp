#include "options.h"

#include <SDL3/SDL.h>

const char* ChoiceOption::GetSelectedString()
{
    return optionStrings[selectedChoice];
}

void IntOption::Increase()
{
    value = SDL_min(value + 1, max);
}
void IntOption::Decrease()
{
    value = SDL_max(value - 1, min);
}
void IntOption::SetMin(int newMin)
{
    min = newMin;
    value = SDL_max(value, min);
}
void IntOption::SetMax(int newMax)
{
    max = newMax;
    value = SDL_min(value, max);
}
