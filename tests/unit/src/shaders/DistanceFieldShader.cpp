/**
* Copyright 2017 BitTorrent Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "../RenderOnce.h"
#include "../TestFramebuffer.h"

#include <gtest/gtest.h>

#if ONAIR_OKUI_HAS_NATIVE_APPLICATION && !OPENGL_ES // TODO: fix for OpenGL ES

using namespace okui;


TEST(DistanceFieldShader, edgeZero) {
    std::shared_ptr<TextureInterface> texture;

    RenderOnce([&](View* view) {
        texture = view->loadTextureResource("PlayIcon.png");
        EXPECT_NE(texture, nullptr);
        EXPECT_FALSE(texture->isLoaded());
    },
    [&](View* view) { // edge 0
        EXPECT_TRUE(texture->isLoaded());

        TestFramebuffer framebuffer(320, 200);

        auto shader = view->distanceFieldShader();
        shader->setTransformation(framebuffer.transformation());
        shader->setEdge(0);

        shader->drawScaledFill(*texture, 0, 0, 128, 128);

        shader->flush();

        framebuffer.finish();

        // top left
        auto pixel = framebuffer.getPixel(19, 19);
        EXPECT_EQ(pixel, Color::kBlack);
        pixel = framebuffer.getPixel(20, 20);
        EXPECT_GT(pixel.redF(), 0.5);
        EXPECT_LT(pixel.redF(), 1);
        pixel = framebuffer.getPixel(21, 21);
        EXPECT_EQ(pixel, Color::kWhite);
        pixel = framebuffer.getPixel(28, 28);
        EXPECT_EQ(pixel, Color::kWhite);

        // bottom right
        pixel = framebuffer.getPixel(69, 89);
        EXPECT_EQ(pixel, Color::kWhite);
        pixel = framebuffer.getPixel(70, 90);
        EXPECT_GT(pixel.redF(), 0);
        pixel = framebuffer.getPixel(72, 92);
        EXPECT_EQ(pixel, Color::kBlack);
    });
}

TEST(DistanceFieldShader, edgeHalf) {
    std::shared_ptr<TextureInterface> texture;

    RenderOnce([&](View* view) {
        texture = view->loadTextureResource("PlayIcon.png");
        EXPECT_NE(texture, nullptr);
        EXPECT_FALSE(texture->isLoaded());
    },
    [&](View* view) { // edge 0.5
        EXPECT_TRUE(texture->isLoaded());

        TestFramebuffer framebuffer(320, 200);

        auto shader = view->distanceFieldShader();
        shader->setTransformation(framebuffer.transformation());
        shader->setEdge(0.5);

        shader->drawScaledFill(*texture, 0, 0, 128, 128);

        shader->flush();

        framebuffer.finish();

        // top left
        auto pixel = framebuffer.getPixel(26, 26);
        EXPECT_EQ(pixel, Color::kBlack);
        pixel = framebuffer.getPixel(28, 28);
        EXPECT_GT(pixel.redF(), 0.5);
        EXPECT_LT(pixel.redF(), 1);
        pixel = framebuffer.getPixel(29, 29);
        EXPECT_EQ(pixel, Color::kWhite);

        // bottom right
        pixel = framebuffer.getPixel(61, 81);
        EXPECT_EQ(pixel, Color::kWhite);
        pixel = framebuffer.getPixel(63, 83);
        EXPECT_GT(pixel.redF(), 0);
        EXPECT_LT(pixel.redF(), 1);
        pixel = framebuffer.getPixel(64, 84);
        EXPECT_EQ(pixel, Color::kBlack);
    });
}

#endif // ONAIR_OKUI_HAS_NATIVE_APPLICATION
