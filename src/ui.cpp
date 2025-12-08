#include "ui.h"

#include "main_game.h"
#include "world.h"
#include "options.h"

#include <glm/glm.hpp>



UI::Menu optionsMenu = {
    {
        {"Option 1", MA_NONE},
        {"Option 2", MA_NONE},
        {"Option 3", MA_NONE},
        {"Back",     MA_BACK}
    },
    4, 0
};

UI::Menu UI::mainMenu = {
    {
        {"Play",    MA_START_GAME}, 
        {"Starting Map", MA_CYCLE_CHOICE, nullptr, &World::gMapOption},
        {"Options", MA_OPEN_MENU, &optionsMenu}, 
        {"Quit",    MA_QUIT}, 
    },
    4, 0
};

UI::Menu pauseMenu = {
    {
        {"Resume", MA_BACK},
        {"Options", MA_OPEN_MENU, &optionsMenu},
        {"Quit Game", MA_QUIT},
    },
    3, 0
};


static constexpr int cMaxStackSize = 8;

UI::Menu* menuStack[cMaxStackSize];
int menuStackSize = 0;


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
            SelectNext();
        }
        else if (event->key.scancode == SDL_SCANCODE_UP) {
            SelectPrev();
        }
        else if (event->key.scancode == SDL_SCANCODE_RETURN) {
            //UI::DoMenuAction(GetSelectedMenuAction());
            GetSelectedMenuItem()->DoAction();
        }
    }
}

UI::MenuItem* UI::Menu::GetSelectedMenuItem()
{
    return &items[selectedIdx];
}


void UI::MenuItem::DoAction()
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
        case MA_OPEN_MENU:
            UI::OpenMenu(menuToOpen);
            break;
        case MA_BACK:
            UI::MenuBack();
            break;
        case MA_CYCLE_CHOICE:
            choiceOption->SelectNext();
            break;
    }
}

MenuAction UI::Menu::GetSelectedMenuAction()
{
    return items[selectedIdx].action;
}


void UI::OpenMenu(UI::Menu *toOpen)
{
    if (toOpen == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                "Warning: Tried to call UI::OpenMenu with nullptr");
        return;
    }
    if (menuStackSize == cMaxStackSize) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Warning: Cannot open menu. Already at max stack size!");
    }
    menuStack[menuStackSize++] = toOpen;
}


void UI::MenuBack()
{
    if (menuStackSize == 0) {
        return;
    }
    menuStack[menuStackSize - 1] = nullptr;
    menuStackSize--;
}


UI::Menu* UI::GetCurrentMenu()
{
    if (menuStackSize == 0) return nullptr;
    return menuStack[menuStackSize-1];
}


void UI::CloseAllMenus()
{
    while (GetCurrentMenu() != nullptr) MenuBack();
}


void UI::MenuItem::GetText(char *outText, int maxlen)
{
    if (action == MA_CYCLE_CHOICE) {
        // TODO: Clean this up
        SDL_snprintf(outText, maxlen, "%s: %s", text,
                choiceOption->optionStrings[choiceOption->selectedChoice]);
    } else {
        SDL_snprintf(outText, maxlen, "%s", text);
    }
}


UI::Menu* UI::GetPauseMenu() { return &pauseMenu; }
