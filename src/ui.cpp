#include "ui.h"

#include "main_game.h"

#include <glm/glm.hpp>

UI::Menu UI::mainMenu = {
    {
        {"Play",    MA_START_GAME}, 
        {"Options", MA_NONE}, 
        {"Quit",    MA_QUIT}, 
    },
    3, 0
};


glm::vec2 UI::GetPositionAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            float boundX, float boundY, float boundW, float boundH)
{
    glm::vec2 pos;
    //float boundW  = (float)screenWidth;
    //float boundH = (float)screenHeight;
    float boundR = boundX + boundW; /* right boundary */
    float boundT = boundY + boundH; /* top boundary   */

    if (anchor & UI_ANCHOR_LEFT) {
        pos.x = boundX + margin.x + size.x;
    } 
    else if (anchor & UI_ANCHOR_RIGHT) {
        pos.x = boundR + margin.x - size.x;
    }
    else { // Centered horizontally
        pos.x = boundX + boundW / 2.0 - size.x / 2.0 + margin.x;
    }

    if (anchor & UI_ANCHOR_BOTTOM) {
        pos.y = boundY + margin.y + size.y;
    }
    else if (anchor & UI_ANCHOR_TOP) {
        pos.y = boundT + margin.y - size.y;
    }
    else { // Centered vertically
        pos.y = boundY + boundH / 2.0 - size.y / 2.0 + margin.y;
    }

    return pos;
}

void UI::Menu::SelectNext()
{
   selectedIdx++;
   if (selectedIdx >= numItems) selectedIdx = 0;
}
void UI::Menu::SelectPrev()
{
   selectedIdx--;
   if (selectedIdx < 0) selectedIdx = numItems - 1;
}

void UI::Menu::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_DOWN) {
            mainMenu.SelectNext();
        }
        else if (event->key.scancode == SDL_SCANCODE_UP) {
            mainMenu.SelectPrev();
        }
        else if (event->key.scancode == SDL_SCANCODE_RETURN) {
            UI::DoMenuAction(GetSelectedMenuAction());
        }
    }
}

void UI::DoMenuAction(MenuAction action)
{
    switch (action) {
        case MA_NONE:
            break;
        case MA_START_GAME:
            MainGame::StartWorld();
            break;
        case MA_QUIT:
            MainGame::Quit();
            break;
    }
}

MenuAction UI::Menu::GetSelectedMenuAction()
{
    return items[selectedIdx].action;
}
