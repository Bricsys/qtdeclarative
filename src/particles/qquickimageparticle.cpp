// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickitem_p.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>
#include <QFile>
#include <QRandomGenerator>
#include "qquickimageparticle_p.h"
#include "qquickparticleemitter_p.h"
#include <private/qquicksprite_p.h>
#include <private/qquickspriteengine_p.h>
#include <QSGRendererInterface>
#include <QtQuick/private/qsgplaintexture_p.h>
#include <private/qqmlglobal_p.h>
#include <QtQml/qqmlinfo.h>
#include <QtCore/QtMath>
#include <rhi/qrhi.h>

#include <cmath>

QT_BEGIN_NAMESPACE

// Must match the shader code
#define UNIFORM_ARRAY_SIZE 64

class ImageMaterialData
{
    public:
    ImageMaterialData()
        : texture(nullptr), colorTable(nullptr)
    {}

    ~ImageMaterialData(){
        delete texture;
        delete colorTable;
    }

    QSGTexture *texture;
    QSGTexture *colorTable;
    float sizeTable[UNIFORM_ARRAY_SIZE];
    float opacityTable[UNIFORM_ARRAY_SIZE];

    qreal dpr;
    qreal timestamp;
    qreal entry;
    QSizeF animSheetSize;
};

class TabledMaterialRhiShader : public QSGMaterialShader
{
public:
    TabledMaterialRhiShader(int viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_tabled.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_tabled.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &renderState, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        QByteArray *buf = renderState.uniformData();
        Q_ASSERT(buf->size() >= 80 + 2 * (UNIFORM_ARRAY_SIZE * 4 * 4));
        const int shaderMatrixCount = newMaterial->viewCount();
        const int matrixCount = qMin(renderState.projectionMatrixCount(), shaderMatrixCount);

        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            if (renderState.isMatrixDirty()) {
                const QMatrix4x4 m = renderState.combinedMatrix(viewIndex);
                memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            }
        }

        if (renderState.isOpacityDirty()) {
            const float opacity = renderState.opacity();
            memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        }

        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();

        float entry = float(state->entry);
        memcpy(buf->data() + 64 * shaderMatrixCount + 4, &entry, 4);

        float timestamp = float(state->timestamp);
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &timestamp, 4);

        float *p = reinterpret_cast<float *>(buf->data() + 64 * shaderMatrixCount + 16);
        for (int i = 0; i < UNIFORM_ARRAY_SIZE; ++i) {
            *p = state->sizeTable[i];
            p += 4;
        }
        p = reinterpret_cast<float *>(buf->data() + 64 * shaderMatrixCount + 16 + (UNIFORM_ARRAY_SIZE * 4 * 4));
        for (int i = 0; i < UNIFORM_ARRAY_SIZE; ++i) {
            *p = state->opacityTable[i];
            p += 4;
        }

        return true;
    }

    void updateSampledImage(RenderState &renderState, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();
        if (binding == 2) {
            state->colorTable->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->colorTable;
        } else if (binding == 1) {
            state->texture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->texture;
        }
    }
};

class TabledMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new TabledMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType TabledMaterial::m_type;

class DeformableMaterialRhiShader : public QSGMaterialShader
{
public:
    DeformableMaterialRhiShader(int viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_deformed.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_deformed.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &renderState, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        QByteArray *buf = renderState.uniformData();
        Q_ASSERT(buf->size() >= 80 + 2 * (UNIFORM_ARRAY_SIZE * 4 * 4));
        const int shaderMatrixCount = newMaterial->viewCount();
        const int matrixCount = qMin(renderState.projectionMatrixCount(), shaderMatrixCount);

        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            if (renderState.isMatrixDirty()) {
                const QMatrix4x4 m = renderState.combinedMatrix(viewIndex);
                memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            }
        }

        if (renderState.isOpacityDirty()) {
            const float opacity = renderState.opacity();
            memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        }

        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();

        float entry = float(state->entry);
        memcpy(buf->data() + 64 * shaderMatrixCount + 4, &entry, 4);

        float timestamp = float(state->timestamp);
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &timestamp, 4);

        return true;
    }

    void updateSampledImage(RenderState &renderState, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();
        if (binding == 1) {
            state->texture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->texture;
        }
    }
};

class DeformableMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new DeformableMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType DeformableMaterial::m_type;

class ParticleSpriteMaterialRhiShader : public QSGMaterialShader
{
public:
    ParticleSpriteMaterialRhiShader(int viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_sprite.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_sprite.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &renderState, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        QByteArray *buf = renderState.uniformData();
        Q_ASSERT(buf->size() >= 80 + 2 * (UNIFORM_ARRAY_SIZE * 4 * 4));
        const int shaderMatrixCount = newMaterial->viewCount();
        const int matrixCount = qMin(renderState.projectionMatrixCount(), shaderMatrixCount);

        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            if (renderState.isMatrixDirty()) {
                const QMatrix4x4 m = renderState.combinedMatrix(viewIndex);
                memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            }
        }

        if (renderState.isOpacityDirty()) {
            const float opacity = renderState.opacity();
            memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        }

        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();

        float entry = float(state->entry);
        memcpy(buf->data() + 64 * shaderMatrixCount + 4, &entry, 4);

        float timestamp = float(state->timestamp);
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &timestamp, 4);

        float *p = reinterpret_cast<float *>(buf->data() + 64 * shaderMatrixCount + 16);
        for (int i = 0; i < UNIFORM_ARRAY_SIZE; ++i) {
            *p = state->sizeTable[i];
            p += 4;
        }
        p = reinterpret_cast<float *>(buf->data() + 64 * shaderMatrixCount + 16 + (UNIFORM_ARRAY_SIZE * 4 * 4));
        for (int i = 0; i < UNIFORM_ARRAY_SIZE; ++i) {
            *p = state->opacityTable[i];
            p += 4;
        }

        return true;
    }

    void updateSampledImage(RenderState &renderState, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();
        if (binding == 2) {
            state->colorTable->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->colorTable;
        } else if (binding == 1) {
            state->texture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->texture;
        }
    }
};

class SpriteMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new ParticleSpriteMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType SpriteMaterial::m_type;

class ColoredPointMaterialRhiShader : public QSGMaterialShader
{
public:
    ColoredPointMaterialRhiShader(int viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_coloredpoint.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_coloredpoint.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &renderState, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        QByteArray *buf = renderState.uniformData();
        Q_ASSERT(buf->size() >= 80 + 2 * (UNIFORM_ARRAY_SIZE * 4 * 4));
        const int shaderMatrixCount = newMaterial->viewCount();
        const int matrixCount = qMin(renderState.projectionMatrixCount(), shaderMatrixCount);

        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            if (renderState.isMatrixDirty()) {
                const QMatrix4x4 m = renderState.combinedMatrix(viewIndex);
                memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            }
        }

        if (renderState.isOpacityDirty()) {
            const float opacity = renderState.opacity();
            memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        }

        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();

        float entry = float(state->entry);
        memcpy(buf->data() + 64 * shaderMatrixCount + 4, &entry, 4);

        float timestamp = float(state->timestamp);
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &timestamp, 4);

        float dpr = float(state->dpr);
        memcpy(buf->data() + 64 * shaderMatrixCount + 12, &dpr, 4);

        return true;
    }

    void updateSampledImage(RenderState &renderState, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();
        if (binding == 1) {
            state->texture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->texture;
        }
    }
};

class ColoredPointMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new ColoredPointMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType ColoredPointMaterial::m_type;

