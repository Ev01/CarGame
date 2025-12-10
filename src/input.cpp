#include "input.h"
#include "player.h"

#include "../vendor/imgui/imgui.h"
#include <SDL3/SDL.h>

#include <unordered_set>

#define DEADZONE 0.03
#define MAX_GAMEPADS 8
#define MAX_KEYBOARDS 4

bool scancodesDown[SDL_SCANCODE_COUNT];
SDL_Gamepad *gamepads[MAX_GAMEPADS];
SDL_KeyboardID keyboards[MAX_KEYBOARDS];
// A physical keyboard can have multiple SDL_KeyboardIDs. For example a gaming
// keyboard might have one ID for the arrow keys and another ID for the other
// keys. Each set groups together the IDs for a single keyboard.
//std::unordered_set<SDL_KeyboardID> realKeyboards[MAX_KEYBOARDS];
RealKeyboard realKeyboards[MAX_KEYBOARDS];
int numRealKeyboards = 0;
int RealKeyboard::idCounter = 0;

int numKeyboards = 0;
int numOpenedGamepads = 0;
//InputMappingKeyboard gDefaultKeyboardMapping;

Uint8 keyboardStates[MAX_KEYBOARDS][SDL_SCANCODE_COUNT];
enum {
    KEY_STATE_FLAG_PRESSED       = 1,
    KEY_STATE_FLAG_JUST_PRESSED  = 2,
    KEY_STATE_FLAG_JUST_RELEASED = 4
};

ControlScheme gDefaultControlScheme;


void InputMapping::AssignMapping(SDL_Scancode aScancode)
{
    mappingType = MAPPING_TYPE_SCANCODE;
    scancode = aScancode;
}
void InputMapping::AssignMapping(SDL_GamepadButton aButton)
{
    mappingType = MAPPING_TYPE_GAMEPAD_BUTTON;
    button = aButton;
}
void InputMapping::AssignMapping(SDL_GamepadAxis aAxis, bool isAxisPos)
{
    if (isAxisPos) {
        mappingType = MAPPING_TYPE_GAMEPAD_AXIS_POS;
    }
    else {
        mappingType = MAPPING_TYPE_GAMEPAD_AXIS_NEG;
    }
    axis = aAxis;
}
float InputMapping::GetValue(SDL_Gamepad *gamepad)
{
    if (gamepad == nullptr && mappingType != MAPPING_TYPE_SCANCODE) {
        return 0.0;
    }

    switch (mappingType) {
        case MAPPING_TYPE_SCANCODE:
            // Only enable keyboard when not given controller to use
            /*
            if (gamepad == nullptr) {
                return (float) Input::IsScanDown(scancode);
            }
            */
            break;
        case MAPPING_TYPE_GAMEPAD_AXIS_POS:
            return Input::GetGamepadAxisPos(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_AXIS_NEG:
            return Input::GetGamepadAxisNeg(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_BUTTON:
            return Input::GetGamepadButton(gamepad, button);
        case MAPPING_TYPE_NONE:
            return 0.0;
    }

    return 0.0;
}
float InputMapping::GetValue(int realKeyboardID)
{
    //if (realKeyboardID == 0 || mappingType != MAPPING_TYPE_SCANCODE) {
    if (mappingType != MAPPING_TYPE_SCANCODE) {
        return 0.0;
    }
    if (realKeyboardID == 0) {
        // If the keyboard ID is invalid, default to SDL's internal keyboard
        // state, which does not differentiate between different keyboards.
        return (float) SDL_GetKeyboardState(NULL)[scancode];
    }
    
    RealKeyboard *realKeyboard = RealKeyboard::GetByID(realKeyboardID);
    if (realKeyboard != nullptr) {
        return (float) realKeyboard->IsScanDown(scancode);
    } else {
        return 0.0;
    }
}


void InputAction::AddMapping(SDL_Scancode scancode)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(scancode);
}
void InputAction::AddMapping(SDL_GamepadButton button)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(button);
}
void InputAction::AddMapping(SDL_GamepadAxis axis, bool isPosAxis)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(axis, isPosAxis);
}
float InputAction::GetValue(SDL_Gamepad *gamepad) 
{
    float toReturn = 0.0;
    for (int i = 0; i < numMappingsSet; i++) {
        // Choose the mapping with the greatest input value. This should
        // work fine in most occasions
        toReturn = SDL_max(toReturn, mappings[i].GetValue(gamepad));
    }
    return toReturn;
}
float InputAction::GetValue(int realKeyboardID) 
{
    /*
    if (realKeyboardID == 0) {
        return 0.0;
    }
    */
    float toReturn = 0.0;
    for (int i = 0; i < numMappingsSet; i++) {
        // Choose the mapping with the greatest input value. This should
        // work fine in most occasions
        toReturn = SDL_max(toReturn, mappings[i].GetValue(realKeyboardID));
    }
    return toReturn;
}
void InputAction::ClearMappings()
{
    numMappingsSet = 0;
}
void InputAction::HandleEvent(SDL_Event *event)
{
    /*
    for (int i = 0; i < numMappingsSet; i++) {
        switch (mappings[i].mappingType) {
            case MAPPING_TYPE_SCANCODE:
                if (event->type == SDL_EVENT_KEY_DOWN 
                        || event->type == SDL_EVENT_KEY_UP) {
                    lastPressed = i;
                    return;
                }
                break;

            case MAPPING_TYPE_GAMEPAD_BUTTON:
                if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN
                        || event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
                    lastPressed = i;
                    return;
                }
                break;
            
            case MAPPING_TYPE_GAMEPAD_AXIS_NEG:
            case MAPPING_TYPE_GAMEPAD_AXIS_POS:
                if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                    lastPressed = i;
                    return;
                }
                break;
            case MAPPING_TYPE_NONE:
        }
    }
    */

}


