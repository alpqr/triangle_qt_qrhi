#ifndef RHIITEM_H
#define RHIITEM_H

#include <QQuickRhiItem>
#include <rhi/qrhi.h>

class RhiItemRenderer;

class RhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(float triangleRotation READ angle WRITE setAngle NOTIFY angleChanged)

public:
    QQuickRhiItemRenderer *createRenderer() override;

    float angle() const { return m_angle; }
    void setAngle(float a);

signals:
    void angleChanged();

private:
    float m_angle = 0.0f;
};

class RhiItemRenderer : public QQuickRhiItemRenderer
{
public:
    void initialize(QRhiCommandBuffer *cb) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;

    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    QMatrix4x4 m_viewProjection;
    float m_angle = 0.0f;
};

#endif