class ColoredMaterialRhiShader : public ColoredPointMaterialRhiShader
{
public:
    ColoredMaterialRhiShader(int viewCount)
        : ColoredPointMaterialRhiShader(viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_colored.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_colored.frag.qsb"), viewCount);
    }
};

class ColoredMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new ColoredMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType ColoredMaterial::m_type;

class SimplePointMaterialRhiShader : public QSGMaterialShader
{
public:
    SimplePointMaterialRhiShader(int viewCount)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/particles/shaders_ng/imageparticle_simplepoint.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, QStringLiteral(":/particles/shaders_ng/imageparticle_simplepoint.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &renderState, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        QByteArray *buf = renderState.uniformData();
        Q_ASSERT(buf->size() >= 80 + 2 * (UNIFORM_ARRAY_SIZE * 4 * 4));
        const int shaderMatrixCount = newMaterial->viewCount();
        const int matrixCount = qMin(renderState.projectionMatrixCount(), shaderMatrixCount);

        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            if (renderState.isMatrixDirty()) {
                const QMatrix4x4 m = renderState.combinedMatrix(viewIndex);
                memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            }
        }

        if (renderState.isOpacityDirty()) {
            const float opacity = renderState.opacity();
            memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        }

        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();

        float entry = float(state->entry);
        memcpy(buf->data() + 64 * shaderMatrixCount + 4, &entry, 4);

        float timestamp = float(state->timestamp);
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &timestamp, 4);

        float dpr = float(state->dpr);
        memcpy(buf->data() + 64 * shaderMatrixCount + 12, &dpr, 4);

        return true;
    }

    void updateSampledImage(RenderState &renderState, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        ImageMaterialData *state = static_cast<ImageMaterial *>(newMaterial)->state();
        if (binding == 1) {
            state->texture->commitTextureOperations(renderState.rhi(), renderState.resourceUpdateBatch());
            *texture = state->texture;
        }
    }
};

class SimplePointMaterial : public ImageMaterial
{
public:
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override {
        Q_UNUSED(renderMode);
        return new SimplePointMaterialRhiShader(viewCount());
    }
    QSGMaterialType *type() const override { return &m_type; }

    ImageMaterialData *state() override { return &m_state; }

private:
    static QSGMaterialType m_type;
    ImageMaterialData m_state;
};

QSGMaterialType SimplePointMaterial::m_type;

void fillUniformArrayFromImage(float* array, const QImage& img, int size)
{
    if (img.isNull()){
        for (int i=0; i<size; i++)
            array[i] = 1.0;
        return;
    }
    QImage scaled = img.scaled(size,1);
    for (int i=0; i<size; i++)
        array[i] = qAlpha(scaled.pixel(i,0))/255.0;
}

/*!
    \qmltype ImageParticle
    \nativetype QQuickImageParticle
    \inqmlmodule QtQuick.Particles
    \inherits ParticlePainter
    \brief For visualizing logical particles using an image.
    \ingroup qtquick-particles

    This element renders a logical particle as an image. The image can be
    \list
    \li colorized
    \li rotated
    \li deformed
    \li a sprite-based animation
    \endlist

    ImageParticles implictly share data on particles if multiple ImageParticles are painting
    the same logical particle group. This is broken down along the four capabilities listed
    above. So if one ImageParticle defines data for rendering the particles in one of those
    capabilities, and the other does not, then both will draw the particles the same in that
    aspect automatically. This is primarily useful when there is some random variation on
    the particle which is supposed to stay with it when switching painters. If both ImageParticles
    define how they should appear for that aspect, they diverge and each appears as it is defined.

    This sharing of data happens behind the scenes based off of whether properties were implicitly or explicitly
    set. One drawback of the current implementation is that it is only possible to reset the capabilities as a whole.
    So if you explicitly set an attribute affecting color, such as redVariation, and then reset it (by setting redVariation
    to undefined), all color data will be reset and it will begin to have an implicit value of any shared color from
    other ImageParticles.

    \note The maximum number of image particles is limited to 16383.
*/
/*!
    \qmlproperty url QtQuick.Particles::ImageParticle::source

    The source image to be used.

    If the image is a sprite animation, use the sprite property instead.

    Since Qt 5.2, some default images are provided as resources to aid prototyping:
    \table
    \row
    \li qrc:///particleresources/star.png
    \li \inlineimage particles/star.png
    \row
    \li qrc:///particleresources/glowdot.png
    \li \inlineimage particles/glowdot.png
    \row
    \li qrc:///particleresources/fuzzydot.png
    \li \inlineimage particles/fuzzydot.png
    \endtable

    Note that the images are white and semi-transparent, to allow colorization
    and alpha levels to have maximum effect.
*/
/*!
    \qmlproperty list<Sprite> QtQuick.Particles::ImageParticle::sprites

    The sprite or sprites used to draw this particle.

    Note that the sprite image will be scaled to a square based on the size of
    the particle being rendered.

    For full details, see the \l{Sprite Animations} overview.
*/
/*!
    \qmlproperty url QtQuick.Particles::ImageParticle::colorTable

    An image whose color will be used as a 1D texture to determine color over life. E.g. when
    the particle is halfway through its lifetime, it will have the color specified halfway
    across the image.

    This color is blended with the color property and the color of the source image.
*/
/*!
    \qmlproperty url QtQuick.Particles::ImageParticle::sizeTable

    An image whose opacity will be used as a 1D texture to determine size over life.

    This property is expected to be removed shortly, in favor of custom easing curves to determine size over life.
*/
/*!
    \qmlproperty url QtQuick.Particles::ImageParticle::opacityTable

    An image whose opacity will be used as a 1D texture to determine size over life.

    This property is expected to be removed shortly, in favor of custom easing curves to determine opacity over life.
*/
/*!
    \qmlproperty color QtQuick.Particles::ImageParticle::color

    If a color is specified, the provided image will be colorized with it.

    Default is white (no change).
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::colorVariation

    This number represents the color variation applied to individual particles.
    Setting colorVariation is the same as setting redVariation, greenVariation,
    and blueVariation to the same number.

    Each channel can vary between particle by up to colorVariation from its usual color.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::redVariation
    The variation in the red color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::greenVariation
    The variation in the green color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::blueVariation
    The variation in the blue color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::alpha
    An alpha to be applied to the image. This value is multiplied by the value in
    the image, and the value in the color property.

    Particles have additive blending, so lower alpha on single particles leads
    to stronger effects when multiple particles overlap.

    Alpha is measured from 0.0 to 1.0.

    Default is 1.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::alphaVariation
    The variation in the alpha channel between particles.

    Alpha is measured from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::rotation

    If set the image will be rotated by this many degrees before it is drawn.

    The particle coordinates are not transformed.
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::rotationVariation

    If set the rotation of individual particles will vary by up to this much
    between particles.

*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::rotationVelocity

    If set particles will rotate at this velocity in degrees/second.
*/
/*!
    \qmlproperty real QtQuick.Particles::ImageParticle::rotationVelocityVariation

    If set the rotationVelocity of individual particles will vary by up to this much
    between particles.

*/
/*!
    \qmlproperty bool QtQuick.Particles::ImageParticle::autoRotation

    If set to true then a rotation will be applied on top of the particles rotation, so
    that it faces the direction of travel. So to face away from the direction of travel,
    set autoRotation to true and rotation to 180.

    Default is false
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles::ImageParticle::xVector

    Allows you to deform the particle image when drawn. The rectangular image will
    be deformed so that the horizontal sides are in the shape of this vector instead
    of (1,0).
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles::ImageParticle::yVector

    Allows you to deform the particle image when drawn. The rectangular image will
    be deformed so that the vertical sides are in the shape of this vector instead
    of (0,1).
*/
/*!
    \qmlproperty EntryEffect QtQuick.Particles::ImageParticle::entryEffect

    This property provides basic and cheap entrance and exit effects for the particles.
    For fine-grained control, see sizeTable and opacityTable.

    Acceptable values are

    \value ImageParticle.None   Particles just appear and disappear.
    \value ImageParticle.Fade   Particles fade in from 0 opacity at the start of their life, and fade out to 0 at the end.
    \value ImageParticle.Scale  Particles scale in from 0 size at the start of their life, and scale back to 0 at the end.

    The default value is \c ImageParticle.Fade.
*/
/*!
    \qmlproperty bool QtQuick.Particles::ImageParticle::spritesInterpolate

    If set to true, sprite particles will interpolate between sprite frames each rendered frame, making
    the sprites look smoother.

    Default is true.
*/