/* a
static void RegisterKeyboard(SDL_KeyboardID keyboardID)
{
    if (numKeyboards >= MAX_KEYBOARDS) return;
    // Check if keyboard isn't already registered
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboards[i] == keyboardID) return;
    }
    // keyboardID of 0 is invalid
    if (keyboardID == 0) return;
    keyboards[numKeyboards++] = keyboardID;
    SDL_Log("Registered keyboard %d", keyboardID);
}
*/

/*pawfhe
static void RegisterRealKeyboard(
        const std::unordered_set<SDL_KeyboardID>& realKeyboard)
{
    if (numRealKeyboards >= MAX_KEYBOARDS) return;
    realKeyboards[numRealKeyboards++] = realKeyboard;
}
*/


void RealKeyboard::Init()
{
    // Called Init on already inited keyboard.
    SDL_assert(rawKeyboardIDs.size() == 0);
    SDL_assert(id == 0);

    id = ++RealKeyboard::idCounter;
}
void RealKeyboard::Deinit()
{
    id = 0;
    rawKeyboardIDs.clear();
    // Reset keyState
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        keyState[i] = 0;
    }
}

bool RealKeyboard::ContainsRawKeyboard(SDL_KeyboardID rawID)
{
    return rawKeyboardIDs.find(rawID) != rawKeyboardIDs.end();
}
void RealKeyboard::HandleEvent(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP
            || !ContainsRawKeyboard(event->key.which)) {
        return;
    }

    // Reset key state pressed flag
    keyState[event->key.scancode] &= ~KEY_STATE_FLAG_PRESSED;
    if (event->key.down) {
        keyState[event->key.scancode] |= KEY_STATE_FLAG_PRESSED;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        keyState[event->key.scancode] |= KEY_STATE_FLAG_JUST_PRESSED;
    }
    if (event->type == SDL_EVENT_KEY_UP) {
        keyState[event->key.scancode] |= KEY_STATE_FLAG_JUST_RELEASED;
    }
    
    if (event->key.scancode == SDL_SCANCODE_RETURN) {
        SDL_Log("Return state: %d", keyState[event->key.scancode]);
    }
}
void RealKeyboard::NewFrame()
{
    for (int j = 0; j < SDL_SCANCODE_COUNT; j++) {
        // Erase all flags except for pressed flag
        keyState[j] &= KEY_STATE_FLAG_PRESSED;
    }
}
RealKeyboard* RealKeyboard::GetByID(int id)
{
    for (int i = 0; i < MAX_KEYBOARDS; i++) {
        if (!realKeyboards[i].IsValid()) {
            continue;
        }
        if (realKeyboards[i].id == id) {
            return &realKeyboards[i];
        }
    }
    return nullptr;
}
const char* RealKeyboard::GetName()
{
    return SDL_GetKeyboardNameForID(*rawKeyboardIDs.begin());
}
bool RealKeyboard::IsScanDown(SDL_Scancode scancode)
{
    if (!IsValid()) return false;
    return keyState[scancode] & KEY_STATE_FLAG_PRESSED;
}


