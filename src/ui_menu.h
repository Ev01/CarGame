#pragma once

#include <SDL3/SDL.h>

enum MenuAction {
    MA_NONE,
    MA_START_GAME,
    MA_OPEN_MENU,
    MA_BACK,
    MA_CYCLE_CHOICE,
    MA_ADD_PLAYER,
    MA_REMOVE_PLAYER,
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
        union {
            Menu *menuToOpen = NULL; // If action == MA_OPEN_MENU
            ChoiceOption *choiceOption; // If action == MA_CYCLE_CHOICE
            int playerToRemove; // If action == MA_REMOVE_PLAYER
        };

        void DoAction();
        /* Put the full display text in outText, including the choice value if
         * this is a CYCLE_CHOICE item. */
        void GetText(char *outText, int maxlen);
    };

    static constexpr int cMaxMenuItems = 8;
    struct Menu {
        MenuItem items[cMaxMenuItems];
        int numItems = 0;
        int selectedIdx = 0;
        void (*OnOpenFunc)(Menu *menu) = nullptr;

        void SelectNext();
        void SelectPrev();
        void ClearItems();
        void AddItem(MenuItem newItem);
        MenuAction GetSelectedMenuAction();
        MenuItem* GetSelectedMenuItem();
        void HandleEvent(SDL_Event *event);
        void Update();
    };

    void CloseAllMenus();
    void OpenMenu(Menu *toOpen);
    void MenuBack();
    Menu* GetCurrentMenu();
    Menu* GetPauseMenu();
    Menu* GetMainMenu();
}
