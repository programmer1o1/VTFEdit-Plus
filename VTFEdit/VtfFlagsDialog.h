#pragma once

#include <QDialog>
#include <QList>

#include <VTFLib.h>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QScrollArea;

class VtfFlagsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit VtfFlagsDialog(vlUInt initialFlags, QWidget *parent = nullptr);
    ~VtfFlagsDialog() override;

    vlUInt flags() const;

private slots:
    void filterChanged(const QString &text);
    void updateSummary();
    void selectAll();
    void selectNone();

private:
    struct FlagRow {
        QCheckBox *check = nullptr;
        vlUInt mask = 0;
        QString name;
    };

    void applyFilter();

    vlUInt initialFlags_ = 0;
    QLineEdit *filter_ = nullptr;
    QLabel *summary_ = nullptr;
    QScrollArea *scroll_ = nullptr;
    QWidget *container_ = nullptr;
    QList<FlagRow> rows_;
    QDialogButtonBox *buttons_ = nullptr;
};