// rawIDs: An array of SDL_KeyboardIDs that belong to this real keyboard.
// count: The size of rawIDs array.
static void RegisterRealKeyboard(const SDL_KeyboardID *rawIDs,
                                 unsigned int count)
{
    SDL_assert(count > 0);
    if (numRealKeyboards >= MAX_KEYBOARDS) return;

    int idx = 0;
    // Get the idx of the next unused keyboard slot
    while (idx < MAX_KEYBOARDS && realKeyboards[idx].IsValid()) idx++;

    realKeyboards[idx].Init();   
    SDL_Log("Registering real keyboard...");
    for (unsigned int i = 0; i < count; i++) {
        SDL_Log("Raw keyboard id %d", rawIDs[i]);
        realKeyboards[idx].rawKeyboardIDs.insert(rawIDs[i]);
    }
    SDL_Log("End register");
    numRealKeyboards++;
}


float ControlScheme::GetInputForAction(GameAction action, SDL_Gamepad *gamepad)
{
    return actions[action].GetValue(gamepad);
}

float ControlScheme::GetInputForAction(GameAction action,
                                       int realKeyboardID)
{
    return actions[action].GetValue(realKeyboardID);
}

float ControlScheme::GetSignedInputForAction(GameAction negAction,
                                             GameAction posAction, 
                                             SDL_Gamepad *gamepad)
{
    return actions[posAction].GetValue(gamepad) 
         - actions[negAction].GetValue(gamepad);
}

float ControlScheme::GetSignedInputForAction(GameAction negAction,
                                             GameAction posAction, 
                                             int realKeyboardID)
{
    return actions[posAction].GetValue(realKeyboardID) 
         - actions[negAction].GetValue(realKeyboardID);
}



void Input::Init()
{
    //RealKeyboard::idCounter = 0;
    gDefaultControlScheme.actions[ACTION_STEER_LEFT].AddMapping(
            SDL_GAMEPAD_AXIS_LEFTX, false);
    gDefaultControlScheme.actions[ACTION_STEER_LEFT].AddMapping(
            SDL_SCANCODE_LEFT);
    gDefaultControlScheme.actions[ACTION_STEER_RIGHT].AddMapping(
            SDL_GAMEPAD_AXIS_LEFTX, true);
    gDefaultControlScheme.actions[ACTION_STEER_RIGHT].AddMapping(
            SDL_SCANCODE_RIGHT);

    gDefaultControlScheme.actions[ACTION_BRAKE].AddMapping(
            SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    gDefaultControlScheme.actions[ACTION_BRAKE].AddMapping(
            SDL_SCANCODE_DOWN);

    gDefaultControlScheme.actions[ACTION_FORWARD].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    gDefaultControlScheme.actions[ACTION_FORWARD].AddMapping(
            SDL_SCANCODE_UP);

    gDefaultControlScheme.actions[ACTION_HANDBRAKE].AddMapping(
            SDL_GAMEPAD_BUTTON_SOUTH);
    gDefaultControlScheme.actions[ACTION_HANDBRAKE].AddMapping(
            SDL_SCANCODE_SPACE);

    
    gDefaultControlScheme.actions[ACTION_LOOK_LEFT].AddMapping(
            SDL_SCANCODE_A);
    gDefaultControlScheme.actions[ACTION_LOOK_LEFT].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHTX, false);

    gDefaultControlScheme.actions[ACTION_LOOK_RIGHT].AddMapping(
            SDL_SCANCODE_D);
    gDefaultControlScheme.actions[ACTION_LOOK_RIGHT].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHTX, true);
}


bool Input::IsScanDown(SDL_Scancode scancode)
{
    return scancodesDown[scancode];
}