/*!
    \qmlproperty Status QtQuick.Particles::ImageParticle::status

    The status of loading the image.
*/


QQuickImageParticle::QQuickImageParticle(QQuickItem* parent)
    : QQuickParticlePainter(parent)
    , m_color_variation(0.0)
    , m_outgoingNode(nullptr)
    , m_material(nullptr)
    , m_alphaVariation(0.0)
    , m_alpha(1.0)
    , m_redVariation(0.0)
    , m_greenVariation(0.0)
    , m_blueVariation(0.0)
    , m_rotation(0)
    , m_rotationVariation(0)
    , m_rotationVelocity(0)
    , m_rotationVelocityVariation(0)
    , m_autoRotation(false)
    , m_xVector(nullptr)
    , m_yVector(nullptr)
    , m_spriteEngine(nullptr)
    , m_spritesInterpolate(true)
    , m_explicitColor(false)
    , m_explicitRotation(false)
    , m_explicitDeformation(false)
    , m_explicitAnimation(false)
    , m_bypassOptimizations(false)
    , perfLevel(Unknown)
    , m_targetPerfLevel(Unknown)
    , m_debugMode(false)
    , m_entryEffect(Fade)
    , m_startedImageLoading(0)
    , m_rhi(nullptr)
    , m_apiChecked(false)
    , m_dpr(1.0)
    , m_previousActive(false)
{
    setFlag(ItemHasContents);
}

QQuickImageParticle::~QQuickImageParticle()
{
    clearShadows();
}

QQmlListProperty<QQuickSprite> QQuickImageParticle::sprites()
{
    return QQmlListProperty<QQuickSprite>(this, &m_sprites,
                                          spriteAppend, spriteCount, spriteAt,
                                          spriteClear, spriteReplace, spriteRemoveLast);
}

void QQuickImageParticle::sceneGraphInvalidated()
{
    m_nodes.clear();
    m_material = nullptr;
    delete m_outgoingNode;
    m_outgoingNode = nullptr;
    m_apiChecked = false;
}

void QQuickImageParticle::setImage(const QUrl &image)
{
    if (image.isEmpty()){
        if (m_image) {
            m_image.reset();
            emit imageChanged();
        }
        return;
    }

    if (!m_image)
        m_image.reset(new ImageData);
    if (image == m_image->source)
        return;
    m_image->source = image;
    emit imageChanged();
    reset();
}


void QQuickImageParticle::setColortable(const QUrl &table)
{
    if (table.isEmpty()){
        if (m_colorTable) {
            m_colorTable.reset();
            emit colortableChanged();
        }
        return;
    }

    if (!m_colorTable)
        m_colorTable.reset(new ImageData);
    if (table == m_colorTable->source)
        return;
    m_colorTable->source = table;
    emit colortableChanged();
    reset();
}

void QQuickImageParticle::setSizetable(const QUrl &table)
{
    if (table.isEmpty()){
        if (m_sizeTable) {
            m_sizeTable.reset();
            emit sizetableChanged();
        }
        return;
    }

    if (!m_sizeTable)
        m_sizeTable.reset(new ImageData);
    if (table == m_sizeTable->source)
        return;
    m_sizeTable->source = table;
    emit sizetableChanged();
    reset();
}

void QQuickImageParticle::setOpacitytable(const QUrl &table)
{
    if (table.isEmpty()){
        if (m_opacityTable) {
            m_opacityTable.reset();
            emit opacitytableChanged();
        }
        return;
    }

    if (!m_opacityTable)
        m_opacityTable.reset(new ImageData);
    if (table == m_opacityTable->source)
        return;
    m_opacityTable->source = table;
    emit opacitytableChanged();
    reset();
}

void QQuickImageParticle::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    emit colorChanged();
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setColorVariation(qreal var)
{
    if (var == m_color_variation)
        return;
    m_color_variation = var;
    emit colorVariationChanged();
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setAlphaVariation(qreal arg)
{
    if (m_alphaVariation != arg) {
        m_alphaVariation = arg;
        emit alphaVariationChanged(arg);
    }
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setAlpha(qreal arg)
{
    if (m_alpha != arg) {
        m_alpha = arg;
        emit alphaChanged(arg);
    }
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setRedVariation(qreal arg)
{
    if (m_redVariation != arg) {
        m_redVariation = arg;
        emit redVariationChanged(arg);
    }
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setGreenVariation(qreal arg)
{
    if (m_greenVariation != arg) {
        m_greenVariation = arg;
        emit greenVariationChanged(arg);
    }
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setBlueVariation(qreal arg)
{
    if (m_blueVariation != arg) {
        m_blueVariation = arg;
        emit blueVariationChanged(arg);
    }
    m_explicitColor = true;
    checkPerfLevel(ColoredPoint);
}

void QQuickImageParticle::setRotation(qreal arg)
{
    if (m_rotation != arg) {
        m_rotation = arg;
        emit rotationChanged(arg);
    }
    m_explicitRotation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setRotationVariation(qreal arg)
{
    if (m_rotationVariation != arg) {
        m_rotationVariation = arg;
        emit rotationVariationChanged(arg);
    }
    m_explicitRotation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setRotationVelocity(qreal arg)
{
    if (m_rotationVelocity != arg) {
        m_rotationVelocity = arg;
        emit rotationVelocityChanged(arg);
    }
    m_explicitRotation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setRotationVelocityVariation(qreal arg)
{
    if (m_rotationVelocityVariation != arg) {
        m_rotationVelocityVariation = arg;
        emit rotationVelocityVariationChanged(arg);
    }
    m_explicitRotation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setAutoRotation(bool arg)
{
    if (m_autoRotation != arg) {
        m_autoRotation = arg;
        emit autoRotationChanged(arg);
    }
    m_explicitRotation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setXVector(QQuickDirection* arg)
{
    if (m_xVector != arg) {
        m_xVector = arg;
        emit xVectorChanged(arg);
    }
    m_explicitDeformation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setYVector(QQuickDirection* arg)
{
    if (m_yVector != arg) {
        m_yVector = arg;
        emit yVectorChanged(arg);
    }
    m_explicitDeformation = true;
    checkPerfLevel(Deformable);
}

void QQuickImageParticle::setSpritesInterpolate(bool arg)
{
    if (m_spritesInterpolate != arg) {
        m_spritesInterpolate = arg;
        emit spritesInterpolateChanged(arg);
    }
}

void QQuickImageParticle::setBypassOptimizations(bool arg)
{
    if (m_bypassOptimizations != arg) {
        m_bypassOptimizations = arg;
        emit bypassOptimizationsChanged(arg);
    }
    // Applies regardless of perfLevel
    reset();
}

void QQuickImageParticle::setEntryEffect(EntryEffect arg)
{
    if (m_entryEffect != arg) {
        m_entryEffect = arg;
        if (m_material)
            getState(m_material)->entry = (qreal) m_entryEffect;
        emit entryEffectChanged(arg);
    }
}

void QQuickImageParticle::resetColor()
{
    m_explicitColor = false;
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* d : std::as_const(m_system->groupData[groupId]->data)) {
            if (d->colorOwner == this) {
                d->colorOwner = nullptr;
            }
        }
    }
    m_color = QColor();
    m_color_variation = 0.0f;
    m_redVariation = 0.0f;
    m_blueVariation = 0.0f;
    m_greenVariation = 0.0f;
    m_alpha = 1.0f;
    m_alphaVariation = 0.0f;
}

void QQuickImageParticle::resetRotation()
{
    m_explicitRotation = false;
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* d : std::as_const(m_system->groupData[groupId]->data)) {
            if (d->rotationOwner == this) {
                d->rotationOwner = nullptr;
            }
        }
    }
    m_rotation = 0;
    m_rotationVariation = 0;
    m_rotationVelocity = 0;
    m_rotationVelocityVariation = 0;
    m_autoRotation = false;
}

void QQuickImageParticle::resetDeformation()
{
    m_explicitDeformation = false;
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* d : std::as_const(m_system->groupData[groupId]->data)) {
            if (d->deformationOwner == this) {
                d->deformationOwner = nullptr;
            }
        }
    }
    if (m_xVector)
        delete m_xVector;
    if (m_yVector)
        delete m_yVector;
    m_xVector = nullptr;
    m_yVector = nullptr;
}

void QQuickImageParticle::reset()
{
    QQuickParticlePainter::reset();
    m_pleaseReset = true;
    update();
}


void QQuickImageParticle::invalidateSceneGraph()
{
    reset();
}

void QQuickImageParticle::createEngine()
{
    if (m_spriteEngine)
        delete m_spriteEngine;
    if (m_sprites.size()) {
        m_spriteEngine = new QQuickSpriteEngine(m_sprites, this);
        connect(m_spriteEngine, &QQuickStochasticEngine::stateChanged,
                this, &QQuickImageParticle::spriteAdvance, Qt::DirectConnection);
        m_explicitAnimation = true;
    } else {
        m_spriteEngine = nullptr;
        m_explicitAnimation = false;
    }
    reset();
}

static QSGGeometry::Attribute SimplePointParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),      // Position
    QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),            // Data
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType)             // Vectors
};

static QSGGeometry::AttributeSet SimplePointParticle_AttributeSet =
{
    3, // Attribute Count
    ( 2 + 4 + 4 ) * sizeof(float),
    SimplePointParticle_Attributes
};

static QSGGeometry::Attribute ColoredPointParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),      // Position
    QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),            // Data
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType),            // Vectors
    QSGGeometry::Attribute::create(3, 4, QSGGeometry::UnsignedByteType),     // Colors
};

