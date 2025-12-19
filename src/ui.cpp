#include "ui.h"

#include "main_game.h"
#include "world.h"
#include "options.h"
#include "player.h"
#include "input.h"
#include "input_mapping.h"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>



UI::Menu optionsMenu = {
    {
        //{"Add Player", MA_ADD_PLAYER_AND_VEHICLE},
        {"Option 1", MA_NONE},
        {"Option 2", MA_NONE},
        {"Option 3", MA_NONE},
        {"Back",     MA_BACK}
    },
    4, 0
};

UI::Menu UI::mainMenu = {
    {
        // Title         Action           Menu     ChoiceOption
        {"Play",         MA_START_GAME}, 
        {"Starting Map", MA_CYCLE_CHOICE, nullptr, &World::gMapOption},
        {"Add Player",   MA_ADD_PLAYER},
        {"Options",      MA_OPEN_MENU, &optionsMenu}, 
        {"Quit",         MA_QUIT}, 
    },
    5, 0
};

UI::Menu pauseMenu = {
    {
        {"Resume", MA_BACK},
        {"Options", MA_OPEN_MENU, &optionsMenu},
        {"Quit to Main Menu", MA_EXIT_WORLD},
        {"Quit Game", MA_QUIT},
    },
    4, 0
};


static constexpr int cMaxStackSize = 8;

UI::Menu* menuStack[cMaxStackSize];
int menuStackSize = 0;
bool showAddPlayerDialog = false;


glm::vec2 UI::GetPositionAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            Rect bound)
{
    glm::vec2 pos;
    //float boundW  = (float)screenWidth;
    //float boundH = (float)screenHeight;
    float boundR = bound.x + bound.w; /* right boundary */
    float boundT = bound.y + bound.h; /* top boundary   */

    if (anchor & UI_ANCHOR_LEFT) {
        pos.x = bound.x + margin.x;
    } 
    else if (anchor & UI_ANCHOR_RIGHT) {
        pos.x = boundR + margin.x - size.x;
    }
    else { // Centered horizontally
        pos.x = bound.x + bound.w / 2.0 - size.x / 2.0 + margin.x;
    }

    if (anchor & UI_ANCHOR_BOTTOM) {
        pos.y = bound.y + margin.y;
    }
    else if (anchor & UI_ANCHOR_TOP) {
        pos.y = boundT + margin.y - size.y;
    }
    else { // Centered vertically
        pos.y = bound.y + bound.h / 2.0 - size.y / 2.0 + margin.y;
    }

    return pos;
}

Rect UI::GetRectAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
        Rect bound)
{
    glm::vec2 bl;
    glm::vec2 tr;
    glm::vec2 blMargin = margin;
    glm::vec2 trMargin = margin;
    if (anchor & UI_ANCHOR_LEFT) {
        trMargin.x += size.x;
    }
    else if (anchor & UI_ANCHOR_RIGHT) {
        blMargin.x -= size.x;
    }
    else {
        blMargin.x -= size.x / 2.0;
        trMargin.x += size.x / 2.0;
    }
    if (anchor & UI_ANCHOR_BOTTOM) {
        trMargin.y += size.y;
    }
    else if (anchor & UI_ANCHOR_TOP) {
        blMargin.y -= size.y;
    }
    else {
        blMargin.y -= size.y / 2.0;
        trMargin.y += size.y / 2.0;
    }
    bl = GetPositionAnchored(glm::vec2(0, 0), blMargin, anchor,
            bound);
    tr = GetPositionAnchored(glm::vec2(0, 0), trMargin, anchor,
            bound);

    return {bl.x, bl.y, tr.x - bl.x, tr.y - bl.y};
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
    if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_DOWN)) {
        SelectNext();
    }
    if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_UP)) {
        SelectPrev();
    }
    if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_ACCEPT)) {
        GetSelectedMenuItem()->DoAction();
    }


}

void UI::Menu::Update()
{
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
            if (gNumPlayers > 0) {
                MainGame::StartWorld();
            }
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
        case MA_ADD_PLAYER:
            //Player::AddPlayer();
            showAddPlayerDialog = true;
            break;
        case MA_EXIT_WORLD:
            UI::MenuBack();
            MainGame::EndWorld();
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


void UI::HandleEvent(SDL_Event *event)
{
    if (showAddPlayerDialog) {
        int newRealKeyboard = Input::ListenKeyboardPressJoin();
        SDL_Gamepad *gamepad = Input::ListenGamepadPressA(event);
        if (newRealKeyboard != 0) {
            int newPlayerId = Player::AddPlayer();
            gPlayers[newPlayerId].UseKeyboard(newRealKeyboard);
            showAddPlayerDialog = false;
            SDL_Log("Keyboard pressed join");
        }
        else if (gamepad != nullptr) {
            int newPlayerId = Player::AddPlayer();
            gPlayers[newPlayerId].UseGamepad(gamepad);
            showAddPlayerDialog = false;
        }
        //else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        else if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_CANCEL)) {
            showAddPlayerDialog = false;
        }
    }
    else {
        UI::Menu *currentMenu = UI::GetCurrentMenu();
        if (currentMenu != nullptr) {
            currentMenu->HandleEvent(event);
        }
    }
}


void UI::Update()
{
    UI::Menu *currentMenu = UI::GetCurrentMenu();
    if (currentMenu != nullptr) {
        currentMenu->Update();
    }
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
bool UI::GetShowPlayerAddDialog() { return showAddPlayerDialog; }
