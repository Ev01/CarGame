#include "font.h"

#include "../glad/glad.h"
#include "glerr.h"

FT_Library ft;
FT_Face face;
Font::Face *Font::defaultFace;
//Character characters[96];

bool Font::Init()
{
    if (FT_Init_FreeType(&ft)) {
        SDL_Log("Could not init FreeType Library");
        return false;
    }

    Font::defaultFace = CreateFaceFromFile("data/fonts/liberation_sans/LiberationSans-Regular.ttf");

    return true;
}


float Font::Face::GetWidthOfText(const char* text, int textLength)
{
    float width = 0.0;
    for (int i = 0; i < textLength; i++) {
        char c = text[i];
        width += (characters[c - 32].advance >> 6);
    }
    return width;
}

Font::Face* Font::CreateFaceFromFile(const char* filename)
{
    Font::Face* newFace = new Font::Face();
    FT_Face ftFace;
    if (FT_New_Face(ft, filename, 0, &ftFace)) {
        SDL_Log("Failed to load font");
    }

    FT_Set_Pixel_Sizes(ftFace, 0, 48);
    if (FT_Load_Char(ftFace, 'X', FT_LOAD_RENDER)) {
        SDL_Log("Failed to load glyph");
    }
    newFace->ftFace = ftFace;

    GLERR;
    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Loop through first 128 ascii characters (0-31 are control characters)
    for (unsigned char c = 32; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(ftFace, c, FT_LOAD_RENDER)) {
            SDL_Log("Failed to load glyph");
        }
        Texture tex;
        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            ftFace->glyph->bitmap.width,
            ftFace->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ftFace->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLERR;
        // Store character for later
        Character character = {
            tex,
            glm::ivec2(ftFace->glyph->bitmap.width, ftFace->glyph->bitmap.rows),
            glm::ivec2(ftFace->glyph->bitmap_left,  ftFace->glyph->bitmap_top),
            static_cast<unsigned int>(ftFace->glyph->advance.x)
        };
        newFace->characters[c - 32] = character;
    }

    return newFace;
}


float Font::Face::GetLineHeight()
{
    return ftFace->size->metrics.height >> 6;
}


void Font::CleanUp()
{
    FT_Done_FreeType(ft);
    FT_Done_Face(face);
    delete Font::defaultFace;
}