static QSGGeometry::AttributeSet ColoredPointParticle_AttributeSet =
{
    4, // Attribute Count
    ( 2 + 4 + 4 ) * sizeof(float) + 4 * sizeof(uchar),
    ColoredPointParticle_Attributes
};

static QSGGeometry::Attribute ColoredParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),      // Position
    QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),            // Data
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType),            // Vectors
    QSGGeometry::Attribute::create(3, 4, QSGGeometry::UnsignedByteType),     // Colors
    QSGGeometry::Attribute::create(4, 4, QSGGeometry::UnsignedByteType),     // TexCoord
};

static QSGGeometry::AttributeSet ColoredParticle_AttributeSet =
{
    5, // Attribute Count
    ( 2 + 4 + 4 ) * sizeof(float) + (4 + 4) * sizeof(uchar),
    ColoredParticle_Attributes
};

static QSGGeometry::Attribute DeformableParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 4, QSGGeometry::FloatType),            // Position & Rotation
    QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),            // Data
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType),            // Vectors
    QSGGeometry::Attribute::create(3, 4, QSGGeometry::UnsignedByteType),     // Colors
    QSGGeometry::Attribute::create(4, 4, QSGGeometry::FloatType),            // DeformationVectors
    QSGGeometry::Attribute::create(5, 4, QSGGeometry::UnsignedByteType),     // TexCoord & autoRotate
};

static QSGGeometry::AttributeSet DeformableParticle_AttributeSet =
{
    6, // Attribute Count
    (4 + 4 + 4 + 4) * sizeof(float) + (4 + 4) * sizeof(uchar),
    DeformableParticle_Attributes
};

static QSGGeometry::Attribute SpriteParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 4, QSGGeometry::FloatType),            // Position & Rotation
    QSGGeometry::Attribute::create(1, 4, QSGGeometry::FloatType),            // Data
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType),            // Vectors
    QSGGeometry::Attribute::create(3, 4, QSGGeometry::UnsignedByteType),     // Colors
    QSGGeometry::Attribute::create(4, 4, QSGGeometry::FloatType),            // DeformationVectors
    QSGGeometry::Attribute::create(5, 4, QSGGeometry::UnsignedByteType),     // TexCoord & autoRotate
    QSGGeometry::Attribute::create(6, 3, QSGGeometry::FloatType),            // Anim Data
    QSGGeometry::Attribute::create(7, 3, QSGGeometry::FloatType)             // Anim Pos
};

static QSGGeometry::AttributeSet SpriteParticle_AttributeSet =
{
    8, // Attribute Count
    (4 + 4 + 4 + 4 + 3 + 3) * sizeof(float) + (4 + 4) * sizeof(uchar),
    SpriteParticle_Attributes
};

void QQuickImageParticle::clearShadows()
{
    foreach (const QVector<QQuickParticleData*> data, m_shadowData)
        qDeleteAll(data);
    m_shadowData.clear();
}

//Only call if you need to, may initialize the whole array first time
QQuickParticleData* QQuickImageParticle::getShadowDatum(QQuickParticleData* datum)
{
    //Will return datum if the datum is a sentinel or uninitialized, to centralize that one check
    if (datum->systemIndex == -1)
        return datum;
    if (!m_shadowData.contains(datum->groupId)) {
        QQuickParticleGroupData* gd = m_system->groupData[datum->groupId];
        QVector<QQuickParticleData*> data;
        const int gdSize = gd->size();
        data.reserve(gdSize);
        for (int i = 0; i < gdSize; i++) {
            QQuickParticleData* datum = new QQuickParticleData;
            *datum = *(gd->data[i]);
            data << datum;
        }
        m_shadowData.insert(datum->groupId, data);
    }
    //### If dynamic resize is added, remember to potentially resize the shadow data on out-of-bounds access request

    return m_shadowData[datum->groupId][datum->index];
}

void QQuickImageParticle::checkPerfLevel(PerformanceLevel level)
{
    if (m_targetPerfLevel < level) {
        m_targetPerfLevel = level;
        reset();
    }
}

bool QQuickImageParticle::loadingSomething()
{
    return (m_image && m_image->pix.isLoading())
        || (m_colorTable && m_colorTable->pix.isLoading())
        || (m_sizeTable && m_sizeTable->pix.isLoading())
        || (m_opacityTable && m_opacityTable->pix.isLoading())
        || (m_spriteEngine && m_spriteEngine->isLoading());
}

