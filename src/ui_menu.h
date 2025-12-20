#pragma once

#include <SDL3/SDL.h>

enum MenuAction {
    MA_NONE,
    MA_START_GAME,
    MA_OPEN_MENU,
    MA_BACK,
    MA_CYCLE_CHOICE,
    MA_ADD_PLAYER,
    MA_QUIT,
    MA_EXIT_WORLD,
};

// Forward declarations
struct ChoiceOption;


namespace UI {
    struct Menu;


    struct MenuItem {
        char text[64];
        MenuAction action = MA_NONE;
        Menu *menuToOpen  = nullptr; // If action == MA_OPEN_MENU
        ChoiceOption *choiceOption = nullptr; // If action == MA_CYCLE_CHOICE

        void DoAction();
        /* Put the full display text in outText, including the choice value if
         * this is a CYCLE_CHOICE item. */
        void GetText(char *outText, int maxlen);
    };


    struct Menu {
        MenuItem items[8];   
        int numItems = 0;
        int selectedIdx = 0;

        void SelectNext();
        void SelectPrev();
        MenuAction GetSelectedMenuAction();
        MenuItem* GetSelectedMenuItem();
        void HandleEvent(SDL_Event *event);
        void Update();
    };
}
