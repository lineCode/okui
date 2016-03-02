#include "onair/okui/views/Image.h"

namespace onair {
namespace okui {
namespace views {

void Image::setTextureResource(std::string resource) {
    _texture = loadTextureResource(resource);
    _resource = std::move(resource);
    _fromURL = false;
}

void Image::setTextureFromURL(std::string url) {
    _texture = loadTextureResource(url);
    _resource = std::move(url);
    _fromURL = true;
}

void Image::setTextureColor(Color color) {
    _color = color;
    invalidateRenderCache();
}

void Image::setTextureDistanceField(double edge) {
    _distanceFieldEdge = edge;
    invalidateRenderCache();
}

void Image::render() {
    if (_texture.isLoaded()) {
        auto shader = textureShader();

        if (_distanceFieldEdge) {
            auto dfShader = distanceFieldShader();
            dfShader->setEdge(*_distanceFieldEdge);
            shader = dfShader;
        }

        shader->setColor(_color);
        shader->drawScaledFit(*_texture, 0, 0, bounds().width, bounds().height);
        shader->flush();
    }
}

void Image::windowChanged() {
    if (!_resource.empty()) {
        _texture = _fromURL ? loadTextureFromURL(_resource) : loadTextureResource(_resource);
    }
}

}}}