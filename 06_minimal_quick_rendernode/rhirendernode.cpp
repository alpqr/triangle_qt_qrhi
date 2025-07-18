// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhirendernode.h"
#include <QQuickWindow>
#include <QFile>

// RhiItem lives on the main (gui) thread

RhiItem::RhiItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void RhiItem::setAngle(float a)
{
    if (m_angle == a)
        return;

    m_angle = a;
    emit angleChanged();
    update();
}

QSGNode *RhiItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    // called on the render thread (if there is one), while the main thread blocks

    RhiRenderNode *node = static_cast<RhiRenderNode *>(old);

    if (!node) {
        node = new RhiRenderNode(window());
        window()->setColor(QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f));
    }

    node->m_angle = m_angle;

    return node;
}

// RhiRenderNode lives on the render thread

void RhiRenderNode::releaseResources()
{
    m_vbuf.reset();
    m_ubuf.reset();
    m_srb.reset();
    m_pipeline.reset();
}

QSGRenderNode::RenderingFlags RhiRenderNode::flags() const
{
    return QSGRenderNode::NoExternalRendering;
}

QSGRenderNode::StateFlags RhiRenderNode::changedStates() const
{
    return QSGRenderNode::StateFlag::ViewportState | QSGRenderNode::StateFlag::CullState;
}

void RhiRenderNode::prepare()
{
    QRhi *rhi = m_window->rhi();
    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

    if (!m_pipeline) {
        static float vertexData[] = { // Y up, CCW
            0.0f,   0.5f,     1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
            0.5f,  -0.5f,     0.0f, 0.0f, 1.0f,
        };

        m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
        m_vbuf->create();

        m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
        m_ubuf->create();

        m_srb.reset(rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
        });
        m_srb->create();

        m_pipeline.reset(rhi->newGraphicsPipeline());
        m_pipeline->setDepthTest(true); // unlike other examples
        static auto getShader = [](const QString &name) {
            QFile f(name);
            return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
        };
        m_pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shaders/color.vert.qsb")) },
            { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shaders/color.frag.qsb")) }
        });
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 5 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
        });
        m_pipeline->setVertexInputLayout(inputLayout);
        m_pipeline->setShaderResourceBindings(m_srb.get());
        m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_pipeline->create();

        resourceUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);
    }

    // This is where things get hairy. QSGRenderNode is used to implement
    // "inline" rendering, and is not going through an intermediate texture
    // (unlike minimal_quick_item), and then we try to replicate the behavior of
    // a proper item. (unlike minimal_quick, that is basically fullscreen, here
    // it is up to us to deal with taking the item geometry and placement into
    // account)
    //
    // QMatrix4x4 mvp = *projectionMatrix() * *matrix() would give us a matrix
    // respecting the item geometry, but that's with the scenegraph renderer's
    // orthographic projection, thus expecting vertices in screen space (as in,
    // pixels).
    //
    // QSGRenderNode is _not_ meant to integrate 3D content generally, unlike
    // the other approaches. It is rather suited for specific 2D-ish content
    // when there is a good reason to integrate the custom rendering using this
    // method instead of using a scene underlay/overlay or going through a
    // texture. It should be avoided otherwise.

    // Kind of a bad example. Always render fullscreen, achieving what the
    // underlay approach in minimal_quick in does, except that here rendering
    // happens based on the RhiItem's stacking order. Its size, position, etc.
    // is all ignored.
    const QSize outputSizeInPixels = renderTarget()->pixelSize();
    QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
    mvp.perspective(45.0f, outputSizeInPixels.width() / (float) outputSizeInPixels.height(), 0.01f, 1000.0f);
    mvp.translate(0, 0, -4);
    mvp.rotate(m_angle, 0, 1, 0);

    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
    commandBuffer()->resourceUpdate(resourceUpdates);
}

void RhiRenderNode::render(const RenderState *)
{
    QRhiCommandBuffer *cb = commandBuffer();
    const QSize outputSizeInPixels = renderTarget()->pixelSize();

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
}