void QQuickImageParticle::mainThreadFetchImageData()
{
    const QQmlContext *context = nullptr;
    QQmlEngine *engine = nullptr;
    const auto loadPix = [&](ImageData *image) {
        if (!engine) {
            context = qmlContext(this);
            engine = context->engine();
        }
        image->pix.load(engine, context->resolvedUrl(image->source));
    };


    if (m_image) {//ImageData created on setSource
        m_image->pix.clear(this);
        loadPix(m_image.get());
    }

    if (m_spriteEngine)
        m_spriteEngine->startAssemblingImage();

    if (m_colorTable)
        loadPix(m_colorTable.get());

    if (m_sizeTable)
        loadPix(m_sizeTable.get());

    if (m_opacityTable)
        loadPix(m_opacityTable.get());

    m_startedImageLoading = 2;
}

void QQuickImageParticle::buildParticleNodes(QSGNode** passThrough)
{
    // Starts async parts, like loading images, on gui thread
    // Not on individual properties, because we delay until system is running
    if (*passThrough || loadingSomething())
        return;

    if (m_startedImageLoading == 0) {
        m_startedImageLoading = 1;
        //stage 1 is in gui thread
        QQuickImageParticle::staticMetaObject.invokeMethod(this, "mainThreadFetchImageData", Qt::QueuedConnection);
    } else if (m_startedImageLoading == 2) {
        finishBuildParticleNodes(passThrough); //rest happens in render thread
    }

    //No mutex, because it's slow and a compare that fails due to a race condition means just a dropped frame
}

void QQuickImageParticle::finishBuildParticleNodes(QSGNode** node)
{
    if (!m_rhi)
        return;

    if (m_count * 4 > 0xffff) {
        // Index data is ushort.
        qmlInfo(this) << "ImageParticle: Too many particles - maximum 16383 per ImageParticle";
        return;
    }

    if (m_count <= 0)
        return;

    m_debugMode = m_system->m_debugMode;

    if (m_sprites.size() || m_bypassOptimizations) {
        perfLevel = Sprites;
    } else if (m_colorTable || m_sizeTable || m_opacityTable) {
        perfLevel = Tabled;
    } else if (m_autoRotation || m_rotation || m_rotationVariation
               || m_rotationVelocity || m_rotationVelocityVariation
               || m_xVector || m_yVector) {
        perfLevel = Deformable;
    } else if (m_alphaVariation || m_alpha != 1.0 || m_color.isValid() || m_color_variation
               || m_redVariation || m_blueVariation || m_greenVariation) {
        perfLevel = ColoredPoint;
    } else {
        perfLevel = SimplePoint;
    }

    for (auto groupId : groupIds()) {
        //For sharing higher levels, need to have highest used so it renders
        for (QQuickParticlePainter* p : std::as_const(m_system->groupData[groupId]->painters)) {
            QQuickImageParticle* other = qobject_cast<QQuickImageParticle*>(p);
            if (other){
                if (other->perfLevel > perfLevel) {
                    if (other->perfLevel >= Tabled){//Deformable is the highest level needed for this, anything higher isn't shared (or requires your own sprite)
                        if (perfLevel < Deformable)
                            perfLevel = Deformable;
                    } else {
                        perfLevel = other->perfLevel;
                    }
                } else if (other->perfLevel < perfLevel) {
                    other->reset();
                }
            }
        }
    }

    // Points with a size other than 1 are an optional feature with QRhi
    // because some of the underlying APIs have no support for this.
    // Therefore, avoid the point sprite path with APIs like Direct3D.
    if (perfLevel < Colored && !m_rhi->isFeatureSupported(QRhi::VertexShaderPointSize))
        perfLevel = Colored;

    if (perfLevel >= ColoredPoint  && !m_color.isValid())
        m_color = QColor(Qt::white);//Hidden default, but different from unset

    m_targetPerfLevel = perfLevel;

    clearShadows();
    if (m_material)
        m_material = nullptr;

    //Setup material
    QImage colortable;
    QImage sizetable;
    QImage opacitytable;
    QImage image;
    bool imageLoaded = false;
    switch (perfLevel) {//Fallthrough intended
    case Sprites:
    {
        if (!m_spriteEngine) {
            qWarning() << "ImageParticle: No sprite engine...";
            //Sprite performance mode with static image is supported, but not advised
            //Note that in this case it always uses shadow data
        } else {
            image = m_spriteEngine->assembledImage();
            if (image.isNull())//Warning is printed in engine
                return;
            imageLoaded = true;
        }
        m_material = new SpriteMaterial;
        ImageMaterialData *state = getState(m_material);
        if (imageLoaded)
            state->texture = QSGPlainTexture::fromImage(image);
        state->animSheetSize = QSizeF(image.size() / image.devicePixelRatio());
        if (m_spriteEngine)
            m_spriteEngine->setCount(m_count);
    }
        Q_FALLTHROUGH();
    case Tabled:
    {
        if (!m_material)
            m_material = new TabledMaterial;

        if (m_colorTable) {
            if (m_colorTable->pix.isReady())
                colortable = m_colorTable->pix.image();
            else
                qmlWarning(this) << "Error loading color table: " << m_colorTable->pix.error();
        }

        if (m_sizeTable) {
            if (m_sizeTable->pix.isReady())
                sizetable = m_sizeTable->pix.image();
            else
                qmlWarning(this) << "Error loading size table: " << m_sizeTable->pix.error();
        }

        if (m_opacityTable) {
            if (m_opacityTable->pix.isReady())
                opacitytable = m_opacityTable->pix.image();
            else
                qmlWarning(this) << "Error loading opacity table: " << m_opacityTable->pix.error();
        }

        if (colortable.isNull()){//###Goes through image just for this
            colortable = QImage(1,1,QImage::Format_ARGB32_Premultiplied);
            colortable.fill(Qt::white);
        }
        ImageMaterialData *state = getState(m_material);
        state->colorTable = QSGPlainTexture::fromImage(colortable);
        fillUniformArrayFromImage(state->sizeTable, sizetable, UNIFORM_ARRAY_SIZE);
        fillUniformArrayFromImage(state->opacityTable, opacitytable, UNIFORM_ARRAY_SIZE);
    }
        Q_FALLTHROUGH();
    case Deformable:
    {
        if (!m_material)
            m_material = new DeformableMaterial;
    }
        Q_FALLTHROUGH();
    case Colored:
    {
        if (!m_material)
            m_material = new ColoredMaterial;
    }
        Q_FALLTHROUGH();
    case ColoredPoint:
    {
        if (!m_material)
            m_material = new ColoredPointMaterial;
    }
        Q_FALLTHROUGH();
    default://Also Simple
    {
        if (!m_material)
            m_material = new SimplePointMaterial;
        ImageMaterialData *state = getState(m_material);
        if (!imageLoaded) {
            if (!m_image || !m_image->pix.isReady()) {
                if (m_image)
                    qmlWarning(this) << m_image->pix.error();
                delete m_material;
                return;
            }
            //state->texture //TODO: Shouldn't this be better? But not crash?
            //    = QQuickItemPrivate::get(this)->sceneGraphContext()->textureForFactory(m_imagePix.textureFactory());
            state->texture = QSGPlainTexture::fromImage(m_image->pix.image());
        }
        state->texture->setFiltering(QSGTexture::Linear);
        state->entry = (qreal) m_entryEffect;
        state->dpr = m_dpr;

        m_material->setFlag(QSGMaterial::Blending | QSGMaterial::RequiresFullMatrix);
    }
    }

    m_nodes.clear();
    for (auto groupId : groupIds()) {
        int count = m_system->groupData[groupId]->size();
        QSGGeometryNode* node = new QSGGeometryNode();
        node->setMaterial(m_material);
        node->markDirty(QSGNode::DirtyMaterial);

        m_nodes.insert(groupId, node);
        m_idxStarts.insert(groupId, m_lastIdxStart);
        m_startsIdx.append(std::make_pair(m_lastIdxStart, groupId));
        m_lastIdxStart += count;

        //Create Particle Geometry
        int vCount = count * 4;
        int iCount = count * 6;

        QSGGeometry *g;
        if (perfLevel == Sprites)
            g = new QSGGeometry(SpriteParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Tabled)
            g = new QSGGeometry(DeformableParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Deformable)
            g = new QSGGeometry(DeformableParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Colored)
            g = new QSGGeometry(ColoredParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == ColoredPoint)
            g = new QSGGeometry(ColoredPointParticle_AttributeSet, count, 0);
        else //Simple
            g = new QSGGeometry(SimplePointParticle_AttributeSet, count, 0);

        node->setFlag(QSGNode::OwnsGeometry);
        node->setGeometry(g);
        if (perfLevel <= ColoredPoint){
            g->setDrawingMode(QSGGeometry::DrawPoints);
            if (m_debugMode)
                qDebug("Using point sprites");
        } else {
            g->setDrawingMode(QSGGeometry::DrawTriangles);
        }

        for (int p=0; p < count; ++p)
            commit(groupId, p);//commit sets geometry for the node, has its own perfLevel switch

        if (perfLevel == Sprites)
            initTexCoords<SpriteVertex>((SpriteVertex*)g->vertexData(), vCount);
        else if (perfLevel == Tabled)
            initTexCoords<DeformableVertex>((DeformableVertex*)g->vertexData(), vCount);
        else if (perfLevel == Deformable)
            initTexCoords<DeformableVertex>((DeformableVertex*)g->vertexData(), vCount);
        else if (perfLevel == Colored)
            initTexCoords<ColoredVertex>((ColoredVertex*)g->vertexData(), vCount);

        if (perfLevel > ColoredPoint){
            quint16 *indices = g->indexDataAsUShort();
            for (int i=0; i < count; ++i) {
                int o = i * 4;
                indices[0] = o;
                indices[1] = o + 1;
                indices[2] = o + 2;
                indices[3] = o + 1;
                indices[4] = o + 3;
                indices[5] = o + 2;
                indices += 6;
            }
        }
    }

    if (perfLevel == Sprites)
        spritesUpdate();//Gives all vertexes the initial sprite data, then maintained per frame

    foreach (QSGGeometryNode* node, m_nodes){
        if (node == *(m_nodes.begin()))
            node->setFlag(QSGGeometryNode::OwnsMaterial);//Root node owns the material for memory management purposes
        else
            (*(m_nodes.begin()))->appendChildNode(node);
    }

    *node = *(m_nodes.begin());
    update();
}

