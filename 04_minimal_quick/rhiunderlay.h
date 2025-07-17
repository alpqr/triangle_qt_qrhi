#ifndef RHIUNDERLAY_H
#define RHIUNDERLAY_H

#include <QQuickWindow>
#include <QQuickItem>
#include <rhi/qrhi.h>

class UnderlayRenderer;

// derives from QQuickItem, instances live on the main thread
class RhiUnderlay : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal triangleRotation READ r WRITE setR NOTIFY rChanged)

public:
    RhiUnderlay();

    qreal r() const { return m_r; }
    void setR(qreal t);

signals:
    void rChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    void releaseResources() override;

    qreal m_r = 0;
    UnderlayRenderer *m_renderer = nullptr;
};

// all graphics-related logic and resources - instances live on the render thread, if there is one
class UnderlayRenderer : public QObject
{
    Q_OBJECT

public:
    void setWindow(QQuickWindow *window) { m_window = window; }
    void setR(qreal r) { m_r = r; }

public slots:
    void frameStart();
    void mainPassRecordingStart();

private:
    QQuickWindow *m_window;
    qreal m_r = 0.0f;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
};

#endif
