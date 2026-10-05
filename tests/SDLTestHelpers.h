#ifndef SDLTESTHELPERS_H
#define SDLTESTHELPERS_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <cstdlib>
#include <filesystem>
#include <ostream>
#include <string>

struct Rgb
{
    int r, g, b;
    bool operator==(const Rgb &other) const { return r == other.r && g == other.g && b == other.b; }
};

inline std::ostream &operator<<(std::ostream &out, const Rgb &c)
{
    return out << "rgb(" << c.r << "," << c.g << "," << c.b << ")";
}

const Rgb BLACK{0, 0, 0};
const Rgb RED{255, 0, 0};
const Rgb BLUE{0, 0, 255};

// Draws into an in-memory surface, so render tests need no window or display.
class SoftwareRenderTarget
{
public:
    SoftwareRenderTarget(int width, int height)
    {
        surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
        renderer = surface ? SDL_CreateSoftwareRenderer(surface) : nullptr;
        if (renderer)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
        }
    }
    ~SoftwareRenderTarget()
    {
        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
        }
        if (surface)
        {
            SDL_FreeSurface(surface);
        }
    }
    SoftwareRenderTarget(const SoftwareRenderTarget &) = delete;
    SoftwareRenderTarget &operator=(const SoftwareRenderTarget &) = delete;

    bool IsValid() const { return renderer != nullptr; }
    SDL_Renderer *Renderer() const { return renderer; }

    // SDL batches draw calls; they only reach the surface on present.
    void Present() { SDL_RenderPresent(renderer); }

    Rgb PixelAt(int x, int y) const
    {
        const Uint32 *pixels = static_cast<const Uint32 *>(surface->pixels);
        Uint32 pixel = pixels[y * (surface->pitch / 4) + x];
        Uint8 r, g, b;
        SDL_GetRGB(pixel, surface->format, &r, &g, &b);
        return Rgb{r, g, b};
    }

private:
    SDL_Surface *surface = nullptr;
    SDL_Renderer *renderer = nullptr;
};

// Writes an image of `width` x `height` whose left `splitX` columns are `left`
// and the rest `right`; pass splitX == width for a single solid colour.
inline std::string WriteTestImage(const std::string &name, int width, int height, Rgb left, Rgb right, int splitX)
{
    SDL_Surface *image = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    SDL_Rect leftRect{0, 0, splitX, height};
    SDL_Rect rightRect{splitX, 0, width - splitX, height};
    SDL_FillRect(image, &leftRect, SDL_MapRGBA(image->format, left.r, left.g, left.b, 255));
    SDL_FillRect(image, &rightRect, SDL_MapRGBA(image->format, right.r, right.g, right.b, 255));
    std::string path = (std::filesystem::temp_directory_path() / ("g2d-test-" + name + ".bmp")).string();
    SDL_SaveBMP(image, path.c_str());
    SDL_FreeSurface(image);
    return path;
}

inline std::string WriteSolidImage(const std::string &name, int width, int height, Rgb color)
{
    return WriteTestImage(name, width, height, color, color, width);
}

// Uses SDL's dummy driver: channels play and report status without a sound card.
struct DummyAudio
{
    bool isOpen = false;
    DummyAudio()
    {
        setenv("SDL_AUDIODRIVER", "dummy", 1);
        isOpen = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0;
    }
    ~DummyAudio()
    {
        if (isOpen)
        {
            Mix_HaltChannel(-1);
            Mix_CloseAudio();
        }
    }
    DummyAudio(const DummyAudio &) = delete;
    DummyAudio &operator=(const DummyAudio &) = delete;
};

inline int TextureWidth(SDL_Texture *texture)
{
    int width = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, nullptr);
    return width;
}
#endif