QSGNode *QQuickImageParticle::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    if (!m_apiChecked || m_windowChanged) {
        m_apiChecked = true;
        m_windowChanged = false;

        QSGRenderContext *rc = QQuickItemPrivate::get(this)->sceneGraphRenderContext();
        QSGRendererInterface *rif = rc->sceneGraphContext()->rendererInterface(rc);
        if (!rif)
            return nullptr;

        QSGRendererInterface::GraphicsApi api = rif->graphicsApi();
        const bool isRhi = QSGRendererInterface::isApiRhiBased(api);

        if (!node && !isRhi)
            return nullptr;

        if (isRhi)
            m_rhi = static_cast<QRhi *>(rif->getResource(m_window, QSGRendererInterface::RhiResource));
        else
            m_rhi = nullptr;

        if (isRhi && !m_rhi) {
            qWarning("Failed to query QRhi, particles disabled");
            return nullptr;
        }
        // Get the pixel ratio of the window, used for pointsize scaling
        m_dpr = m_window ? m_window->devicePixelRatio() : 1.0;
    }

    if (m_pleaseReset){
        // Cannot just destroy the node and then return null (in case image
        // loading is still in progress). Rather, keep track of the old node
        // until we have a new one.
        delete m_outgoingNode;
        m_outgoingNode = node;
        node = nullptr;

        m_nodes.clear();

        m_idxStarts.clear();
        m_startsIdx.clear();
        m_lastIdxStart = 0;

        m_material = nullptr;

        m_pleaseReset = false;
        m_startedImageLoading = 0;//Cancel a part-way build (may still have a pending load)
    } else if (!m_material) {
        delete node;
        node = nullptr;
    }

    if (m_system && m_system->isRunning() && !m_system->isPaused()){
        bool dirty = prepareNextFrame(&node);
        if (node) {
            update();
            if (dirty) {
                foreach (QSGGeometryNode* n, m_nodes)
                    n->markDirty(QSGNode::DirtyGeometry);
            }
        } else if (m_startedImageLoading < 2) {
            update();//To call prepareNextFrame() again from the renderThread
        }
    }

    if (!node) {
        node = m_outgoingNode;
        m_outgoingNode = nullptr;
    }

    return node;
}

bool QQuickImageParticle::prepareNextFrame(QSGNode **node)
{
    if (*node == nullptr){//TODO: Staggered loading (as emitted)
        buildParticleNodes(node);
        if (m_debugMode) {
            qDebug() << "QQuickImageParticle Feature level: " << perfLevel;
            qDebug() << "QQuickImageParticle Nodes: ";
            int count = 0;
            for (auto it = m_nodes.keyBegin(), end = m_nodes.keyEnd(); it != end; ++it) {
                qDebug() << "Group " << *it << " (" << m_system->groupData[*it]->size()
                         << " particles)";
                count += m_system->groupData[*it]->size();
            }
            qDebug() << "Total count: " << count;
        }
        if (*node == nullptr)
            return false;
    }
    qint64 timeStamp = m_system->systemSync(this);

    qreal time = timeStamp / 1000.;

    switch (perfLevel){//Fall-through intended
    case Sprites:
        //Advance State
        if (m_spriteEngine)
            m_spriteEngine->updateSprites(timeStamp);//fires signals if anim changed
        spritesUpdate(time);
        Q_FALLTHROUGH();
    case Tabled:
    case Deformable:
    case Colored:
    case ColoredPoint:
    case SimplePoint:
    default: //Also Simple
        getState(m_material)->timestamp = time;
        break;
    }

    bool active = false;
    for (auto groupId : groupIds()) {
        if (m_system->groupData[groupId]->isActive()) {
            active = true;
            break;
        }
    }

    const bool dirty = active || m_previousActive;
    if (dirty) {
        foreach (QSGGeometryNode* node, m_nodes)
            node->markDirty(QSGNode::DirtyMaterial);
    }

    m_previousActive = active;
    return dirty;
}

