#include "rhiunderlay.h"
#include <QtQuick/QQuickWindow>
#include <QtCore/QFile>
#include <QtCore/QRunnable>

RhiUnderlay::RhiUnderlay()
{
    connect(this, &QQuickItem::windowChanged, this, &RhiUnderlay::handleWindowChanged);
}

void RhiUnderlay::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &RhiUnderlay::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &RhiUnderlay::cleanup, Qt::DirectConnection);
        win->setColor(QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f));
    }
}

// The proper and safe way to release custom graphics resources is to both
// connect to sceneGraphInvalidated() and implement releaseResources(). To
// support threaded render loops, the latter performs the UnderlayRenderer
// destruction via scheduleRenderJob(). Note that the RhiUnderlay may be gone by
// the time the QRunnable is invoked.

void RhiUnderlay::cleanup()
{
    // This function is invoked on the render thread, if there is one.

    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable
{
public:
    CleanupJob(UnderlayRenderer *renderer) : m_renderer(renderer) { }
    void run() override { delete m_renderer; }
private:
    UnderlayRenderer *m_renderer;
};

void RhiUnderlay::releaseResources()
{
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

void RhiUnderlay::setAngle(float a)
{
    if (m_angle == a)
        return;

    m_angle = a;
    emit angleChanged();

    // update() (as in QQuickItem's) is not sufficient here; RhiUnderlay is not
    // a proper visual Item, so use the window's update() instead
    if (window())
        window()->update();
}

void RhiUnderlay::sync()
{
    // This function is invoked on the render thread, if there is one.

    if (!m_renderer) {
        m_renderer = new UnderlayRenderer;
        // Initializing resources is done before starting to record the
        // renderpass, regardless of wanting an underlay or overlay.
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &UnderlayRenderer::frameStart, Qt::DirectConnection);
        // Here we want an underlay and therefore connect to
        // beforeRenderPassRecording. Changing to afterRenderPassRecording
        // would render the Underlay on top (overlay).
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &UnderlayRenderer::mainPassRecordingStart, Qt::DirectConnection);
    }
    m_renderer->setAngle(m_angle);
    m_renderer->setWindow(window());
}

void UnderlayRenderer::frameStart()
{
    // This function is invoked on the render thread, if there is one.

    QRhi *rhi = m_window->rhi();
    if (!rhi) {
        qWarning("QQuickWindow is not using QRhi for rendering");
        return;
    }

    QRhiSwapChain *swapChain = m_window->swapChain();
    if (!swapChain) {
        qWarning("No QRhiSwapChain?");
        return;
    }

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
        m_pipeline->setRenderPassDescriptor(swapChain->currentFrameRenderTarget()->renderPassDescriptor());
        m_pipeline->create();

        resourceUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);
    }

    const QSize outputSizeInPixels = swapChain->currentFrameRenderTarget()->pixelSize();
    QMatrix4x4 viewProjection = rhi->clipSpaceCorrMatrix();
    viewProjection.perspective(45.0f, outputSizeInPixels.width() / (float) outputSizeInPixels.height(), 0.01f, 1000.0f);
    viewProjection.translate(0, 0, -4);
    QMatrix4x4 modelViewProjection = viewProjection;
    modelViewProjection.rotate(m_angle, 0, 1, 0);

    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());

    swapChain->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);
}

void UnderlayRenderer::mainPassRecordingStart()
{
    // This function is invoked on the render thread, if there is one.

    QRhi *rhi = m_window->rhi();
    QRhiSwapChain *swapChain = m_window->swapChain();
    if (!rhi || !swapChain)
        return;

    const QSize outputSizeInPixels = swapChain->currentFrameRenderTarget()->pixelSize();
    QRhiCommandBuffer *cb = m_window->swapChain()->currentFrameCommandBuffer();

    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
}
