#pragma once

#include <QDialog>

#include <VTFLib.h>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

class WadConvertDialog final : public QDialog {
    Q_OBJECT

public:
    explicit WadConvertDialog(QWidget *parent = nullptr);
    ~WadConvertDialog() override;

private:
    void browseWad();
    void browseOutput();
    void chooseCreateOptions();
    void convert();

    void appendLog(const QString &line);
    void setBusy(bool busy);

    QLineEdit *wadPath_ = nullptr;
    QLineEdit *outputDir_ = nullptr;
    QCheckBox *createVmts_ = nullptr;
    QLineEdit *vmtShader_ = nullptr;
    QPushButton *createOptionsButton_ = nullptr;

    QLabel *progressLabel_ = nullptr;
    QProgressBar *progressBar_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
    QPushButton *convertButton_ = nullptr;

    SVTFCreateOptions createOptions_{};
    bool useAlphaFormat_ = false;
    VTFImageFormat alphaFormat_ = IMAGE_FORMAT_DXT5;
};

