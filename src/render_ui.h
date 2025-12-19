#pragma once

#include <glm/glm.hpp>
#include <string>

// Forward declarations
struct Texture;
struct ShaderProg;
struct Rect;
namespace Font {
    struct Face;
}

enum UIAnchor : unsigned int;

namespace Render {
    void DrawRect(float x, float y, float w, float h, glm::vec4 colour);
    void RenderText(Font::Face *face, std::string text, float x, float y,
                    float scale, glm::vec3 colour);
    void RenderTextAnchored(Font::Face *face, const char* text, 
                            glm::vec2 margin, float scale, UIAnchor anchor, 
                            glm::vec3 colour, Rect bound);
    void RenderUIAnchored(Texture tex, glm::vec2 scale, glm::vec2 margin,
                          float rotation, UIAnchor anchor, Rect bound);
    void RenderPlayerTachometer(int playerNum);
    void GuiPass();
}