void QQuickImageParticle::spritesUpdate(qreal time)
{
    ImageMaterialData *state = getState(m_material);
    // Sprite progression handled CPU side, so as to have per-frame control.
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* mainDatum : std::as_const(m_system->groupData[groupId]->data)) {
            QSGGeometryNode *node = m_nodes[groupId];
            if (!node)
                continue;
            //TODO: Interpolate between two different animations if it's going to transition next frame
            //      This is particularly important for cut-up sprites.
            QQuickParticleData* datum = (mainDatum->animationOwner == this ? mainDatum : getShadowDatum(mainDatum));
            int spriteIdx = 0;
            for (int i = 0; i<m_startsIdx.size(); i++) {
                if (m_startsIdx[i].second == groupId){
                    spriteIdx = m_startsIdx[i].first + datum->index;
                    break;
                }
            }

            double frameAt;
            qreal progress = 0;

            if (datum->frameDuration > 0) {
                qreal frame = (time - datum->animT)/(datum->frameDuration / 1000.0);
                frame = qBound((qreal)0.0, frame, (qreal)((qreal)datum->frameCount - 1.0));//Stop at count-1 frames until we have between anim interpolation
                if (m_spritesInterpolate)
                    progress = std::modf(frame,&frameAt);
                else
                    std::modf(frame,&frameAt);
            } else {
                datum->frameAt++;
                if (datum->frameAt >= datum->frameCount){
                    datum->frameAt = 0;
                    m_spriteEngine->advance(spriteIdx);
                }
                frameAt = datum->frameAt;
            }
            if (m_spriteEngine->sprite(spriteIdx)->reverse())//### Store this in datum too?
                frameAt = (datum->frameCount - 1) - frameAt;
            QSizeF sheetSize = state->animSheetSize;
            qreal y = datum->animY / sheetSize.height();
            qreal w = datum->animWidth / sheetSize.width();
            qreal h = datum->animHeight / sheetSize.height();
            qreal x1 = datum->animX / sheetSize.width();
            x1 += frameAt * w;
            qreal x2 = x1;
            if (frameAt < (datum->frameCount-1))
                x2 += w;

            SpriteVertex *spriteVertices = (SpriteVertex *) node->geometry()->vertexData();
            spriteVertices += datum->index*4;
            for (int i=0; i<4; i++) {
                spriteVertices[i].animX1 = x1;
                spriteVertices[i].animY1 = y;
                spriteVertices[i].animX2 = x2;
                spriteVertices[i].animW = w;
                spriteVertices[i].animH = h;
                spriteVertices[i].animProgress = progress;
            }
        }
    }
}

void QQuickImageParticle::spriteAdvance(int spriteIdx)
{
    if (!m_startsIdx.size())//Probably overly defensive
        return;

    int gIdx = -1;
    int i;
    for (i = 0; i<m_startsIdx.size(); i++) {
        if (spriteIdx < m_startsIdx[i].first) {
            gIdx = m_startsIdx[i-1].second;
            break;
        }
    }
    if (gIdx == -1)
        gIdx = m_startsIdx[i-1].second;
    int pIdx = spriteIdx - m_startsIdx[i-1].first;

    QQuickParticleData* mainDatum = m_system->groupData[gIdx]->data[pIdx];
    QQuickParticleData* datum = (mainDatum->animationOwner == this ? mainDatum : getShadowDatum(mainDatum));

    datum->animIdx = m_spriteEngine->spriteState(spriteIdx);
    datum->animT = m_spriteEngine->spriteStart(spriteIdx)/1000.0;
    datum->frameCount = m_spriteEngine->spriteFrames(spriteIdx);
    datum->frameDuration = m_spriteEngine->spriteDuration(spriteIdx) / datum->frameCount;
    datum->animX = m_spriteEngine->spriteX(spriteIdx);
    datum->animY = m_spriteEngine->spriteY(spriteIdx);
    datum->animWidth = m_spriteEngine->spriteWidth(spriteIdx);
    datum->animHeight = m_spriteEngine->spriteHeight(spriteIdx);
}

void QQuickImageParticle::initialize(int gIdx, int pIdx)
{
    Color4ub color;
    QQuickParticleData* datum = m_system->groupData[gIdx]->data[pIdx];
    qreal redVariation = m_color_variation + m_redVariation;
    qreal greenVariation = m_color_variation + m_greenVariation;
    qreal blueVariation = m_color_variation + m_blueVariation;
    int spriteIdx = 0;
    if (m_spriteEngine) {
        spriteIdx = m_idxStarts[gIdx] + datum->index;
        if (spriteIdx >= m_spriteEngine->count())
            m_spriteEngine->setCount(spriteIdx+1);
    }

    float rotation;
    float rotationVelocity;
    uchar autoRotate;
    switch (perfLevel){//Fall-through is intended on all of them
        case Sprites:
            // Initial Sprite State
            if (m_explicitAnimation && m_spriteEngine){
                if (!datum->animationOwner)
                    datum->animationOwner = this;
                QQuickParticleData* writeTo = (datum->animationOwner == this ? datum : getShadowDatum(datum));
                writeTo->animT = writeTo->t;
                //writeTo->animInterpolate = m_spritesInterpolate;
                if (m_spriteEngine){
                    m_spriteEngine->start(spriteIdx);
                    writeTo->frameCount = m_spriteEngine->spriteFrames(spriteIdx);
                    writeTo->frameDuration = m_spriteEngine->spriteDuration(spriteIdx) / writeTo->frameCount;
                    writeTo->animIdx = 0;//Always starts at 0
                    writeTo->frameAt = -1;
                    writeTo->animX = m_spriteEngine->spriteX(spriteIdx);
                    writeTo->animY = m_spriteEngine->spriteY(spriteIdx);
                    writeTo->animWidth = m_spriteEngine->spriteWidth(spriteIdx);
                    writeTo->animHeight = m_spriteEngine->spriteHeight(spriteIdx);
                }
            } else {
                ImageMaterialData *state = getState(m_material);
                QQuickParticleData* writeTo = getShadowDatum(datum);
                writeTo->animT = datum->t;
                writeTo->frameCount = 1;
                writeTo->frameDuration = 60000000.0;
                writeTo->frameAt = -1;
                writeTo->animIdx = 0;
                writeTo->animT = 0;
                writeTo->animX = writeTo->animY = 0;
                writeTo->animWidth = state->animSheetSize.width();
                writeTo->animHeight = state->animSheetSize.height();
            }
            Q_FALLTHROUGH();
        case Tabled:
        case Deformable:
            //Initial Rotation
            if (m_explicitDeformation){
                if (!datum->deformationOwner)
                    datum->deformationOwner = this;
                if (m_xVector){
                    const QPointF &ret = m_xVector->sample(QPointF(datum->x, datum->y));
                    if (datum->deformationOwner == this) {
                        datum->xx = ret.x();
                        datum->xy = ret.y();
                    } else {
                        QQuickParticleData* shadow = getShadowDatum(datum);
                        shadow->xx = ret.x();
                        shadow->xy = ret.y();
                    }
                }
                if (m_yVector){
                    const QPointF &ret = m_yVector->sample(QPointF(datum->x, datum->y));
                    if (datum->deformationOwner == this) {
                        datum->yx = ret.x();
                        datum->yy = ret.y();
                    } else {
                        QQuickParticleData* shadow = getShadowDatum(datum);
                        shadow->yx = ret.x();
                        shadow->yy = ret.y();
                    }
                }
            }

            if (m_explicitRotation){
                if (!datum->rotationOwner)
                    datum->rotationOwner = this;
                rotation = qDegreesToRadians(
                    m_rotation + (m_rotationVariation
                                  - 2 * QRandomGenerator::global()->bounded(m_rotationVariation)));
                rotationVelocity = qDegreesToRadians(
                    m_rotationVelocity
                    + (m_rotationVelocityVariation
                       - 2 * QRandomGenerator::global()->bounded(m_rotationVelocityVariation)));
                autoRotate = m_autoRotation ? 1 : 0;
                if (datum->rotationOwner == this) {
                    datum->rotation = rotation;
                    datum->rotationVelocity = rotationVelocity;
                    datum->autoRotate = autoRotate;
                } else {
                    QQuickParticleData* shadow = getShadowDatum(datum);
                    shadow->rotation = rotation;
                    shadow->rotationVelocity = rotationVelocity;
                    shadow->autoRotate = autoRotate;
                }
            }
            Q_FALLTHROUGH();
        case Colored:
            Q_FALLTHROUGH();
        case ColoredPoint:
            //Color initialization
            // Particle color
            if (m_explicitColor) {
                if (!datum->colorOwner)
                    datum->colorOwner = this;
                const auto rgbColor = m_color.toRgb();
                color.r = rgbColor.red() * (1 - redVariation) + QRandomGenerator::global()->bounded(256) * redVariation;
                color.g = rgbColor.green() * (1 - greenVariation) + QRandomGenerator::global()->bounded(256) * greenVariation;
                color.b = rgbColor.blue() * (1 - blueVariation) + QRandomGenerator::global()->bounded(256) * blueVariation;
                color.a = m_alpha * rgbColor.alpha() * (1 - m_alphaVariation) + QRandomGenerator::global()->bounded(256) * m_alphaVariation;
                if (datum->colorOwner == this)
                    datum->color = color;
                else
                    getShadowDatum(datum)->color = color;
            }
            break;
        default:
            break;
    }
}

