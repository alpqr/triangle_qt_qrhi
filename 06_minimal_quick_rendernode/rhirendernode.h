#ifndef RHIITEM_H
#define RHIITEM_H

#include <QQuickItem>
#include <QSGRenderNode>
#include <rhi/qrhi.h>

class RhiItemRenderer;

class RhiItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(float triangleRotation READ angle WRITE setAngle NOTIFY angleChanged)

public:
    RhiItem(QQuickItem *parent = nullptr);

    float angle() const { return m_angle; }
    void setAngle(float a);

signals:
    void angleChanged();

private:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;
    float m_angle = 0.0f;
};

class RhiRenderNode : public QSGRenderNode
{
public:
    RhiRenderNode(QQuickWindow *window) : m_window(window) { }

    void prepare() override;
    void render(const RenderState *state) override;
    void releaseResources() override;
    RenderingFlags flags() const override;
    QSGRenderNode::StateFlags changedStates() const override;

private:
    QQuickWindow *m_window;
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    float m_angle = 0.0f;

    friend class RhiItem;
};

#endif
