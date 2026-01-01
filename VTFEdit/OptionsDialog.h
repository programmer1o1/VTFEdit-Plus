#pragma once

#include <QDialog>

#include <VTFLib.h>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;

class OptionsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog() override;

private:
    void loadFromSettings();
    void saveToSettings() const;
    void applyVtflibGlobals() const;
    void resetToDefaults();
    void updateEnabledStates();

    QTabWidget *tabs_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;

    // General / Create
    QSpinBox *versionMinor_ = nullptr;
    QComboBox *textureType_ = nullptr;
    QComboBox *normalFormat_ = nullptr;
    QCheckBox *useAlphaFormat_ = nullptr;
    QComboBox *alphaFormat_ = nullptr;
    QCheckBox *mipmaps_ = nullptr;
    QComboBox *mipmapFilter_ = nullptr;
    QCheckBox *thumbnail_ = nullptr;
    QCheckBox *reflectivity_ = nullptr;
    QCheckBox *sphereMap_ = nullptr;
    QCheckBox *srgb_ = nullptr;

    // Advanced
    QCheckBox *gammaCorrection_ = nullptr;
    QDoubleSpinBox *gammaValue_ = nullptr;
    QCheckBox *resizeEnabled_ = nullptr;
    QComboBox *resizeMethod_ = nullptr;
    QComboBox *resizeFilter_ = nullptr;
    QCheckBox *resizeClamp_ = nullptr;
    QSpinBox *resizeClampW_ = nullptr;
    QSpinBox *resizeClampH_ = nullptr;
    QSpinBox *alphaThreshold_ = nullptr;
    QDoubleSpinBox *lumR_ = nullptr;
    QDoubleSpinBox *lumG_ = nullptr;
    QDoubleSpinBox *lumB_ = nullptr;

    // Resources
    QCheckBox *createLodResource_ = nullptr;
    QSpinBox *lodU_ = nullptr;
    QSpinBox *lodV_ = nullptr;
    QCheckBox *createInformationResource_ = nullptr;
    QLineEdit *infoAuthor_ = nullptr;
    QLineEdit *infoContact_ = nullptr;
    QLineEdit *infoVersion_ = nullptr;
    QLineEdit *infoModification_ = nullptr;
    QPlainTextEdit *infoDescription_ = nullptr;
    QPlainTextEdit *infoComments_ = nullptr;
};
