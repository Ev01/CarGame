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



/*
enum MenuItemType {
    MI_BUTTON,
    MI_CHOICE
};
*/

enum DialogType {
    DIALOG_PLAYER_ADD,
    DIALOG_RACE_END
};

// Forward declarations
struct ChoiceOption;

struct Rect {
    float x = 0.0;
    float y = 0.0;
    float w = 0.0;
    float h = 0.0;
};

namespace UI {
    // Forward declarations
    //struct Menu;
    //struct MenuItem;

    struct Dialog {
        const char* line1;
        const char* line2;
        DialogType type;
        void HandleEvent(SDL_Event *event);
        void GetLine1(char *outText, int maxLen);
        void GetLine2(char *outText, int maxLen);
    };

    glm::vec2 GetPositionAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            Rect bound);
    Rect GetRectAnchored(glm::vec2 size, glm::vec2 margin, UIAnchor anchor, 
            Rect bound);
    Dialog* GetCurrentDialog();
    void OpenDialog(Dialog *toOpen);
    void OpenEndRaceDialog();
    void OpenAddPlayerDialog();
    void CloseCurrentDialog();
    void HandleEvent(SDL_Event *event);
    void Update();

}
