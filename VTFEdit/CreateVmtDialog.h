#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;

class CreateVmtDialog final : public QDialog {
    Q_OBJECT

public:
    explicit CreateVmtDialog(QString suggestedBaseTexture = {}, QWidget *parent = nullptr, bool suggestAlphaTest = false, bool suggestTranslucent = false);
    ~CreateVmtDialog() override;

    QString vmtText() const;

private slots:
    void updatePreview();

private:
    QString buildVmt() const;

    QComboBox *shader_ = nullptr;
    QLineEdit *baseTexture_ = nullptr;
    QLineEdit *bumpMap_ = nullptr;
    QLineEdit *baseTexture2_ = nullptr;
    QLineEdit *bumpMap2_ = nullptr;
    QLineEdit *envMap_ = nullptr;
    QLineEdit *envMapMask_ = nullptr;
    QLineEdit *detailTexture_ = nullptr;
    QLineEdit *toolTexture_ = nullptr;
    QLineEdit *normalMap_ = nullptr;
    QLineEdit *duDvMap_ = nullptr;
    QLineEdit *surfaceProp1_ = nullptr;
    QLineEdit *surfaceProp2_ = nullptr;
    QLineEdit *keywords_ = nullptr;

    QCheckBox *translucent_ = nullptr;
    QCheckBox *alphaTest_ = nullptr;
    QCheckBox *additive_ = nullptr;
    QCheckBox *noCull_ = nullptr;
    QCheckBox *noDecal_ = nullptr;
    QCheckBox *noLod_ = nullptr;
    QCheckBox *vertexColor_ = nullptr;
    QCheckBox *vertexAlpha_ = nullptr;
    QCheckBox *model_ = nullptr;
    QCheckBox *decal_ = nullptr;
    QCheckBox *halfLambert_ = nullptr;
    QCheckBox *ignoreZ_ = nullptr;
    QCheckBox *noFog_ = nullptr;
    QCheckBox *alphaCoverage_ = nullptr;
    QCheckBox *selfIllum_ = nullptr;
    QPlainTextEdit *preview_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};
