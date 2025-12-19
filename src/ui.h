#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

enum UIAnchor : unsigned int {
    UI_ANCHOR_CENTRE       = 0,
    UI_ANCHOR_RIGHT        = 0b0001,
    UI_ANCHOR_LEFT         = 0b0010,
    UI_ANCHOR_TOP          = 0b0100,
    UI_ANCHOR_BOTTOM       = 0b1000,
    UI_ANCHOR_BOTTOM_LEFT  = UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT,
    UI_ANCHOR_TOP_LEFT     = UI_ANCHOR_TOP    | UI_ANCHOR_LEFT,
    UI_ANCHOR_BOTTOM_RIGHT = UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT,
    UI_ANCHOR_TOP_RIGHT    = UI_ANCHOR_TOP    | UI_ANCHOR_RIGHT,
};


enum MenuAction {
    MA_NONE,
    MA_START_GAME,
    MA_OPEN_MENU,
    MA_BACK,
    MA_CYCLE_CHOICE,
    MA_ADD_PLAYER,
    MA_QUIT,
};

/*
enum MenuItemType {
    MI_BUTTON,
    MI_CHOICE
};
*/

struct ChoiceOption;

struct Rect {
    float x = 0.0;
    float y = 0.0;
    float w = 0.0;
    float h = 0.0;
};

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

    //void DoMenuAction(MenuAction action);

    glm::vec2 GetPositionAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            Rect bound);
    Rect GetRectAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            Rect bound);
    void OpenMenu(Menu *toOpen);
    void MenuBack();
    Menu* GetCurrentMenu();
    void CloseAllMenus();
    void HandleEvent(SDL_Event *event);
    void Update();
    bool GetShowPlayerAddDialog();

    Menu* GetPauseMenu();

    extern Menu mainMenu;
}
