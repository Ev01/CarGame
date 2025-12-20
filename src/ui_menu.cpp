#include "ui_menu.h"
#include "ui.h"
#include "options.h"
#include "input_mapping.h"
#include "player.h"
#include "main_game.h"
#include "world.h"

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

static void RemovePlayerMenuOnOpen(UI::Menu *menu)
{
    menu->ClearItems();
    for (int i = 0; i < gNumPlayers; i++) {
        UI::MenuItem newItem;
        newItem.action = MA_REMOVE_PLAYER;
        newItem.playerToRemove = i;
        SDL_snprintf(newItem.text, sizeof(newItem.text), "Remove Player %d", i+1);
        menu->AddItem(newItem);
    }
    menu->AddItem({"Back", MA_BACK});
}
UI::Menu removePlayerMenu = {
    {}, 0, 0, RemovePlayerMenuOnOpen
};

UI::Menu mainMenu = {
    {
        // Title             Action                             Menu / ChoiceOption
        {"Play",             MA_START_GAME}, 
        {"Starting Map",     MA_CYCLE_CHOICE, {.choiceOption = &World::gMapOption}},
        {"Num Laps",         MA_INC_INTOPTION,{.intOption = &World::gLapsOption}, 
            MA_DEC_INTOPTION, MA_INC_INTOPTION},
        {"Add Player",       MA_ADD_PLAYER},
        {"Remove Player",    MA_OPEN_MENU,    {.menuToOpen = &removePlayerMenu}},
        {"Options",          MA_OPEN_MENU,    {.menuToOpen = &optionsMenu}}, 
        {"Quit",             MA_QUIT}, 
    },
    7, 0
};

UI::Menu pauseMenu = {
    {
        {"Resume",            MA_BACK},
        {"Options",           MA_OPEN_MENU, {.menuToOpen = &optionsMenu}},
        {"Quit to Main Menu", MA_EXIT_WORLD},
        {"Quit Game",         MA_QUIT},
    },
    4, 0
};


static constexpr int cMaxStackSize = 8;
int menuStackSize = 0;
UI::Menu* menuStack[cMaxStackSize];

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
        GetSelectedMenuItem()->DoAction(ACTION_UI_ACCEPT);
    }
    if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_LEFT)) {
        GetSelectedMenuItem()->DoAction(ACTION_UI_LEFT);
    }
    if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_UI_RIGHT)) {
        GetSelectedMenuItem()->DoAction(ACTION_UI_RIGHT);
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
void UI::Menu::ClearItems()
{
    numItems = 0;
    selectedIdx = 0;
}
void UI::Menu::AddItem(MenuItem newItem)
{
    if (numItems < UI::cMaxMenuItems) {
        items[numItems++] = newItem;
    }
}



void UI::MenuItem::DoAction(GameAction inputAction)
{
    MenuAction actionToUse;
    switch (inputAction) {
        case ACTION_UI_ACCEPT:
            actionToUse = action;
            break;
        case ACTION_UI_LEFT:
            actionToUse = leftAction;
            break;
        case ACTION_UI_RIGHT:
            actionToUse = rightAction;
            break;
        default:
            return;
    }

    switch (actionToUse) {
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
        case MA_INC_INTOPTION:
            intOption->Increase();
            break;
        case MA_DEC_INTOPTION:
            intOption->Decrease();
            break;
        case MA_ADD_PLAYER:
            //Player::AddPlayer();
            //showAddPlayerDialog = true;
            OpenAddPlayerDialog();
            break;
        case MA_REMOVE_PLAYER:
            SDL_Log("Remove player %d", playerToRemove);
            Player::RemovePlayer(playerToRemove);
            UI::MenuBack();
            break;
        case MA_EXIT_WORLD:
            UI::MenuBack();
            MainGame::EndWorld();
            break;
    }
}

void UI::MenuItem::GetText(char *outText, int maxlen)
{
    // TODO: Could be good if there was a separate menuItemType member to
    // differentiate between these.
    switch (action) {
        case MA_CYCLE_CHOICE:
            SDL_snprintf(outText, maxlen, "%s: %s", text,
                    choiceOption->GetSelectedString());
            break;
        case MA_INC_INTOPTION:
        case MA_DEC_INTOPTION:
            SDL_snprintf(outText, maxlen, "%s: %d", text, intOption->value);
            break;
        default:
            SDL_snprintf(outText, maxlen, "%s", text);
            break;
    }
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
    
    if (toOpen->OnOpenFunc != nullptr) {
        toOpen->OnOpenFunc(toOpen);
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

UI::Menu* UI::GetPauseMenu() { return &pauseMenu; }
UI::Menu* UI::GetMainMenu()  { return &mainMenu; }
