#include "ResourceHexDialog.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <cctype>

ResourceHexDialog::ResourceHexDialog(QString title, QByteArray data, QWidget *parent)
    : QDialog(parent), data_(std::move(data)) {
    setWindowTitle(std::move(title));
    setModal(true);
    resize(760, 480);

    auto *layout = new QVBoxLayout(this);
    edit_ = new QPlainTextEdit(this);
    edit_->setReadOnly(true);
    edit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    edit_->setPlainText(toHexDump(data_));
    layout->addWidget(edit_);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons_);
}

ResourceHexDialog::~ResourceHexDialog() = default;

QString ResourceHexDialog::toHexDump(const QByteArray &data) {
    constexpr int kBytesPerLine = 16;
    QString out;
    out.reserve(data.size() * 4);

    for(int offset = 0; offset < data.size(); offset += kBytesPerLine) {
        out += QString("%1  ").arg(offset, 8, 16, QChar('0')).toUpper();

        QString ascii;
        ascii.reserve(kBytesPerLine);

        for(int i = 0; i < kBytesPerLine; ++i) {
            if(i == 8) out += ' ';

            const int idx = offset + i;
            if(idx < data.size()) {
                const unsigned char b = static_cast<unsigned char>(data.at(idx));
                out += QString("%1 ").arg(static_cast<unsigned int>(b), 2, 16, QChar('0')).toUpper();
                ascii += (std::isprint(b) ? QChar(static_cast<char>(b)) : QChar('.'));
            } else {
                out += "   ";
                ascii += ' ';
            }
        }

        out += " |";
        out += ascii;
        out += "|\n";
    }

    return out;
}
