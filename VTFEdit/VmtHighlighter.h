#pragma once

#include <QSyntaxHighlighter>

class VmtHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit VmtHighlighter(QTextDocument *parent);
    ~VmtHighlighter() override;

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat fmtComment_;
    QTextCharFormat fmtString_;
    QTextCharFormat fmtKey_;
    QTextCharFormat fmtNumber_;
    QTextCharFormat fmtBrace_;
};

