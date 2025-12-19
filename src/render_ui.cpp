#include "render_ui.h"
#include "render_internal.h"
#include "render.h"
#include "main_game.h"
#include "player.h"
#include "vehicle.h"
#include "world.h"
#include "texture.h"
#include "ui.h"
#include "font.h"
#include "shader.h"
#include "glerr.h"

#include "../glad/glad.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


static void DrawMenu()
{
    const float scale = 1.0f;
    UI::Menu* menu = UI::GetCurrentMenu();
    if (menu == nullptr) return;

    for (int i = 0; i < menu->numItems; i++) {
        bool isSelected = menu->selectedIdx == i;
        glm::vec3 col = isSelected ? glm::vec3(0.0, 0.0, 0.0) : glm::vec3(0.3, 0.3, 0.3);
        float yOffset = -Font::defaultFace->GetLineHeight() * scale * i;
        char text[64];
        menu->items[i].GetText(text, 64);
        float width = Font::defaultFace->GetWidthOfText(text, SDL_strlen(text)) * scale;
        glm::vec2 pos = UI::GetPositionAnchored(
                glm::vec2(width, 0.0), glm::vec2(0.0, yOffset),
                UI_ANCHOR_CENTRE, 0, 0, screenWidth, screenHeight);
        Render::RenderText(Font::defaultFace, text, pos.x, pos.y,
                           scale, col);
    }
}


void Render::DrawRect(float x, float y, float w, float h, glm::vec4 colour)
{
    /*
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glScissor(x, y, w, h);
    glClearColor(colour.x, colour.y, colour.z, colour.w);
    glClear(GL_COLOR_BUFFER_BIT);

    //glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    */
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(x, y, 0.0));
    trans = glm::scale(trans, glm::vec3(w, h, 0.0));

    glUseProgram(uiShader.id);
    glBindVertexArray(uiQuadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gDefaultTexture.id);
    uiShader.SetVec4((char*)"colourMod", glm::value_ptr(colour));
    uiShader.SetMat4fv((char*)"trans", glm::value_ptr(trans));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    const glm::vec4 white = glm::vec4(1, 1, 1, 1);
    uiShader.SetVec4((char*)"colourMod", glm::value_ptr(white));
}