void QQuickImageParticle::commit(int gIdx, int pIdx)
{
    if (m_pleaseReset)
        return;
    QSGGeometryNode *node = m_nodes[gIdx];
    if (!node)
        return;
    QQuickParticleData* datum = m_system->groupData[gIdx]->data[pIdx];
    SpriteVertex *spriteVertices = (SpriteVertex *) node->geometry()->vertexData();
    DeformableVertex *deformableVertices = (DeformableVertex *) node->geometry()->vertexData();
    ColoredVertex *coloredVertices = (ColoredVertex *) node->geometry()->vertexData();
    ColoredPointVertex *coloredPointVertices = (ColoredPointVertex *) node->geometry()->vertexData();
    SimplePointVertex *simplePointVertices = (SimplePointVertex *) node->geometry()->vertexData();
    switch (perfLevel){//No automatic fall through intended on this one
    case Sprites:
        spriteVertices += pIdx*4;
        for (int i=0; i<4; i++){
            spriteVertices[i].x = datum->x  - m_systemOffset.x();
            spriteVertices[i].y = datum->y  - m_systemOffset.y();
            spriteVertices[i].t = datum->t;
            spriteVertices[i].lifeSpan = datum->lifeSpan;
            spriteVertices[i].size = datum->size;
            spriteVertices[i].endSize = datum->endSize;
            spriteVertices[i].vx = datum->vx;
            spriteVertices[i].vy = datum->vy;
            spriteVertices[i].ax = datum->ax;
            spriteVertices[i].ay = datum->ay;
            if (m_explicitDeformation && datum->deformationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].xx = shadow->xx;
                spriteVertices[i].xy = shadow->xy;
                spriteVertices[i].yx = shadow->yx;
                spriteVertices[i].yy = shadow->yy;
            } else {
                spriteVertices[i].xx = datum->xx;
                spriteVertices[i].xy = datum->xy;
                spriteVertices[i].yx = datum->yx;
                spriteVertices[i].yy = datum->yy;
            }
            if (m_explicitRotation && datum->rotationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].rotation = shadow->rotation;
                spriteVertices[i].rotationVelocity = shadow->rotationVelocity;
                spriteVertices[i].autoRotate = shadow->autoRotate;
            } else {
                spriteVertices[i].rotation = datum->rotation;
                spriteVertices[i].rotationVelocity = datum->rotationVelocity;
                spriteVertices[i].autoRotate = datum->autoRotate;
            }
            //Sprite-related vertices updated per-frame in spritesUpdate(), not on demand
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].color = shadow->color;
            } else {
                spriteVertices[i].color = datum->color;
            }
        }
        break;
    case Tabled: //Fall through until it has its own vertex class
    case Deformable:
        deformableVertices += pIdx*4;
        for (int i=0; i<4; i++){
            deformableVertices[i].x = datum->x  - m_systemOffset.x();
            deformableVertices[i].y = datum->y  - m_systemOffset.y();
            deformableVertices[i].t = datum->t;
            deformableVertices[i].lifeSpan = datum->lifeSpan;
            deformableVertices[i].size = datum->size;
            deformableVertices[i].endSize = datum->endSize;
            deformableVertices[i].vx = datum->vx;
            deformableVertices[i].vy = datum->vy;
            deformableVertices[i].ax = datum->ax;
            deformableVertices[i].ay = datum->ay;
            if (m_explicitDeformation && datum->deformationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].xx = shadow->xx;
                deformableVertices[i].xy = shadow->xy;
                deformableVertices[i].yx = shadow->yx;
                deformableVertices[i].yy = shadow->yy;
            } else {
                deformableVertices[i].xx = datum->xx;
                deformableVertices[i].xy = datum->xy;
                deformableVertices[i].yx = datum->yx;
                deformableVertices[i].yy = datum->yy;
            }
            if (m_explicitRotation && datum->rotationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].rotation = shadow->rotation;
                deformableVertices[i].rotationVelocity = shadow->rotationVelocity;
                deformableVertices[i].autoRotate = shadow->autoRotate;
            } else {
                deformableVertices[i].rotation = datum->rotation;
                deformableVertices[i].rotationVelocity = datum->rotationVelocity;
                deformableVertices[i].autoRotate = datum->autoRotate;
            }
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].color = shadow->color;
            } else {
                deformableVertices[i].color = datum->color;
            }
        }
        break;
    case Colored:
        coloredVertices += pIdx*4;
        for (int i=0; i<4; i++){
            coloredVertices[i].x = datum->x  - m_systemOffset.x();
            coloredVertices[i].y = datum->y  - m_systemOffset.y();
            coloredVertices[i].t = datum->t;
            coloredVertices[i].lifeSpan = datum->lifeSpan;
            coloredVertices[i].size = datum->size;
            coloredVertices[i].endSize = datum->endSize;
            coloredVertices[i].vx = datum->vx;
            coloredVertices[i].vy = datum->vy;
            coloredVertices[i].ax = datum->ax;
            coloredVertices[i].ay = datum->ay;
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                coloredVertices[i].color = shadow->color;
            } else {
                coloredVertices[i].color = datum->color;
            }
        }
        break;
    case ColoredPoint:
        coloredPointVertices += pIdx*1;
        for (int i=0; i<1; i++){
            coloredPointVertices[i].x = datum->x  - m_systemOffset.x();
            coloredPointVertices[i].y = datum->y  - m_systemOffset.y();
            coloredPointVertices[i].t = datum->t;
            coloredPointVertices[i].lifeSpan = datum->lifeSpan;
            coloredPointVertices[i].size = datum->size;
            coloredPointVertices[i].endSize = datum->endSize;
            coloredPointVertices[i].vx = datum->vx;
            coloredPointVertices[i].vy = datum->vy;
            coloredPointVertices[i].ax = datum->ax;
            coloredPointVertices[i].ay = datum->ay;
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                coloredPointVertices[i].color = shadow->color;
            } else {
                coloredPointVertices[i].color = datum->color;
            }
        }
        break;
    case SimplePoint:
        simplePointVertices += pIdx*1;
        for (int i=0; i<1; i++){
            simplePointVertices[i].x = datum->x - m_systemOffset.x();
            simplePointVertices[i].y = datum->y - m_systemOffset.y();
            simplePointVertices[i].t = datum->t;
            simplePointVertices[i].lifeSpan = datum->lifeSpan;
            simplePointVertices[i].size = datum->size;
            simplePointVertices[i].endSize = datum->endSize;
            simplePointVertices[i].vx = datum->vx;
            simplePointVertices[i].vy = datum->vy;
            simplePointVertices[i].ax = datum->ax;
            simplePointVertices[i].ay = datum->ay;
        }
        break;
    default:
        break;
    }
}



QT_END_NAMESPACE

#include "moc_qquickimageparticle_p.cpp"