bool Input::IsScanDown(SDL_Scancode scancode, SDL_KeyboardID keyboardID)
{
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboards[i] == keyboardID) {
            return keyboardStates[i][scancode] & KEY_STATE_FLAG_PRESSED;
        }
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "keyboard with id %d not registered", keyboardID);
    return false;
}


bool Input::IsScanJustPressed(SDL_Scancode scancode, SDL_KeyboardID keyboardID)
{
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboards[i] == keyboardID) {
            return keyboardStates[i][scancode] & KEY_STATE_FLAG_JUST_PRESSED;
        }
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "keyboard with id %d not registered", keyboardID);
    return false;
}


bool Input::IsScanJustReleased(SDL_Scancode scancode, SDL_KeyboardID keyboardID)
{
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboards[i] == keyboardID) {
            return keyboardStates[i][scancode] & KEY_STATE_FLAG_JUST_RELEASED;
        }
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "keyboard with id %d not registered", keyboardID);
    return false;
}


void Input::NewFrame()
{
    for (int i = 0; i < numKeyboards; i++) {
        for (int j = 0; j < SDL_SCANCODE_COUNT; j++) {
            // Erase all flags except for pressed flag
            keyboardStates[i][j] &= KEY_STATE_FLAG_PRESSED;
        }
    }
    for (int i = 0; i < MAX_KEYBOARDS; i++) {
        if (!realKeyboards[i].IsValid()) {
            continue;
        }
        realKeyboards[i].NewFrame();
    }
}


static bool IsKeyboardRegistered(SDL_KeyboardID keyboardID)
{
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboards[i] == keyboardID) {
            return true;
        }
    }
    return false;
}


// Returns true if the keyboard id is registered as part of a real keyboard.
static bool IsRawKeyboardRegistered(SDL_KeyboardID keyboardID)
{
    for (int i = 0; i < MAX_KEYBOARDS; i++) {
        if (!realKeyboards[i].IsValid()) {
            continue;
        }
        if (realKeyboards[i].rawKeyboardIDs.find(keyboardID) 
                != realKeyboards[i].rawKeyboardIDs.end()) {
            // Found the keyboard ID.
            return true;
        }
    }
    return false;
}


static void EventListenNewKeyboard(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP) {
        return;
    }
    
    if (IsRawKeyboardRegistered(event->key.which)) {
        return;
    }
    
    static SDL_KeyboardID lastKeyboardPressedReturn = 0;
    static SDL_KeyboardID lastKeyboardPressedUp = 0;


    if (event->type == SDL_EVENT_KEY_DOWN) {
        // TODO: if two keyboards press the same button, cancel it by setting
        // last keyboard pressed to 0
        if (event->key.scancode == SDL_SCANCODE_RETURN) {
            lastKeyboardPressedReturn = event->key.which;
            //SDL_Log("%d pressed return", event->key.which);
        }
        else if (event->key.scancode == SDL_SCANCODE_UP) {
            lastKeyboardPressedUp = event->key.which;
            //SDL_Log("%d pressed up", event->key.which);
        }
    }
    else if (event->type == SDL_EVENT_KEY_UP) {
        if (event->key.scancode == SDL_SCANCODE_RETURN) {
            lastKeyboardPressedReturn = 0;
        }
        else if (event->key.scancode == SDL_SCANCODE_UP) {
            lastKeyboardPressedUp = 0;
        }
    }

    if (lastKeyboardPressedUp && lastKeyboardPressedReturn) {
        // Register the keyboard with these ids
        SDL_Log("Register");
        const SDL_KeyboardID rawIDs[] = {lastKeyboardPressedReturn,
                                         lastKeyboardPressedUp};
        RegisterRealKeyboard(rawIDs, 2);
        lastKeyboardPressedUp = 0;
        lastKeyboardPressedReturn = 0;
    }

}


static void DeregisterKeyboard(SDL_KeyboardID kID)
{
    for (int i = 0; i < MAX_KEYBOARDS; i++) {
        if (!realKeyboards[i].IsValid()) {
            continue;
        }
        if (realKeyboards[i].ContainsRawKeyboard(kID)) {
            realKeyboards[i].Deinit();
            numRealKeyboards--;
            return;
        }
    }
}


