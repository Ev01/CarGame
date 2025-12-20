#include "ui.h"
#include "ui_menu.h"

#include "main_game.h"
#include "world.h"
#include "player.h"
#include "input.h"
#include "input_mapping.h"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>



UI::Dialog addPlayerDialog = {
    "Press Up + Enter on keyboard or A on controller...",
    "Press Escape to cancel.",
    DIALOG_PLAYER_ADD
};

UI::Dialog endRaceDialog = {
    "Race over. Player %d wins",
    "Press Enter to continue...",
    DIALOG_RACE_END
};



UI::Dialog *currentDialog = nullptr;
//bool showAddPlayerDialog = false;
//bool showRaceEndDialog = false;


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





UI::Dialog* UI::GetCurrentDialog()
{
    return currentDialog;
}


void UI::CloseCurrentDialog()
{
    currentDialog = nullptr;
}


void UI::OpenDialog(Dialog *toOpen)
{
    currentDialog = toOpen;
}


void UI::OpenEndRaceDialog()
{
    OpenDialog(&endRaceDialog);
}


void UI::OpenAddPlayerDialog()
{
    OpenDialog(&addPlayerDialog);
}






void UI::HandleEvent(SDL_Event *event)
{
    if (currentDialog) {
        //AddPlayerDialogHandleEvent(event);
        currentDialog->HandleEvent(event);
    }
    else if (World::GetRaceState() == RACE_ENDED) {
        if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_ACCEPT)) {
            MainGame::EndWorld();
        }
    }
    else {
        UI::Menu *currentMenu = UI::GetCurrentMenu();
        if (currentMenu != nullptr) {
            currentMenu->HandleEvent(event);
        }
    }
}

static void AddPlayerDialogHandleEvent(SDL_Event *event)
{
    int newRealKeyboard = Input::ListenKeyboardPressJoin();
    SDL_Gamepad *gamepad = Input::ListenGamepadPressA(event);
    if (newRealKeyboard != 0) {
        int newPlayerId = Player::AddPlayer();
        gPlayers[newPlayerId].UseKeyboard(newRealKeyboard);
        //showAddPlayerDialog = false;
        UI::CloseCurrentDialog();
        SDL_Log("Keyboard pressed join");
    }
    else if (gamepad != nullptr) {
        int newPlayerId = Player::AddPlayer();
        gPlayers[newPlayerId].UseGamepad(gamepad);
        //showAddPlayerDialog = false;
        UI::CloseCurrentDialog();
    }
    //else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
    else if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_CANCEL)) {
        //showAddPlayerDialog = false;
        UI::CloseCurrentDialog();
    }
}

void UI::Dialog::HandleEvent(SDL_Event *event)
{
    switch (type) {
        case DIALOG_PLAYER_ADD:
            AddPlayerDialogHandleEvent(event);
            break;
        case DIALOG_RACE_END:
            if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_ACCEPT)) {
                MainGame::EndWorld();
                UI::CloseCurrentDialog();
            }
            break;
    }
}


void UI::Dialog::GetLine1(char *outText, int maxLen)
{
    switch (type) {
        case DIALOG_PLAYER_ADD:
            SDL_snprintf(outText, maxLen, "%s", line1);
            break;
        case DIALOG_RACE_END:
            SDL_snprintf(outText, maxLen, line1, World::GetRaceProgress().mWinningPlayerIdx + 1);
            break;
    }
}


void UI::Dialog::GetLine2(char *outText, int maxLen)
{
    switch (type) {
        case DIALOG_PLAYER_ADD:
        case DIALOG_RACE_END:
            SDL_snprintf(outText, maxLen, "%s", line2);
            break;
    }
}


void UI::Update()
{
    UI::Menu *currentMenu = UI::GetCurrentMenu();
    if (currentMenu != nullptr) {
        currentMenu->Update();
    }
}




