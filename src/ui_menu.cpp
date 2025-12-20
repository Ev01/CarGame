#include "ui_menu.h"
#include "ui.h"
#include "options.h"
#include "input_mapping.h"
#include "player.h"
#include "main_game.h"

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


MenuAction UI::Menu::GetSelectedMenuAction()
{
    return items[selectedIdx].action;
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
            //showAddPlayerDialog = true;
            OpenAddPlayerDialog();
            break;
        case MA_EXIT_WORLD:
            UI::MenuBack();
            MainGame::EndWorld();
            break;
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
