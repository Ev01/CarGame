/*
 * This file contains functions that are only used in other render files. It
 * should only be included in other render files.
 */
#pragma once

#include "font.h"
#include "shader.h"
#include "texture.h"
#include "model.h"
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

extern unsigned int fbo;
extern unsigned int rbo;
extern unsigned int textureColourBuffer;
extern unsigned int msFBO;
extern unsigned int msRBO;
extern unsigned int msTexColourBuffer;
extern unsigned int textVAO, textVBO;
extern unsigned int quadVAO, quadVBO;
extern int screenWidth;
extern int screenHeight;
extern Model *cubeModel;
extern Model *quadModel;
extern Material windowMat;
extern Texture tachoMeterTex;
extern Texture tachoNeedleTex;
extern Character characters[96];
extern glm::mat4 uiProj;

extern ShaderProg simpleDepthShader;
extern ShaderProg pbrShader;
extern ShaderProg screenShader;

extern bool doSplitScreen;
extern SDL_Window *window;


enum UIAnchor {
    UI_ANCHOR_BOTTOM_LEFT  = 0b00,
    UI_ANCHOR_TOP_LEFT     = 0b01,
    UI_ANCHOR_BOTTOM_RIGHT = 0b10,
    UI_ANCHOR_TOP_RIGHT    = 0b11,
};

namespace Render {
    void CreateFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO, 
                              unsigned int aWidth, unsigned int aHeight);
    void CreateMSFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO,
                                    unsigned int aWidth, unsigned int aHeight);
    void CreateFBOs();
    void RenderSkybox(glm::mat4 view, glm::mat4 projection);
    void DrawMap(ShaderProg &shader);
    void DrawCars(ShaderProg &shader);
    void DrawCheckpoints(ShaderProg &shader);
    void DebugGUI();
    bool LoadFont();
    void LoadShaders();
    void InitSkybox();
    void CreateQuadVAO();
    void UpdatePlayerCamAspectRatios();
    void RenderUIAnchored(Texture tex, glm::vec2 scale, glm::vec2 margin,
                                 float rotation, UIAnchor anchor,
                                 float boundX=0.0, float boundY=0.0,
                                 float boundW=0.0, float boundH=0.0);
    void GetPlayerSplitScreenBounds(int playerNum, int *outX, int *outY,
                                           int *outW, int *outH);
    void RenderPlayerTachometer(int playerNum);
    void GuiPass();
    void RenderSceneSplitScreen();
    void UpdateWindowSize();
    void RenderShadowDepthToScreen();
    void ToggleFullscreen();
}