void Input::HandleEvent(SDL_Event *event)
{
    for (int i = 0; i < MAX_KEYBOARDS; i++) {
        if (!realKeyboards[i].IsValid()) {
            continue;
        }
        realKeyboards[i].HandleEvent(event);
    }
    if (event->type == SDL_EVENT_KEYBOARD_ADDED) {
        SDL_Log("keyboard added. id %d at time %lu",
                event->kdevice.which, event->kdevice.timestamp);
    }
    else if (event->type == SDL_EVENT_KEYBOARD_REMOVED) {
        DeregisterKeyboard(event->kdevice.which);
    }
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
        //RegisterKeyboard(event->key.which);
        //SDL_Log("return pressed %d", event->key.which);
    }
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        EventListenNewKeyboard(event);
        scancodesDown[event->key.scancode] = event->key.down;
        for (int i = 0; i < numKeyboards; i++) {
            if (keyboards[i] == event->key.which) {
                keyboardStates[i][event->key.scancode] &= ~KEY_STATE_FLAG_PRESSED;
                if (event->key.down) {
                    keyboardStates[i][event->key.scancode] |= 
                        KEY_STATE_FLAG_PRESSED;
                }
                if (event->type == SDL_EVENT_KEY_DOWN) {
                    keyboardStates[i][event->key.scancode] |= 
                        KEY_STATE_FLAG_JUST_PRESSED;
                }
                if (event->type == SDL_EVENT_KEY_UP) {
                    keyboardStates[i][event->key.scancode] |= 
                        KEY_STATE_FLAG_JUST_RELEASED;
                }
                break;
            }
        }
        //SDL_Log("keyboard id %d", event->key.which);
    }
    else if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_JoystickID joyId = event->gdevice.which;
        gamepads[numOpenedGamepads] = SDL_OpenGamepad(joyId);
        if (gamepads[numOpenedGamepads] == NULL) {
            SDL_Log("Could not open gamepad with Joystick id %d: %s", 
                    joyId, SDL_GetError());
        }
        else {
            numOpenedGamepads++;
        }
    }
    else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        SDL_JoystickID joyId = event->gdevice.which;
        for (int i = 0; i < MAX_GAMEPADS; i++) {
            if (SDL_GetGamepadID(gamepads[i]) == joyId) {
                // Move last gamepad to location of deleted gamepad to keep them
                // all at the start of the array
                SDL_CloseGamepad(gamepads[i]);
                gamepads[i] = gamepads[numOpenedGamepads];
                gamepads[numOpenedGamepads] = NULL; // Not necessary
                numOpenedGamepads--;
                break;
            }
        }
    }
}


float Input::GetScanAxis(SDL_Scancode negScan, SDL_Scancode posScan)
{
    bool isDownNeg = Input::IsScanDown(negScan);
    bool isDownPos = Input::IsScanDown(posScan);
    return (float)isDownPos - (float)isDownNeg;
}


void Input::Update()
{
    /*
    for (int i = 0; i < numKeyboards; i++) {
        if (keyboardStates[i][SDL_SCANCODE_SPACE] & KEY_STATE_FLAG_JUST_PRESSED) {
            SDL_Log("space pressed: id = %d", keyboards[i]);
        }
        if (keyboardStates[i][SDL_SCANCODE_SPACE] & KEY_STATE_FLAG_PRESSED) {
            SDL_Log("space being pressed: id = %d", keyboards[i]);
        }
    }
    */
    //SDL_Log("%d", keyboardStates[0][SDL_SCANCODE_SPACE]);

    DebugGUI();
}


float Input::GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
    Sint16 amount = SDL_GetGamepadAxis(gamepad, axis);
    // Normalise the result to between -1.0f and 1.0f. Note that
    // SDL_JOYSTICK_AXIS_MAX is closer to 0 than SDL_JOYSTICK_AXIS_MIN,
    // so doing it this way may result in normAmount being able to go 
    // slightly below -1.0f.
    float normAmount = (float) amount / SDL_JOYSTICK_AXIS_MAX;
    normAmount = normAmount < -1.0f ? -1.0f : normAmount;
    if (SDL_fabsf(normAmount) < DEADZONE) {
        normAmount = 0.0;
    }
    return normAmount;
}
float Input::GetGamepadAxisPos(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
    return SDL_max(GetGamepadAxis(gamepad, axis), 0);
}
float Input::GetGamepadAxisNeg(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
    return -SDL_min(GetGamepadAxis(gamepad, axis), 0);
}


