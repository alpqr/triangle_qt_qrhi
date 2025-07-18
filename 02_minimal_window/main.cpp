// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QWindow>
#include <QOffscreenSurface>
#include <QPlatformSurfaceEvent>
#include <rhi/qrhi.h>

class HelloWindow : public QWindow
{
public:
    HelloWindow(QRhi::Implementation graphicsApi);
    void releaseSwapChain();

private:
#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;

    std::unique_ptr<QRhiSwapChain> m_sc;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;
    // not that we need a depth buffer for our triangle; have one just to
    // match what Qt does behind the scenes in minimal_widget
    std::unique_ptr<QRhiRenderBuffer> m_ds;

    QRhi::Implementation m_graphicsApi;
    bool m_initialized = false;
    bool m_hasSwapChain = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;
    QMatrix4x4 m_viewProjection;

    void init();
    void resizeSwapChain();
    void render();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    QRhiResourceUpdateBatch *m_initialUpdates = nullptr;
    float m_rotation = 0.0f;
};

HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : m_graphicsApi(graphicsApi)
{
    switch (graphicsApi) {
    case QRhi::OpenGLES2:
        setSurfaceType(OpenGLSurface);
        break;
    case QRhi::Vulkan:
        setSurfaceType(VulkanSurface);
        break;
    case QRhi::D3D11:
    case QRhi::D3D12:
        setSurfaceType(Direct3DSurface);
        break;
    case QRhi::Metal:
        setSurfaceType(MetalSurface);
        break;
    case QRhi::Null:
        break;
    }
}

void HelloWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed() && !m_initialized) {
        init();
        resizeSwapChain();
        m_initialized = true;
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
        m_notExposed = true;

    if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    if (isExposed() && !surfaceSize.isEmpty())
        render();
}

bool HelloWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

void HelloWindow::init()
{
    if (m_graphicsApi == QRhi::Null) {
        QRhiNullInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Null, &params));
    }

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
    } else if (m_graphicsApi == QRhi::D3D12) {
        QRhiD3D12InitParams params;
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D12, &params));
    }
#endif

#if QT_CONFIG(metal)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params));
    }
#endif

    if (!m_rhi)
        qFatal("Failed to create RHI backend");

    m_sc.reset(m_rhi->newSwapChain());
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                      QSize(),
                                      1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());

    // prepare what we need to render that triangle
    {
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
        m_pipeline->setRenderPassDescriptor(m_rp.get());
        m_pipeline->create();

        m_initialUpdates = m_rhi->nextResourceUpdateBatch();
        m_initialUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);
    }

    setTitle(m_rhi->backendName());
}

void HelloWindow::resizeSwapChain()
{
    m_hasSwapChain = m_sc->createOrResize();

    const QSize outputSize = m_sc->currentPixelSize();
    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    m_viewProjection.translate(0, 0, -4);
}

void HelloWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->destroy();
    }
}

void HelloWindow::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;

    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }

    QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        result = m_rhi->beginFrame(m_sc.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, will retry", result);
        requestUpdate();
        return;
    }

    // the actual rendering
    {
        QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
        const QSize outputSizeInPixels = m_sc->currentPixelSize();

        QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
        if (m_initialUpdates) {
            resourceUpdates->merge(m_initialUpdates);
            m_initialUpdates->release();
            m_initialUpdates = nullptr;
        }

        m_rotation += 1.0f;
        QMatrix4x4 modelViewProjection = m_viewProjection;
        modelViewProjection.rotate(m_rotation, 0, 1, 0);
        resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());

        const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
        cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);

        cb->setGraphicsPipeline(m_pipeline.get());
        cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(3);

        cb->endPass();
    }

    m_rhi->endFrame(m_sc.get());

    requestUpdate();
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QRhi::Implementation graphicsApi;

#if defined(Q_OS_WIN)
    graphicsApi = QRhi::D3D11;
#elif QT_CONFIG(metal)
    graphicsApi = QRhi::Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = QRhi::Vulkan;
#else
    graphicsApi = QRhi::OpenGLES2;
#endif

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Null"));
    cmdLineParser.addOption(nullOption);
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("OpenGL"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Vulkan"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3d11Option({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3d11Option);
    QCommandLineOption d3d12Option({ "D", "d3d12" }, QLatin1String("Direct3D 12"));
    cmdLineParser.addOption(d3d12Option);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);

    cmdLineParser.process(app);
    if (cmdLineParser.isSet(nullOption))
        graphicsApi = QRhi::Null;
    if (cmdLineParser.isSet(glOption))
        graphicsApi = QRhi::OpenGLES2;
    if (cmdLineParser.isSet(vkOption))
        graphicsApi = QRhi::Vulkan;
    if (cmdLineParser.isSet(d3d11Option))
        graphicsApi = QRhi::D3D11;
    if (cmdLineParser.isSet(d3d12Option))
        graphicsApi = QRhi::D3D12;
    if (cmdLineParser.isSet(mtlOption))
        graphicsApi = QRhi::Metal;

    // OpenGL-specifics; the (fake) swapchain has no means to control these when on GL
    {
        QSurfaceFormat fmt;
        fmt.setDepthBufferSize(24);
        fmt.setStencilBufferSize(8);
        QSurfaceFormat::setDefaultFormat(fmt);
    }

#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == QRhi::Vulkan) {
        //inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
        inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
        if (!inst.create()) {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = QRhi::OpenGLES2;
        }
    }
#endif

    HelloWindow window(graphicsApi);

#if QT_CONFIG(vulkan)
    if (graphicsApi == QRhi::Vulkan)
        window.setVulkanInstance(&inst);
#endif
    window.resize(1280, 720);
    window.show();

    int ret = app.exec();

    if (window.handle())
        window.releaseSwapChain();

    return ret;
}
