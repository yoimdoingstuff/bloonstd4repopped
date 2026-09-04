#include "SDLRenderer.hpp"
#include "../../engine/core/Logger.hpp"
#include <algorithm>
#include <cmath>

namespace btd4 {

// Built-in 5x7 minimalist ASCII font (characters 32..126)
// Each character is 5 columns of 7 bits
static const uint8_t s_font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // '#'
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '\''
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // ')'
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // '0'
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // '@'
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 'A'
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 'D'
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 'G'
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 'H'
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 'J'
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 'M'
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 'N'
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 'O'
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 'Q'
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 'T'
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 'U'
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 'V'
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7f, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7f, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7f, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7f}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7e, 0x09, 0x01, 0x02}, // 'f'
    {0x0c, 0x52, 0x52, 0x52, 0x3e}, // 'g'
    {0x7f, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7d, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3d, 0x00}, // 'j'
    {0x7f, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7f, 0x40, 0x00}, // 'l'
    {0x7c, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7c, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7c, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7c}, // 'q'
    {0x7c, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3f, 0x44, 0x40, 0x20}, // 't'
    {0x3c, 0x40, 0x40, 0x20, 0x7c}, // 'u'
    {0x1c, 0x20, 0x40, 0x20, 0x1c}, // 'v'
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0c, 0x50, 0x50, 0x50, 0x3c}, // 'y'
    {0x44, 0x64, 0x54, 0x4c, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7f, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x08, 0x08, 0x2a, 0x1c, 0x08}  // '~'
};

SDLRenderer::SDLRenderer() = default;

SDLRenderer::~SDLRenderer() {
    shutdown();
}

bool SDLRenderer::initializeWithWindow(SDL_Window* window) {
    if (!window) {
        BTD4_LOG_ERROR("Cannot initialize SDL renderer without a window.");
        return false;
    }
    if (m_renderer) {
        if (m_window == window) {
            return true;
        }
        BTD4_LOG_ERROR("SDL renderer is already bound to a different window.");
        return false;
    }
    if (m_window && m_window != window) {
        BTD4_LOG_ERROR("SDL renderer has an unreleased window from a previous initialization attempt.");
        return false;
    }

    m_window = window;

    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        // Fallback to software renderer
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!m_renderer) {
        BTD4_LOG_ERROR(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        if (!m_ownsWindow) {
            m_window = nullptr;
        }
        return false;
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    return true;
}

bool SDLRenderer::initialize(int windowWidth, int windowHeight) {
    if (m_renderer) {
        return true;
    }

    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
            BTD4_LOG_ERROR(std::string("SDL video initialization failed: ") + SDL_GetError());
            return false;
        }
        m_ownsVideoSubsystem = true;
    }

    if (m_window) {
        return initializeWithWindow(m_window);
    }

    m_window = SDL_CreateWindow(
        "Bloons TD 4 Repopped - Engine Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!m_window) {
        BTD4_LOG_ERROR(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        if (m_ownsVideoSubsystem) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            m_ownsVideoSubsystem = false;
        }
        return false;
    }
    m_ownsWindow = true;
    if (!initializeWithWindow(m_window)) {
        shutdown();
        return false;
    }
    return true;
}

void SDLRenderer::shutdown() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window && m_ownsWindow) {
        SDL_DestroyWindow(m_window);
    }
    m_window = nullptr;
    m_ownsWindow = false;
    if (m_ownsVideoSubsystem) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        m_ownsVideoSubsystem = false;
    }
}

void SDLRenderer::beginFrame() {
    if (!m_renderer) {
        return;
    }
    // Reset viewport to full window to clear letterbox/pillarbox margins
    SDL_RenderSetViewport(m_renderer, nullptr);
}

void SDLRenderer::endFrame() {
    if (!m_renderer) {
        return;
    }
    SDL_RenderPresent(m_renderer);
}

void SDLRenderer::setViewport(const Viewport& viewport) {
    if (!m_renderer) {
        return;
    }
    m_currentViewport = viewport;
    SDL_Rect r{viewport.x, viewport.y, viewport.width, viewport.height};
    SDL_RenderSetViewport(m_renderer, &r);

    // Set logical coordinate size to 480x272 so drawings scale automatically
    SDL_RenderSetLogicalSize(m_renderer, 480, 272);
}

void SDLRenderer::clear(const Color& color) {
    if (!m_renderer) {
        return;
    }
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(m_renderer);
}

void SDLRenderer::drawRect(float x, float y, float w, float h, const Color& color, bool filled) {
    if (!m_renderer) {
        return;
    }
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect r{
        static_cast<int>(std::round(x)),
        static_cast<int>(std::round(y)),
        static_cast<int>(std::round(w)),
        static_cast<int>(std::round(h))
    };
    if (filled) {
        SDL_RenderFillRect(m_renderer, &r);
    } else {
        SDL_RenderDrawRect(m_renderer, &r);
    }
}

void SDLRenderer::drawLine(float x1, float y1, float x2, float y2, const Color& color) {
    if (!m_renderer) {
        return;
    }
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(m_renderer,
        static_cast<int>(std::round(x1)),
        static_cast<int>(std::round(y1)),
        static_cast<int>(std::round(x2)),
        static_cast<int>(std::round(y2))
    );
}

void SDLRenderer::drawText(const std::string& text, float x, float y, float scale, const Color& color) {
    if (!m_renderer) {
        return;
    }
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    int curX = static_cast<int>(std::round(x));
    int curY = static_cast<int>(std::round(y));
    int s = std::max(1, static_cast<int>(scale));

    for (char c : text) {
        if (c < 32 || c > 126) {
            c = '?';
        }
        int idx = c - 32;
        const uint8_t* glyph = s_font5x7[idx];

        for (int col = 0; col < 5; ++col) {
            uint8_t line = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (line & (1 << row)) {
                    SDL_Rect pixel{curX + col * s, curY + row * s, s, s};
                    SDL_RenderFillRect(m_renderer, &pixel);
                }
            }
        }
        curX += (5 + 1) * s; // 5 columns + 1 spacing
    }
}

void SDLRenderer::onResize(int newWidth, int newHeight) {
    // Handled via setViewport
    (void)newWidth;
    (void)newHeight;
}

} // namespace btd4