void Render::RenderText(Font::Face *face, std::string text, float x, float y,
                        float scale, glm::vec3 colour)
{
    glUseProgram(textShader.id);
    textShader.SetVec3((char*)"textColour", colour.x, colour.y, colour.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = face->characters[*c - 32];

        float xPos = x + ch.bearing.x * scale;
        float yPos = y - (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // Update VBO for each character
        float vertices[6][4] = {
            { xPos,     yPos + h, 0.0f, 0.0f },
            { xPos,     yPos,     0.0f, 1.0f },
            { xPos + w, yPos,     1.0f, 1.0f },

            { xPos,     yPos + h, 0.0f, 0.0f },
            { xPos + w, yPos,     1.0f, 1.0f },
            { xPos + w, yPos + h, 1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.texture.id);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Advance cursors for next glyph (advance is in 1/64 pixels)
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


static void DrawAddPlayerDialog()
{
    const float scale = 0.5;
    const char* text = "Press Up + Enter on keyboard or A on controller..."; 
    const char* text2 = "Press Escape to cancel.";
    float textWidth = Font::defaultFace->GetWidthOfText(text, SDL_strlen(text)) * scale;
    float textWidth2 = Font::defaultFace->GetWidthOfText(text2, SDL_strlen(text2)) * scale;
    float textHeight = Font::defaultFace->GetLineHeight() * scale;
    const float vertSpacing = 10.0f;
    glm::vec2 textSize = glm::vec2(SDL_max(textWidth, textWidth2), textHeight * 2) + vertSpacing;
    glm::vec2 rectMargin = glm::vec2(24, 24);

    glm::vec4 rect = UI::GetRectAnchored(textSize + rectMargin, glm::vec2(0, 0),
            UI_ANCHOR_CENTRE, 0, 0, screenWidth, screenHeight);
    glm::vec4 rectInner = UI::GetRectAnchored(textSize, glm::vec2(0, 0),
            UI_ANCHOR_CENTRE, 0, 0, screenWidth, screenHeight);
    const glm::vec4 col = glm::vec4(0.9, 0.8, 0.8, 0.8);
    Render::DrawRect(rect.x, rect.y, rect.z, rect.w, col);
    
    glm::vec2 textPos = UI::GetPositionAnchored(glm::vec2(textWidth, textHeight),
            glm::vec2(0, 0), UI_ANCHOR_TOP, rectInner.x, rectInner.y, rectInner.z, rectInner.w);
    Render::RenderText(Font::defaultFace, text, textPos.x, textPos.y,
            scale, glm::vec3(0.0, 0.0, 0.0));

    glm::vec2 textPos2 = UI::GetPositionAnchored(glm::vec2(textWidth2, textHeight),
            glm::vec2(0, 0), UI_ANCHOR_BOTTOM, rectInner.x, rectInner.y, rectInner.z, rectInner.w);
    Render::RenderText(Font::defaultFace, text2, textPos2.x, textPos2.y,
            scale, glm::vec3(0.0, 0.0, 0.0));
}


void Render::RenderUIAnchored(Texture tex, glm::vec2 scale, glm::vec2 margin,
                             float rotation, UIAnchor anchor,
                             float boundX, float boundY,
                             float boundW, float boundH)
{
    glm::mat4 trans = glm::mat4(1.0);
    glm::vec2 pos;
    //float boundW  = (float)screenWidth;
    //float boundH = (float)screenHeight;
    if (boundW == 0.0) boundW = (float)screenWidth;
    if (boundH == 0.0) boundH = (float)screenHeight;
    
    // Divide scale because the quad is originally 2 wide and 2 high.
    //scale = scale / 2.0f;
    glm::vec2 rotPivot = scale / 2.0f;

    pos = UI::GetPositionAnchored(scale, margin, anchor, boundX, boundY, boundW, boundH);
    trans = glm::translate(trans, glm::vec3(pos.x, pos.y, 0.0f));
    // Translate back after rotating so the pivot point stays in the same
    // place.
    trans = glm::translate(trans, glm::vec3(rotPivot.x, rotPivot.y, 0));
    trans = glm::rotate(trans, rotation, glm::vec3(0, 0, 1));
    // Translate before rotating so it rotates around pivot.
    trans = glm::translate(trans, glm::vec3(-rotPivot.x, -rotPivot.y, 0));
    trans = glm::scale(trans, glm::vec3(scale.x, scale.y, 1.0f));

    glViewport(0, 0, screenWidth, screenHeight);

    // Draw the texture
    glUseProgram(uiShader.id);
    GLERR;
    uiShader.SetMat4fv((char*)"trans", glm::value_ptr(trans));
    uiShader.SetMat4fv((char*)"proj", glm::value_ptr(uiProj));
    glBindVertexArray(uiQuadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void Render::RenderPlayerTachometer(int playerNum)
{
    int boundX, boundY, boundW, boundH;
    GetPlayerSplitScreenBounds(playerNum, &boundX, &boundY, &boundW, &boundH);
    RenderUIAnchored(tachoMeterTex, glm::vec2(256.0f, 256.0f),
                     glm::vec2(-32.0f, 32.0f), 0.0, UI_ANCHOR_BOTTOM_RIGHT,
                     boundX, boundY, boundW, boundH);

    // Draw the needle
    Player &p = gPlayers[playerNum];
    const glm::vec2 margin = glm::vec2(-32.0f, 32.0f);
    const glm::vec2 scale = glm::vec2(256.0f, 256.0f);
    if (p.vehicle != NULL) {
        float rpm = p.vehicle->GetEngineRPM();
        // Change these constants based on the tachometer graphic.
        const float zeroAngle = 0.74;
        const float angleRange = 4.28;
        const float rpmRange = 8000.0;
        float needleAngle = zeroAngle - rpm / rpmRange * angleRange;
        RenderUIAnchored(tachoNeedleTex, scale, margin, needleAngle, 
                UI_ANCHOR_BOTTOM_RIGHT, boundX, boundY, boundW, boundH);

        //int vehKph = (int) (p.vehicle->GetLongVelocity() * 3.6);
        int vehKph = SDL_abs (p.vehicle->GetSpeedoSpeed() * 3.6);
        //char vehKphStr[16];
        //SDL_itoa(vehKph, vehKphStr, 10);
        // TODO: Make this easier to position
        Render::RenderText(Font::defaultFace, std::to_string(vehKph), 
                screenWidth + margin.x - scale.x / 2.0 - 10.0f,
                margin.y + scale.y / 2.0 - 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    }

}


void Render::GuiPass()
{
    if (MainGame::gGameState == GAME_PRESS_START_SCREEN) {

    }
    // Render the current menu
    DrawMenu();
    if (UI::GetShowPlayerAddDialog()) {
        DrawAddPlayerDialog();
    }

    // Render the tachometer for all players
    if (MainGame::gGameState == GAME_IN_WORLD) {
        for (int i = 0; i < gNumPlayers; i++) {
            RenderPlayerTachometer(i);
            // Only render player 1 if doSplitScreen is off.
            if (!doSplitScreen) break;
        }
    }

    // Draw race start countdown
    if (World::GetRaceState() == RACE_COUNTING_DOWN) {
        int secondsLeft = (int) World::GetRaceProgress().mCountdownTimer + 1;
        char text[4];
        SDL_itoa(secondsLeft, text, 10);
        float textWidth = Font::defaultFace->GetWidthOfText(text, SDL_strlen(text));
        float textHeight = Font::defaultFace->GetLineHeight();
        glm::vec2 pos = UI::GetPositionAnchored(glm::vec2(textWidth, textHeight), glm::vec2(0, 0),
                UI_ANCHOR_CENTRE, 0, 0, screenWidth, screenHeight);
        RenderText(Font::defaultFace, text, pos.x, pos.y, 1.0, glm::vec3(0, 0, 0));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