int Input::GetNumRealKeyboards()
{
    return numRealKeyboards;
}


SDL_Gamepad* Input::GetGamepad()
{
    return gamepads[0];
}


bool Input::GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button)
{
    return SDL_GetGamepadButton(gamepad, button);
}


void Input::DebugGUI()
{
    ImGui::Begin("Input", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    for (int i = 0; i < numOpenedGamepads; i++) {
        ImGui::Text("%s %d, player idx %d", SDL_GetGamepadName(gamepads[i]),
                    i, SDL_GetGamepadPlayerIndex(gamepads[i]));
    }

    char comboTitle[32];
    for (int i = 0; i < gNumPlayers; i++) {
        SDL_snprintf(comboTitle, 32, "Player %i Input Device", i);

        char comboValueString[32];
        if (gPlayers[i].inputDeviceType == INPUT_DEVICE_GAMEPAD) {
            const char *gamepadName = SDL_GetGamepadName(gPlayers[i].gamepad);
            SDL_snprintf(comboValueString, 32, "%s %d",
                         gamepadName, SDL_GetGamepadID(gPlayers[i].gamepad));
        }
        else if (gPlayers[i].inputDeviceType == INPUT_DEVICE_KEYBOARD) {
            /*
            SDL_snprintf(comboValueString, 32, "%s %d",
                         SDL_GetKeyboardNameForID(gPlayers[i].keyboardID),
                         gPlayers[i].keyboardID);
             */
            RealKeyboard *keyboardUsed = RealKeyboard::GetByID(
                    gPlayers[i].realKeyboardID);

            const char* keyboardName = keyboardUsed != nullptr ?
                keyboardUsed->GetName() : "Any keyboard";

            SDL_snprintf(
                    comboValueString, 32, "%s %d",
                    keyboardName,
                    gPlayers[i].realKeyboardID
            );
        }
        else {
            SDL_snprintf(comboValueString, 32, "None");
        }
        
        if (!ImGui::BeginCombo(comboTitle, comboValueString)) {
            continue;
        }

        const bool isNoneSelected =
            gPlayers[i].inputDeviceType == INPUT_DEVICE_NONE;

        if (ImGui::Selectable("None", isNoneSelected)) {
            gPlayers[i].inputDeviceType = INPUT_DEVICE_NONE;
        }
        // Options for gamepads
        for (int j = 0; j < numOpenedGamepads; j++) {
            SDL_snprintf(comboValueString, 32, "%s %d",
                         SDL_GetGamepadName(gamepads[j]),
                         SDL_GetGamepadID(gamepads[j]));
            const bool isSelected = gPlayers[i].IsUsingGamepad(gamepads[j]);
            if (ImGui::Selectable(comboValueString, isSelected)) {
                gPlayers[i].UseGamepad(gamepads[j]);
            }
        }
        // Any keyboard option (selects keyboard with ID 0, which defaults to
        // using SDL's internal keyboard state)
        SDL_snprintf(comboValueString, 32, "Any keyboard");
        const bool isSelected = gPlayers[i].IsUsingKeyboard(0);
        if (ImGui::Selectable(comboValueString, isSelected)) {
            gPlayers[i].UseKeyboard(0);
        }
        // Options for keyboards
        for (int j = 0; j < MAX_KEYBOARDS; j++) {
            if (!realKeyboards[j].IsValid()) continue;
            SDL_snprintf(comboValueString, 32, "%s %d",
                    realKeyboards[j].GetName(),
                    realKeyboards[j].GetID()
            );
            int realKeyboardID = realKeyboards[j].GetID();
            const bool isSelected = gPlayers[i].IsUsingKeyboard(realKeyboardID);
            if (ImGui::Selectable(comboValueString, isSelected)) {
                gPlayers[i].UseKeyboard(realKeyboardID);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}

