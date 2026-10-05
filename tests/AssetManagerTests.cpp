#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include "Assets/AssetManager.h"

TEST(AssetManagerLoadsTextureFromDisk)
{
    SoftwareRenderTarget target(4, 4);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "red", WriteSolidImage("red4", 4, 4, RED));
    SDL_Texture *texture = assets.GetTexture("red");
    REQUIRE(texture != nullptr);
    CHECK_EQ(TextureWidth(texture), 4);
}

TEST(AssetManagerLoadsTheGamesPngAssets)
{
    SoftwareRenderTarget target(4, 4);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "tank", "./assets/images/tank-big-right.png");
    assets.AddTexture(target.Renderer(), "chopper", "./assets/images/chopper-spritesheet.png");
    CHECK(assets.GetTexture("tank") != nullptr);
    CHECK_EQ(TextureWidth(assets.GetTexture("chopper")), 64);
}

TEST(AssetManagerReturnsNullForUnknownId)
{
    AssetManager assets;
    CHECK(assets.GetTexture("missing") == nullptr);
}

TEST(AssetManagerDoesNotRegisterAFileThatFailsToLoad)
{
    SoftwareRenderTarget target(4, 4);
    REQUIRE(target.IsValid());
    AssetManager assets;
    std::cerr << "  (an image load error is expected below)" << std::endl;
    assets.AddTexture(target.Renderer(), "broken", "./does/not/exist.png");
    CHECK(assets.GetTexture("broken") == nullptr);
}

TEST(AssetManagerReplacesTextureWhenIdIsReused)
{
    SoftwareRenderTarget target(4, 4);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "sprite", WriteSolidImage("small", 4, 4, RED));
    assets.AddTexture(target.Renderer(), "sprite", WriteSolidImage("large", 8, 8, BLUE));
    CHECK_EQ(TextureWidth(assets.GetTexture("sprite")), 8);
}

TEST(AssetManagerClearAssetsRemovesEverything)
{
    SoftwareRenderTarget target(4, 4);
    REQUIRE(target.IsValid());
    AssetManager assets;
    assets.AddTexture(target.Renderer(), "a", WriteSolidImage("a", 4, 4, RED));
    assets.AddTexture(target.Renderer(), "b", WriteSolidImage("b", 4, 4, BLUE));
    assets.ClearAssets();
    CHECK(assets.GetTexture("a") == nullptr);
    CHECK(assets.GetTexture("b") == nullptr);
}
