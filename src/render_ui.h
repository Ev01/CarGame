#pragma once

#include <glm/glm.hpp>
#include <string>

// Forward declarations
struct Texture;
struct ShaderProg;
namespace Font {
    struct Face;
}

enum UIAnchor : unsigned int;

namespace Render {
    void DrawRect(float x, float y, float w, float h, glm::vec4 colour);
    void RenderText(Font::Face *face, ShaderProg &s, std::string text, float x, float y,
                    float scale, glm::vec3 colour);
    void RenderUIAnchored(Texture tex, glm::vec2 scale, glm::vec2 margin,
                                 float rotation, UIAnchor anchor,
                                 float boundX=0.0, float boundY=0.0,
                                 float boundW=0.0, float boundH=0.0);
    void RenderPlayerTachometer(int playerNum);
    void GuiPass();
}
