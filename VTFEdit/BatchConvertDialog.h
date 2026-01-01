#pragma once

#include <QDialog>

#include <VTFLib.h>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

class BatchConvertDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BatchConvertDialog(QWidget *parent = nullptr);
    ~BatchConvertDialog() override;

    QString inputDir() const;
    QString outputDir() const;
    bool outputUnderMaterials() const;
    QString materialsRoot() const;
    bool recursive() const;
    bool overwrite() const;
    bool createVmts() const;
    QString vmtShader() const;
    QString vmtBaseTexturePrefix() const;

    SVTFCreateOptions createOptions() const;
    bool useAlphaFormat() const;
    VTFImageFormat alphaFormat() const;
    bool createLodResource() const;
    int lodClampU() const;
    int lodClampV() const;
    bool createInformationResource() const;
    QString informationAuthor() const;
    QString informationContact() const;
    QString informationVersion() const;
    QString informationModification() const;
    QString informationDescription() const;
    QString informationComments() const;

protected:
    void accept() override;

private slots:
    void browseInput();
    void browseOutput();
    void browseMaterialsRoot();
    void chooseCreateOptions();
    void savePresetAs();
    void deletePreset();
    void presetChanged(int index);

private:
    void updateUi();
    void loadLast();
    void saveLast() const;
    void loadPreset(const QString &name);
    void storePreset(const QString &name) const;

    QLineEdit *inputDir_ = nullptr;
    QLineEdit *outputDir_ = nullptr;
    QCheckBox *outputUnderMaterials_ = nullptr;
    QLineEdit *materialsRoot_ = nullptr;
    QComboBox *presetCombo_ = nullptr;
    QPushButton *createOptionsButton_ = nullptr;
    QPushButton *savePresetButton_ = nullptr;
    QPushButton *deletePresetButton_ = nullptr;
    QCheckBox *recursive_ = nullptr;
    QCheckBox *overwrite_ = nullptr;
    QCheckBox *createVmts_ = nullptr;
    QCheckBox *useAlphaFormat_ = nullptr;
    QComboBox *alphaFormat_ = nullptr;
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
    QLineEdit *vmtShader_ = nullptr;
    QLineEdit *vmtBaseTexturePrefix_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;

    SVTFCreateOptions createOptions_{};
};
