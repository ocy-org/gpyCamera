#ifndef CAMERACONTROLWIDGET_H
#define CAMERACONTROLWIDGET_H

#include <QDialog>
#include <QTimer>
#include <QSlider>
#include <QLineEdit>
#include <QCloseEvent>
#include <opencv2/opencv.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include "util/comm_define.h"

namespace Ui {
class CameraControlWidget;
}

class CameraControlWidget : public QDialog
{
    Q_OBJECT

public:
    explicit CameraControlWidget(QWidget *parent = nullptr);
    ~CameraControlWidget();
    void init(const QString& cameraname);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateFrame();
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);
    void onHueChanged(int value);
    void onSaturationChanged(int value);
    void onSharpnessChanged(int value);
    void onGammaChanged(int value);
    void onWhiteBalanceChanged(int value);
    void onBacklightChanged(int value);
    void onGainChanged(int value);
    void onExposureChanged(int value);
    void onAutoWhiteBalanceToggled(bool checked);
    void onAutoExposureToggled(bool checked);
    void onResetToDefault();
    void onConfToDefault();
    void onApplySettings();
    void onCancel();

private:
    void initCamera(const QString& cameraname);
    void stopCamera();
    void readCurrentValue(int propId, QSlider* slider, QLineEdit* edit);
    void initSliderRangeAndValue(int propId, QSlider* slider, QLineEdit* edit);
    void getCameraList();

    bool readconfbyjson();
    bool insertjsondata(const st_originalParams& originalParams);
    QString m_cameraname;
    QMap<QString,int> m_cameramap;

    Ui::CameraControlWidget *ui;
    cv::VideoCapture* m_cap = nullptr;
    QTimer* m_timer = nullptr;

    bool m_isApplied = false; // 是否点击过 Apply
    st_originalParams m_originalParams;
    st_originalParams m_conf_originalParams;
    QMap<QString,st_originalParams> m_conf_originalParamsmap;
};

#endif // CAMERACONTROLWIDGET_H
