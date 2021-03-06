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
#include <okui/shaders/ColorShader.h>

namespace okui::shaders {

ColorShader::ColorShader() {
    opengl::Shader vsh(scraps::opengl::CommonVertexShaderHeader() + R"(
        ATTRIBUTE_IN vec2 positionAttrib;
        ATTRIBUTE_IN vec4 colorAttrib;
        ATTRIBUTE_IN vec4 curveAttrib;

        VARYING_OUT vec4 color;
        VARYING_OUT vec4 curve;

        void main() {
            color = colorAttrib;
            curve = curveAttrib;
            gl_Position = vec4(positionAttrib, 0.0, 1.0);
        }
    )", opengl::Shader::kVertexShader);

    opengl::Shader fsh(CommonOKUIFragmentShaderHeader() + R"(
        VARYING_IN vec4 color;
        VARYING_IN vec4 curve;

        void main() {
            vec4 c = color;
            float alphaMultiplier = 1.0;
            if (curve.z > 1.5) {
                if (abs(curve.s) >= 0.5 || abs(curve.t) >= 0.5) {
                    discard;
                }
                float dist = sqrt(curve.s * curve.s + curve.t * curve.t);
                float aa = curve.w;
                if (dist > 0.5 + 0.5 * aa) {
                    discard;
                } else if (dist > 0.5 - 0.5 * aa) {
                    alphaMultiplier = 1.0 - (dist - (0.5 - 0.5 * aa)) / aa;
                }
            } else if (curve.z != 0.0) {
                float dist = pow(curve.s, 2.0) - curve.t;
                float aa = curve.w;
                dist -= curve.z * aa;
                if (dist < 0.0 != curve.z < 0.0) {
                    float x = abs(dist) / (2.0 * aa);
                    if (x < 1.0) {
                        alphaMultiplier = (1.0 - x);
                    } else {
                        discard;
                    }
                }
            }

            COLOR_OUT = multipliedOutput(vec4(c.rgb, color.a * alphaMultiplier));
        }
    )", opengl::Shader::kFragmentShader);

    enum : GLuint {
        kPositionAttrib,
        kColorAttrib,
        kCurveAttrib,
    };

    _program.attachShaders(vsh, fsh);
    _program.bindAttribute(kPositionAttrib, "positionAttrib");
    _program.bindAttribute(kColorAttrib, "colorAttrib");
    _program.bindAttribute(kCurveAttrib, "curveAttrib");
    _program.link();
    _program.use();

    _blendingFlagsUniform = _program.uniform("blendingFlags");

    if (!_program.error().empty()) {
        SCRAPS_LOGF_ERROR("error creating shader: %s", _program.error().c_str());
        return;
    }

    auto stride = reinterpret_cast<char*>(&_vertices[1]) - reinterpret_cast<char*>(&_vertices[0]);
    _vertexArrayBuffer.setAttribute(kPositionAttrib, 2, GL_FLOAT, GL_FALSE, stride, offsetof(Vertex, x));
    _vertexArrayBuffer.setAttribute(kColorAttrib, 4, GL_FLOAT, GL_FALSE, stride, offsetof(Vertex, r));
    _vertexArrayBuffer.setAttribute(kCurveAttrib, 4, GL_FLOAT, GL_FALSE, stride, offsetof(Vertex, cu));

    SCRAPS_GL_ERROR_CHECK();
}

void ColorShader::setColor(const Color& color) {
    _triangle.a.r = _triangle.b.r = _triangle.c.r = color.redF();
    _triangle.a.g = _triangle.b.g = _triangle.c.g = color.greenF();
    _triangle.a.b = _triangle.b.b = _triangle.c.b = color.blueF();
    _triangle.a.a = _triangle.b.a = _triangle.c.a = color.alphaF();
    _gradient = false;
}

void ColorShader::setColorA(double x, double y, const Color& color) {
    _transformation.transform(x, y, &_gradientPointA.x, &_gradientPointA.y);
    _gradientPointA.r = color.redF();
    _gradientPointA.g = color.greenF();
    _gradientPointA.b = color.blueF();
    _gradientPointA.a = color.alphaF();
    _gradient = true;
}

void ColorShader::setColorB(double x, double y, const Color& color) {
    _transformation.transform(x, y, &_gradientPointB.x, &_gradientPointB.y);
    _gradientPointB.r = color.redF();
    _gradientPointB.g = color.greenF();
    _gradientPointB.b = color.blueF();
    _gradientPointB.a = color.alphaF();
    _gradient = true;
}

void ColorShader::_processTriangle(const std::array<Point<double>, 3>& p, const std::array<Point<double>, 3>& pT, Shader::Curve curve) {
    TriangleCurveProcessor::Process(_triangle, p, curve);

    if (_gradient) {
        _calculateGradientColor(pT[0].x, pT[0].y, &_triangle.a.r, &_triangle.a.g, &_triangle.a.b, &_triangle.a.a);
        _calculateGradientColor(pT[1].x, pT[1].y, &_triangle.b.r, &_triangle.b.g, &_triangle.b.b, &_triangle.b.a);
        _calculateGradientColor(pT[2].x, pT[2].y, &_triangle.c.r, &_triangle.c.g, &_triangle.c.b, &_triangle.c.a);
    }
}

void ColorShader::_calculateGradientColor(double x, double y, GLfloat* r, GLfloat* g, GLfloat* b, GLfloat* a) {
    auto bWeight = std::min(std::max(_calculateGradientPosition(x, y), 0.0), 1.0);
    auto aWeight = 1.0 - bWeight;

    *r = _gradientPointA.r * aWeight + _gradientPointB.r * bWeight;
    *g = _gradientPointA.g * aWeight + _gradientPointB.g * bWeight;
    *b = _gradientPointA.b * aWeight + _gradientPointB.b * bWeight;
    *a = _gradientPointA.a * aWeight + _gradientPointB.a * bWeight;
}

double ColorShader::_calculateGradientPosition(double x, double y) {
    // if the points are on the same x or y coordinate, things are easy

    if (_gradientPointA.y == _gradientPointB.y) {
        return (x - _gradientPointA.x) / (_gradientPointB.x - _gradientPointA.x);
    } else if (_gradientPointA.x == _gradientPointB.x) {
        return (y - _gradientPointA.y) / (_gradientPointB.y - _gradientPointA.y);
    }

    // otherwise we need to calculate the x coordinate of the intersection of the pointA-pointB line
    // and the perpendicular that passes through (x, y)

    auto gradientSlope = (_gradientPointB.y - _gradientPointA.y) / (_gradientPointB.x - _gradientPointA.x);
    auto perpendicularSlope = -1.0 / gradientSlope;

    auto positionX = (_gradientPointA.y - y - gradientSlope * _gradientPointA.x + perpendicularSlope * x) / (perpendicularSlope - gradientSlope);

    return (positionX - _gradientPointA.x) / (_gradientPointB.x - _gradientPointA.x);
}

void ColorShader::flush() {
    _program.use();
    ShaderBase<Vertex>::_flush(false);
}

} // namespace okui::shaders
