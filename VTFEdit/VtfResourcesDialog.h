#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;

class VtfResourcesDialog final : public QDialog {
    Q_OBJECT

public:
    explicit VtfResourcesDialog(QWidget *parent = nullptr);
    ~VtfResourcesDialog() override;

    bool applyToBoundVtf(QString *error);

private:
    void loadFromBoundVtf();
    void rebuildOtherResourcesList();
    void updateEnabledStates();
    void loadInformationFieldsFromKvdText();
    void updateKvdTextFromInformationFields();

    QCheckBox *crcEnabled_ = nullptr;
    QLineEdit *crcHex_ = nullptr;

    QCheckBox *lodEnabled_ = nullptr;
    QSpinBox *lodU_ = nullptr;
    QSpinBox *lodV_ = nullptr;

    QCheckBox *tsoEnabled_ = nullptr;
    QSpinBox *tso0_ = nullptr;
    QSpinBox *tso1_ = nullptr;
    QSpinBox *tso2_ = nullptr;
    QSpinBox *tso3_ = nullptr;

    QCheckBox *kvdEnabled_ = nullptr;
    QTabWidget *kvdTabs_ = nullptr;
    QPlainTextEdit *kvdText_ = nullptr;

    QLineEdit *infoAuthor_ = nullptr;
    QLineEdit *infoContact_ = nullptr;
    QLineEdit *infoVersion_ = nullptr;
    QLineEdit *infoModification_ = nullptr;
    QPlainTextEdit *infoDescription_ = nullptr;
    QPlainTextEdit *infoComments_ = nullptr;
    QPlainTextEdit *infoParseStatus_ = nullptr;
    bool updatingKvdUi_ = false;

    QPlainTextEdit *otherResources_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};
