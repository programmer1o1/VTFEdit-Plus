#pragma once

#include <QByteArray>
#include <QDialog>

class QDialogButtonBox;
class QPlainTextEdit;

class ResourceHexDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ResourceHexDialog(QString title, QByteArray data, QWidget *parent = nullptr);
    ~ResourceHexDialog() override;

private:
    static QString toHexDump(const QByteArray &data);

    QByteArray data_;
    QPlainTextEdit *edit_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
};

