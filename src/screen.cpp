#include "screen.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <framework/opengl_includes.h>
#include <iostream>
#include <string>

Screen::Screen(const glm::ivec2& resolution, bool presentable)
    : m_presentable(presentable)
    , m_resolution(resolution)
    , m_textureData(size_t(resolution.x * resolution.y), glm::vec3(0.0f))
{
    // Create OpenGL texture if we want to present the screen.
    if (m_presentable) {
        // Generate texture
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Screen::applyBloomFilter(const float threshold, const int boxSize)
{
    // screen containing only pixels where the max of colour values is greater than x
    // Screen filteredScreen(m_resolution, m_presentable);

    glm::vec3 table[m_resolution[0]][m_resolution[1]];

    // iterator for the original vector
    int i = 0;

    // filter the screen for the pixels above the threshold
    for (size_t width = 0; width < m_resolution[0]; width++) {
        for (size_t height = 0; height < m_resolution[1]; height++) {
            auto maxColour = std::max(m_textureData[i][0], std::max(m_textureData[i][1], m_textureData[i][2]));
            if (maxColour > threshold) {
                table[width][height] = m_textureData[i];
            } else {
                table[width][height] = { 0.0f, 0.0f, 0.0f };
            }
            i++;
        }
    }

    // box filter of the image above threshold
    i = 0;
    int half = boxSize / 2;
    for (size_t width = 0; width < m_resolution[0]; width++) {
        for (size_t height = 0; height < m_resolution[1]; height++) {

            if (width >= half && width < m_resolution[0] - half && height >= half && height < m_resolution[1] - half) {
                glm::vec3 sum = { 0.0f, 0.0f, 0.0f };

                for (size_t a = width - half; a < width + half; a++) {
                    for (size_t b = height - half; b < height + half; b++) {
                        sum += table[a][b];
                    }
                }
                table[width][height] = (sum / float(boxSize * boxSize));
            }
            m_textureData[i] += table[width][height];
            // filteredScreen.m_textureData[i] = table[width][height];
            i++;
        }
    }

    // STUFF FOR DEBUG
    // for (size_t i = 0; i < m_textureData.size(); i++) {
    //     auto maxColour = std::max(m_textureData[i][0], std::max(m_textureData[i][1], m_textureData[i][2]));
    //     if (maxColour > threshold) {
    //         filteredScreen.m_textureData[i] = m_textureData[i];
    //     }
    // }
    // m_textureData = filteredScreen.m_textureData;
}

void Screen::clear(const glm::vec3& color)
{
    std::fill(std::begin(m_textureData), std::end(m_textureData), color);
}

void Screen::setPixel(int x, int y, const glm::vec3& color)
{
    // In the window/camera class we use (0, 0) at the bottom left corner of the screen (as used by GLFW).
    // OpenGL / stbi like the origin / (-1,-1) to be at the TOP left corner so transform the y coordinate.
    const int i = (m_resolution.y - 1 - y) * m_resolution.x + x;
    m_textureData[i] = glm::vec4(color, 1.0f);
}

void Screen::writeBitmapToFile(const std::filesystem::path& filePath)
{
    std::vector<glm::u8vec4> textureData8Bits(m_textureData.size());
    std::transform(std::begin(m_textureData), std::end(m_textureData), std::begin(textureData8Bits),
        [](const glm::vec3& color) {
            const glm::vec3 clampedColor = glm::clamp(color, 0.0f, 1.0f);
            return glm::u8vec4(glm::vec4(clampedColor, 1.0f) * 255.0f);
        });

    std::string filePathString = filePath.string();
    stbi_write_bmp(filePathString.c_str(), m_resolution.x, m_resolution.y, 4, textureData8Bits.data());
}

void Screen::draw()
{
    if (m_presentable) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_resolution.x, m_resolution.y, 0, GL_RGB, GL_FLOAT, m_textureData.data());

        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_NORMALIZE);
        glColor3f(1.0f, 1.0f, 1.0f);

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(+1.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(+1.0f, +1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-1.0f, +1.0f, 0.0f);
        glEnd();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glPopAttrib();
    } else {
        std::cerr << "Screen::draw() called on non-presentable screen" << std::endl;
    }
}

glm::ivec2 Screen::resolution() const
{
    return m_resolution;
}

const std::vector<glm::vec3>& Screen::pixels() const
{
    return m_textureData;
}

std::vector<glm::vec3>& Screen::pixels()
{
    return m_textureData;
}

int Screen::indexAt(int x, int y) const
{
    return (m_resolution.y - 1 - y) * m_resolution.x + x;
}
