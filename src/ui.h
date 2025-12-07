#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

enum UIAnchor {
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
    MA_QUIT,
};


namespace UI {
    struct MenuItem {
        char text[64];
        MenuAction action;
    };


    struct Menu {
        MenuItem items[8];   
        int numItems = 0;
        int selectedIdx = 0;

        void SelectNext();
        void SelectPrev();
        MenuAction GetSelectedMenuAction();
        void HandleEvent(SDL_Event *event);
    };

    void DoMenuAction(MenuAction action);

    glm::vec2 GetPositionAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            float boundX, float boundY, float boundW, float boundH);

    extern Menu mainMenu;
}
