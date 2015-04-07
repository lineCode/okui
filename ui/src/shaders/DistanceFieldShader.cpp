#include "bittorrent/ui/shaders/DistanceFieldShader.h"

namespace bittorrent {
namespace ui {
namespace shaders {

DistanceFieldShader::DistanceFieldShader() {
    opengl::Shader vsh(
    #if OPENGL_ES
        "precision highp float;"
    #endif
    R"(
        attribute vec2 positionAttrib;
        attribute vec4 colorAttrib;
        attribute vec2 textureCoordAttrib;
        
        varying vec4 color;
        varying vec2 textureCoord;
        
        void main() {
            color = colorAttrib;
            textureCoord = textureCoordAttrib;
            gl_Position = vec4(positionAttrib, 0.0, 1.0);
        }
    )", opengl::Shader::kVertexShader);

    opengl::Shader fsh(
#if OPENGL_ES
        "precision highp float;"
#endif
    R"(
        varying vec4 color;
        varying vec2 textureCoord;
        
        uniform sampler2D texture;
        
        void main() {
            vec4 sample = texture2D(texture, textureCoord);
            float min = 0.43;
            float max = 0.49;
            if (sample.r > max) {
                discard;
            }
            gl_FragColor = vec4(color.rgb, color.a * (1.0 - max((sample.r - min) / (max - min), 0.0)));
        }
    )", opengl::Shader::kFragmentShader);
    
    _program.attachShaders(vsh, fsh);
    
    _program.bindAttribute(kVertexPositionAttribute, "positionAttrib");
    _program.bindAttribute(kVertexColorAttribute, "colorAttrib");
    _program.bindAttribute(kVertexTextureCoordinateAttribute, "textureCoordAttrib");

    _program.link();
    _program.use();

    _program.uniform("texture") = 0;
    
    if (!_program.error().empty()) {
        BT_LOG_ERROR("error creating shader: %s", _program.error().c_str());
        return;
    }
}

void DistanceFieldShader::setColor(double r, double g, double b, double a) {
    _triangle.a.r = _triangle.b.r = _triangle.c.r = r;
    _triangle.a.g = _triangle.b.g = _triangle.c.g = g;
    _triangle.a.b = _triangle.b.b = _triangle.c.b = b;
    _triangle.a.a = _triangle.b.a = _triangle.c.a = a;
}

void DistanceFieldShader::setTexture(GLuint id, double x, double y, double w, double h) {
    if (_texture != id) {
        flush();
    }
    
    _texture = id;
    
    _transformation.transform(x, y, &_textureX1, &_textureY1);
    
    double x2, y2;
    _transformation.transform(x + w, y + h, &x2, &y2);
    
    _textureWidth  = x2 - _textureX1;
    _textureHeight = y2 - _textureY1;
}

void DistanceFieldShader::drawTriangle(double x1, double y1, double x2, double y2, double x3, double y3, Shader::Bezier bezier) {    
	assert(bezier == kBezierNone);
	
    _transformation.transform(x1, y1, &x1, &y1);
    _transformation.transform(x2, y2, &x2, &y2);
    _transformation.transform(x3, y3, &x3, &y3);

    _triangle.a.x  = x1;
    _triangle.a.y  = y1;
    _triangle.a.s  = (x1 - _textureX1) / _textureWidth;
    _triangle.a.t  = (y1 - _textureY1) / _textureHeight;

    _triangle.b.x  = x2;
    _triangle.b.y  = y2;
    _triangle.b.s  = (x2 - _textureX1) / _textureWidth;
    _triangle.b.t  = (y2 - _textureY1) / _textureHeight;

    _triangle.c.x  = x3;
    _triangle.c.y  = y3;
    _triangle.c.s  = (x3 - _textureX1) / _textureWidth;
    _triangle.c.t  = (y3 - _textureY1) / _textureHeight;

    _vertices.push_back(_triangle.a);
    _vertices.push_back(_triangle.b);
    _vertices.push_back(_triangle.c);
}

void DistanceFieldShader::flush() {
    if (_vertices.empty()) { return; }
    
    _program.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    glEnableVertexAttribArray(kVertexPositionAttribute);
    glEnableVertexAttribArray(kVertexColorAttribute);
    glEnableVertexAttribArray(kVertexTextureCoordinateAttribute);

    auto stride = reinterpret_cast<char*>(&_vertices[1]) - reinterpret_cast<char*>(&_vertices[0]);
    glVertexAttribPointer(kVertexPositionAttribute, 2, GL_FLOAT, GL_FALSE, stride, &_vertices[0].x);
    glVertexAttribPointer(kVertexColorAttribute, 4, GL_FLOAT, GL_FALSE, stride, &_vertices[0].r);
    glVertexAttribPointer(kVertexTextureCoordinateAttribute, 2, GL_FLOAT, GL_FALSE, stride, &_vertices[0].s);

    glDrawArrays(GL_TRIANGLES, 0, _vertices.size());
    
    glDisableVertexAttribArray(kVertexTextureCoordinateAttribute);
    glDisableVertexAttribArray(kVertexColorAttribute);
    glDisableVertexAttribArray(kVertexPositionAttribute);
    
    _vertices.clear();
}

}}}