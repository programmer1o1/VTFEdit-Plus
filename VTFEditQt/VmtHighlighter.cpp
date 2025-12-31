#include "VmtHighlighter.h"

#include <QRegularExpression>
#include <QTextCharFormat>
#include <QTextDocument>

VmtHighlighter::VmtHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    fmtComment_.setForeground(QColor(120, 120, 120));
    fmtString_.setForeground(QColor(0, 140, 0));
    fmtKey_.setForeground(QColor(0, 90, 160));
    fmtKey_.setFontWeight(QFont::Bold);
    fmtNumber_.setForeground(QColor(160, 0, 160));
    fmtBrace_.setForeground(QColor(180, 120, 0));
    fmtBrace_.setFontWeight(QFont::Bold);
}

VmtHighlighter::~VmtHighlighter() = default;

void VmtHighlighter::highlightBlock(const QString &text) {
    static const QRegularExpression commentRe("//.*$");
    static const QRegularExpression stringRe("\"([^\"\\\\]|\\\\.)*\"");
    static const QRegularExpression keyRe("\"\\$?[A-Za-z0-9_]+\"");
    static const QRegularExpression numberRe("(?<![A-Za-z_])[-+]?(?:\\d+\\.\\d+|\\d+)(?![A-Za-z_])");
    static const QRegularExpression braceRe("[{}]");

    for(auto it = commentRe.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmtComment_);
    }

    for(auto it = braceRe.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmtBrace_);
    }

    for(auto it = numberRe.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmtNumber_);
    }

    for(auto it = keyRe.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmtKey_);
    }

    for(auto it = stringRe.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), fmtString_);
    }
}

