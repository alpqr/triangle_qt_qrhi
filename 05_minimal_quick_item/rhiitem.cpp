// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhiitem.h"
#include <QFile>

// RhiItem lives on the main (gui) thread

void RhiItem::setAngle(float a)
{
    if (m_angle == a)
        return;

    m_angle = a;
    emit angleChanged();
    update();
}

QQuickRhiItemRenderer *RhiItem::createRenderer()
{
    // Called on the render thread, if there is one.

    return new RhiItemRenderer;
}

// RhiItemRenderer lives on the render thread

void RhiItemRenderer::synchronize(QQuickRhiItem *rhiItem)
{
    // Called on the render thread, if there is one, while the main thread blocks.

    RhiItem *item = static_cast<RhiItem *>(rhiItem);
    if (item->angle() != m_angle)
        m_angle = item->angle();
}

void RhiItemRenderer::initialize(QRhiCommandBuffer *cb)
{
    // Called on the render thread, if there is one, when the item is
    // initialized for the first time, or when the associated texture's size,
    // format, or sample count changes, etc. (so e.g. when interactively
    // resizing the window which in turn changes the RhiItem's geometry too)

    if (m_rhi != rhi()) {
        m_rhi = rhi();
        m_pipeline.reset();
    }

    // In advanced cases one would here check renderTarget()->sampleCount() and
    // color/resolveTexture()->format() to see if they have changed since the
    // last invocation of this function, and reinitialize relevant resources
    // (e.g. the graphics pipeline) if so, but that's not relevant in this
    // example. Here we'll create our pipeline once and that's it.

    if (!m_pipeline) {
        static float vertexData[] = { // Y up, CCW
            0.0f,   0.5f,     1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
            0.5f,  -0.5f,     0.0f, 0.0f, 1.0f,
        };

        m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
        m_vbuf->create();

        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
        m_ubuf->create();

        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
        });
        m_srb->create();

        m_pipeline.reset(m_rhi->newGraphicsPipeline());
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

        QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
        resourceUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);
        cb->resourceUpdate(resourceUpdates);
    }

    const QSize outputSizeInPixels = renderTarget()->pixelSize();
    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, outputSizeInPixels.width() / (float) outputSizeInPixels.height(), 0.01f, 1000.0f);
    m_viewProjection.translate(0, 0, -4);
}

void RhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    // Called on the render thread, if there is one.

    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
    QMatrix4x4 modelViewProjection = m_viewProjection;
    modelViewProjection.rotate(m_angle, 0, 1, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());

    // Qt Quick expects premultiplied alpha, not that it matters in this example
    const float alpha = 1.0f;
    const QColor clearColor = QColor::fromRgbF(0.4f * alpha, 0.7f * alpha, 0.0f * alpha, alpha);

    const QSize outputSizeInPixels = renderTarget()->pixelSize();

    cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    cb->endPass();
}
