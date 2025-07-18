// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    Q_PROPERTY(float triangleRotation READ angle WRITE setAngle NOTIFY angleChanged)

public:
    RhiUnderlay();

    float angle() const { return m_angle; }
    void setAngle(float a);

signals:
    void angleChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    void releaseResources() override;

    UnderlayRenderer *m_renderer = nullptr;
    float m_angle = 0.0f;
};

// all graphics-related logic and resources - instances live on the render thread, if there is one
class UnderlayRenderer : public QObject
{
    Q_OBJECT

public:
    void setWindow(QQuickWindow *window) { m_window = window; }
    void setAngle(float a) { m_angle = a; }

public slots:
    void frameStart();
    void mainPassRecordingStart();

private:
    QQuickWindow *m_window;
    float m_angle = 0.0f;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
};

#endif
