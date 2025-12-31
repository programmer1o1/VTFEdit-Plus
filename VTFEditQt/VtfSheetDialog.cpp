#include "VtfSheetDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QToolButton *makeToolBtn(const QString &text, QWidget *parent) {
    auto *b = new QToolButton(parent);
    b->setText(text);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    return b;
}

double clamp01(double v) {
    if(v < 0.0) return 0.0;
    if(v > 1.0) return 1.0;
    return v;
}

} // namespace

VtfSheetDialog::VtfSheetDialog(const QByteArray &sheetBytes, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Edit Sheet Resource");
    setModal(true);
    resize(980, 640);

    {
        QString err;
        if(!sheetBytes.isEmpty() && !VtfSheet::parse(sheetBytes, &sheet_, &err)) {
            QMessageBox::warning(this, "Sheet parse failed",
                                 QString("This VTF's Sheet resource could not be decoded:\n\n%1\n\n"
                                         "You can still edit it as a new sheet and overwrite the resource.")
                                     .arg(err));
            sheet_ = VtfSheet{};
        }
    }

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    {
        auto *topRow = new QWidget(this);
        auto *topLayout = new QHBoxLayout(topRow);
        topLayout->setContentsMargins(0, 0, 0, 0);

        topLayout->addWidget(new QLabel("Version:", topRow));
        versionCombo_ = new QComboBox(topRow);
        versionCombo_->addItem("0", 0);
        versionCombo_->addItem("1", 1);
        versionCombo_->setCurrentIndex(sheet_.version == 1 ? 1 : 0);
        topLayout->addWidget(versionCombo_);
        topLayout->addStretch(1);

        addSeqBtn_ = makeToolBtn("Add Sequence", topRow);
        removeSeqBtn_ = makeToolBtn("Remove Sequence", topRow);
        topLayout->addWidget(addSeqBtn_);
        topLayout->addWidget(removeSeqBtn_);

        rootLayout->addWidget(topRow);
    }

    auto *split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);

    seqTree_ = new QTreeWidget(split);
    seqTree_->setHeaderLabels({"Sequence", "Frames"});
    seqTree_->setRootIsDecorated(false);
    seqTree_->setUniformRowHeights(true);
    seqTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    seqTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    seqTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    auto *rightPane = new QWidget(split);
    auto *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    {
        auto *seqGroup = new QGroupBox("Sequence", rightPane);
        auto *form = new QFormLayout(seqGroup);
        form->setContentsMargins(8, 8, 8, 8);

        seqNumSpin_ = new QSpinBox(seqGroup);
        seqNumSpin_->setRange(0, 63);
        form->addRow("Number:", seqNumSpin_);

        seqClampCheck_ = new QCheckBox("Clamp", seqGroup);
        form->addRow({}, seqClampCheck_);

        seqTotalTimeSpin_ = new QDoubleSpinBox(seqGroup);
        seqTotalTimeSpin_->setRange(0.0, 99999.0);
        seqTotalTimeSpin_->setDecimals(6);
        seqTotalTimeSpin_->setSingleStep(0.05);
        form->addRow("Total time:", seqTotalTimeSpin_);

        seqRecalcBtn_ = makeToolBtn("Recalc From Frames", seqGroup);
        form->addRow({}, seqRecalcBtn_);

        rightLayout->addWidget(seqGroup);
    }

    {
        auto *framesGroup = new QGroupBox("Frames", rightPane);
        auto *v = new QVBoxLayout(framesGroup);
        v->setContentsMargins(8, 8, 8, 8);
        v->setSpacing(6);

        framesTable_ = new QTableWidget(framesGroup);
        framesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        framesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        framesTable_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        framesTable_->horizontalHeader()->setStretchLastSection(false);
        framesTable_->verticalHeader()->setVisible(true);
        v->addWidget(framesTable_, 1);

        auto *btnRow = new QWidget(framesGroup);
        auto *h = new QHBoxLayout(btnRow);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);

        addFrameBtn_ = makeToolBtn("Add", btnRow);
        insertFrameBtn_ = makeToolBtn("Insert After", btnRow);
        dupFrameBtn_ = makeToolBtn("Duplicate", btnRow);
        removeFrameBtn_ = makeToolBtn("Remove", btnRow);
        upFrameBtn_ = makeToolBtn("Up", btnRow);
        downFrameBtn_ = makeToolBtn("Down", btnRow);
        clampUvBtn_ = makeToolBtn("Clamp UV 0..1", btnRow);

        h->addWidget(addFrameBtn_);
        h->addWidget(insertFrameBtn_);
        h->addWidget(dupFrameBtn_);
        h->addWidget(removeFrameBtn_);
        h->addSpacing(10);
        h->addWidget(upFrameBtn_);
        h->addWidget(downFrameBtn_);
        h->addSpacing(10);
        h->addWidget(clampUvBtn_);
        h->addStretch(1);
        v->addWidget(btnRow);

        rightLayout->addWidget(framesGroup, 1);
    }

    infoLabel_ = new QLabel(rightPane);
    infoLabel_->setWordWrap(true);
    rightLayout->addWidget(infoLabel_);

    split->addWidget(seqTree_);
    split->addWidget(rightPane);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    rootLayout->addWidget(split, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        applySequenceWidgetsToModel();
        QString err;
        const std::uint32_t ver = static_cast<std::uint32_t>(versionCombo_->currentData().toUInt());
        const QByteArray bytes = sheet_.serialize(ver, &err);
        if(bytes.isEmpty() && !err.isEmpty()) {
            QMessageBox::warning(this, "Sheet encode failed", err);
            return;
        }
        resultBytes_ = bytes;
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(addSeqBtn_, &QToolButton::clicked, this, &VtfSheetDialog::addSequence);
    connect(removeSeqBtn_, &QToolButton::clicked, this, &VtfSheetDialog::removeSequence);

    connect(seqTree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *cur, QTreeWidgetItem *) {
        if(updatingUi_) return;
        applySequenceWidgetsToModel();
        if(!cur) {
            hasCurrentSeq_ = false;
            rebuildSequenceEditor();
            rebuildFramesTable();
            return;
        }
        bool ok = false;
        const uint seqNum = cur->data(0, Qt::UserRole).toUInt(&ok);
        if(!ok) return;
        currentSeqNum_ = static_cast<std::uint32_t>(seqNum);
        hasCurrentSeq_ = true;
        rebuildSequenceEditor();
        rebuildFramesTable();
    });

    connect(versionCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
        setupFramesColumns();
        rebuildFramesTable();
    });

    connect(seqNumSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
        if(updatingUi_) return;
        if(!hasCurrentSeq_) return;
        renumberSequence(static_cast<std::uint32_t>(v));
    });
    connect(seqClampCheck_, &QCheckBox::toggled, this, [this](bool) {
        if(updatingUi_) return;
        applySequenceWidgetsToModel();
        rebuildSequenceTree();
    });
    connect(seqTotalTimeSpin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if(updatingUi_) return;
        applySequenceWidgetsToModel();
        rebuildSequenceTree();
    });
    connect(seqRecalcBtn_, &QToolButton::clicked, this, &VtfSheetDialog::recalcTotalTime);

    connect(addFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::addFrame);
    connect(insertFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::insertFrameAfter);
    connect(dupFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::duplicateFrame);
    connect(removeFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::removeFrame);
    connect(upFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::moveFrameUp);
    connect(downFrameBtn_, &QToolButton::clicked, this, &VtfSheetDialog::moveFrameDown);
    connect(clampUvBtn_, &QToolButton::clicked, this, &VtfSheetDialog::clampSelectedFramesUv01);

    connect(framesTable_, &QTableWidget::cellChanged, this, &VtfSheetDialog::applyCellToModel);

    setupFramesColumns();
    rebuildSequenceTree();

    if(!sheet_.sequences.isEmpty()) {
        currentSeqNum_ = static_cast<std::uint32_t>(sheet_.sequences.firstKey());
        hasCurrentSeq_ = true;
    }

    rebuildSequenceEditor();
    rebuildFramesTable();
}

QByteArray VtfSheetDialog::resultSheetBytes() const {
    return resultBytes_;
}

VtfSheet::Sequence *VtfSheetDialog::currentSequence() {
    if(!hasCurrentSeq_) return nullptr;
    auto it = sheet_.sequences.find(currentSeqNum_);
    if(it == sheet_.sequences.end()) return nullptr;
    return &it.value();
}

const VtfSheet::Sequence *VtfSheetDialog::currentSequence() const {
    if(!hasCurrentSeq_) return nullptr;
    auto it = sheet_.sequences.find(currentSeqNum_);
    if(it == sheet_.sequences.end()) return nullptr;
    return &it.value();
}

int VtfSheetDialog::currentFrameRow() const {
    const QModelIndexList selected = framesTable_->selectionModel() ? framesTable_->selectionModel()->selectedRows() : QModelIndexList{};
    if(selected.isEmpty()) return -1;
    return selected.first().row();
}

void VtfSheetDialog::setupFramesColumns() {
    const bool v1 = versionCombo_->currentData().toInt() == 1;
    updatingUi_ = true;
    framesTable_->clear();
    QStringList headers;
    headers << "Duration"
            << "A.Left" << "A.Top" << "A.Right" << "A.Bottom";
    if(v1) {
        headers << "B.Left" << "B.Top" << "B.Right" << "B.Bottom"
                << "C.Left" << "C.Top" << "C.Right" << "C.Bottom"
                << "D.Left" << "D.Top" << "D.Right" << "D.Bottom";
    }
    framesTable_->setColumnCount(headers.size());
    framesTable_->setHorizontalHeaderLabels(headers);
    framesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    updatingUi_ = false;
}

void VtfSheetDialog::rebuildSequenceTree() {
    updatingUi_ = true;
    seqTree_->clear();

    for(auto it = sheet_.sequences.begin(); it != sheet_.sequences.end(); ++it) {
        const auto &seq = it.value();
        auto *item = new QTreeWidgetItem(seqTree_, {QString("Sequence %1").arg(seq.seqNum), QString::number(seq.frames.size())});
        item->setData(0, Qt::UserRole, static_cast<uint>(seq.seqNum));
        item->setToolTip(0, QString("Clamp: %1\nTotal time: %2")
                                .arg(seq.clamp ? "true" : "false")
                                .arg(seq.totalTime, 0, 'g', 9));
    }

    seqTree_->resizeColumnToContents(1);

    // Restore selection.
    if(hasCurrentSeq_) {
        for(int i = 0; i < seqTree_->topLevelItemCount(); ++i) {
            auto *it = seqTree_->topLevelItem(i);
            bool ok = false;
            const uint seqNum = it->data(0, Qt::UserRole).toUInt(&ok);
            if(ok && static_cast<std::uint32_t>(seqNum) == currentSeqNum_) {
                seqTree_->setCurrentItem(it);
                break;
            }
        }
    }

    updatingUi_ = false;
}

void VtfSheetDialog::rebuildSequenceEditor() {
    updatingUi_ = true;
    const auto *seq = currentSequence();
    const bool enable = seq != nullptr;
    seqNumSpin_->setEnabled(enable);
    seqClampCheck_->setEnabled(enable);
    seqTotalTimeSpin_->setEnabled(enable);
    seqRecalcBtn_->setEnabled(enable);

    removeSeqBtn_->setEnabled(enable);

    if(seq) {
        seqNumSpin_->setValue(static_cast<int>(seq->seqNum));
        seqClampCheck_->setChecked(seq->clamp);
        seqTotalTimeSpin_->setValue(static_cast<double>(seq->totalTime));
        infoLabel_->setText(QString("Sequences: %1 (max 64).").arg(sheet_.sequences.size()));
    } else {
        seqNumSpin_->setValue(0);
        seqClampCheck_->setChecked(false);
        seqTotalTimeSpin_->setValue(0.0);
        infoLabel_->setText("No sequence selected.");
    }
    updatingUi_ = false;
}

void VtfSheetDialog::rebuildFramesTable() {
    updatingUi_ = true;
    framesTable_->setRowCount(0);

    auto *seq = currentSequence();
    const bool enable = seq != nullptr;
    addFrameBtn_->setEnabled(enable);
    insertFrameBtn_->setEnabled(enable);
    dupFrameBtn_->setEnabled(enable);
    removeFrameBtn_->setEnabled(enable);
    upFrameBtn_->setEnabled(enable);
    downFrameBtn_->setEnabled(enable);
    clampUvBtn_->setEnabled(enable);
    framesTable_->setEnabled(enable);

    if(!seq) {
        updatingUi_ = false;
        return;
    }

    const bool v1 = versionCombo_->currentData().toInt() == 1;
    framesTable_->setRowCount(seq->frames.size());
    for(int row = 0; row < seq->frames.size(); ++row) {
        const auto &fr = seq->frames[row];
        int col = 0;
        auto set = [&](double val) {
            auto *it = new QTableWidgetItem(QString::number(val, 'g', 9));
            it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            framesTable_->setItem(row, col++, it);
        };
        set(fr.duration);
        set(fr.a.left);
        set(fr.a.top);
        set(fr.a.right);
        set(fr.a.bottom);
        if(v1) {
            set(fr.b.left);
            set(fr.b.top);
            set(fr.b.right);
            set(fr.b.bottom);
            set(fr.c.left);
            set(fr.c.top);
            set(fr.c.right);
            set(fr.c.bottom);
            set(fr.d.left);
            set(fr.d.top);
            set(fr.d.right);
            set(fr.d.bottom);
        }
        framesTable_->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row)));
    }

    framesTable_->resizeColumnsToContents();
    updatingUi_ = false;
}

void VtfSheetDialog::addSequence() {
    applySequenceWidgetsToModel();
    if(sheet_.sequences.size() >= 64) {
        QMessageBox::warning(this, "Cannot add", "A sheet cannot have more than 64 sequences.");
        return;
    }

    std::uint32_t newNum = 0;
    while(newNum < 64 && sheet_.sequences.contains(newNum)) ++newNum;
    if(newNum >= 64) return;

    VtfSheet::Sequence seq{};
    seq.seqNum = newNum;
    seq.clamp = false;
    seq.totalTime = 0.1f;
    VtfSheet::Frame fr{};
    fr.duration = 0.1f;
    fr.a = VtfSheet::defaultFullFrame();
    fr.b = fr.a;
    fr.c = fr.a;
    fr.d = fr.a;
    seq.frames.push_back(fr);
    sheet_.sequences.insert(newNum, seq);

    currentSeqNum_ = newNum;
    hasCurrentSeq_ = true;
    rebuildSequenceTree();
    rebuildSequenceEditor();
    rebuildFramesTable();
}

void VtfSheetDialog::removeSequence() {
    if(!hasCurrentSeq_) return;
    sheet_.sequences.remove(currentSeqNum_);
    hasCurrentSeq_ = !sheet_.sequences.isEmpty();
    if(hasCurrentSeq_) currentSeqNum_ = static_cast<std::uint32_t>(sheet_.sequences.firstKey());
    rebuildSequenceTree();
    rebuildSequenceEditor();
    rebuildFramesTable();
}

void VtfSheetDialog::renumberSequence(std::uint32_t newNum) {
    if(!hasCurrentSeq_) return;
    if(newNum == currentSeqNum_) return;
    if(newNum >= 64) return;
    if(sheet_.sequences.contains(newNum)) {
        updatingUi_ = true;
        seqNumSpin_->setValue(static_cast<int>(currentSeqNum_));
        updatingUi_ = false;
        QMessageBox::warning(this, "Duplicate", QString("Sequence %1 already exists.").arg(newNum));
        return;
    }

    auto it = sheet_.sequences.find(currentSeqNum_);
    if(it == sheet_.sequences.end()) return;
    VtfSheet::Sequence seq = it.value();
    sheet_.sequences.erase(it);
    seq.seqNum = newNum;
    sheet_.sequences.insert(newNum, seq);
    currentSeqNum_ = newNum;
    rebuildSequenceTree();
    rebuildSequenceEditor();
}

void VtfSheetDialog::applySequenceWidgetsToModel() {
    if(updatingUi_) return;
    auto *seq = currentSequence();
    if(!seq) return;
    seq->clamp = seqClampCheck_->isChecked();
    seq->totalTime = static_cast<float>(seqTotalTimeSpin_->value());
}

void VtfSheetDialog::addFrame() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    VtfSheet::Frame fr{};
    fr.duration = 0.1f;
    fr.a = VtfSheet::defaultFullFrame();
    fr.b = fr.a;
    fr.c = fr.a;
    fr.d = fr.a;
    seq->frames.push_back(fr);
    rebuildSequenceTree();
    rebuildFramesTable();
    framesTable_->selectRow(seq->frames.size() - 1);
}

void VtfSheetDialog::insertFrameAfter() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    const int insertAt = row < 0 ? seq->frames.size() : (row + 1);
    VtfSheet::Frame fr{};
    fr.duration = 0.1f;
    fr.a = VtfSheet::defaultFullFrame();
    fr.b = fr.a;
    fr.c = fr.a;
    fr.d = fr.a;
    seq->frames.insert(insertAt, fr);
    rebuildSequenceTree();
    rebuildFramesTable();
    framesTable_->selectRow(insertAt);
}

void VtfSheetDialog::duplicateFrame() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    if(row < 0 || row >= seq->frames.size()) return;
    seq->frames.insert(row + 1, seq->frames[row]);
    rebuildSequenceTree();
    rebuildFramesTable();
    framesTable_->selectRow(row + 1);
}

void VtfSheetDialog::removeFrame() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    if(row < 0 || row >= seq->frames.size()) return;
    seq->frames.removeAt(row);
    rebuildSequenceTree();
    rebuildFramesTable();
    if(row < seq->frames.size()) framesTable_->selectRow(row);
}

void VtfSheetDialog::moveFrameUp() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    if(row <= 0 || row >= seq->frames.size()) return;
    seq->frames.swapItemsAt(row, row - 1);
    rebuildFramesTable();
    framesTable_->selectRow(row - 1);
}

void VtfSheetDialog::moveFrameDown() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    if(row < 0 || row + 1 >= seq->frames.size()) return;
    seq->frames.swapItemsAt(row, row + 1);
    rebuildFramesTable();
    framesTable_->selectRow(row + 1);
}

void VtfSheetDialog::clampSelectedFramesUv01() {
    applySequenceWidgetsToModel();
    auto *seq = currentSequence();
    if(!seq) return;
    const int row = currentFrameRow();
    if(row < 0 || row >= seq->frames.size()) return;

    auto clampTc = [&](VtfSheet::TexCoord &tc) {
        tc.left = static_cast<float>(clamp01(tc.left));
        tc.top = static_cast<float>(clamp01(tc.top));
        tc.right = static_cast<float>(clamp01(tc.right));
        tc.bottom = static_cast<float>(clamp01(tc.bottom));
    };

    auto &fr = seq->frames[row];
    clampTc(fr.a);
    clampTc(fr.b);
    clampTc(fr.c);
    clampTc(fr.d);
    rebuildFramesTable();
    framesTable_->selectRow(row);
}

void VtfSheetDialog::recalcTotalTime() {
    auto *seq = currentSequence();
    if(!seq) return;
    double sum = 0.0;
    for(const auto &fr : seq->frames) sum += fr.duration;
    updatingUi_ = true;
    seqTotalTimeSpin_->setValue(sum);
    updatingUi_ = false;
    applySequenceWidgetsToModel();
    rebuildSequenceTree();
}

void VtfSheetDialog::applyCellToModel(int row, int col) {
    if(updatingUi_) return;
    auto *seq = currentSequence();
    if(!seq) return;
    if(row < 0 || row >= seq->frames.size()) return;
    auto *item = framesTable_->item(row, col);
    if(!item) return;

    bool ok = false;
    const double val = item->text().toDouble(&ok);
    if(!ok) return;

    const bool v1 = versionCombo_->currentData().toInt() == 1;
    auto &fr = seq->frames[row];

    auto setTcField = [&](VtfSheet::TexCoord &tc, int field, double v) {
        switch(field) {
            case 0: tc.left = static_cast<float>(v); break;
            case 1: tc.top = static_cast<float>(v); break;
            case 2: tc.right = static_cast<float>(v); break;
            case 3: tc.bottom = static_cast<float>(v); break;
            default: break;
        }
    };

    if(col == 0) {
        fr.duration = static_cast<float>(val);
        rebuildSequenceTree();
        return;
    }

    // A fields: 1..4.
    if(col >= 1 && col <= 4) {
        setTcField(fr.a, col - 1, val);
        return;
    }

    if(!v1) return;

    // B: 5..8, C: 9..12, D: 13..16.
    if(col >= 5 && col <= 8) {
        setTcField(fr.b, col - 5, val);
    } else if(col >= 9 && col <= 12) {
        setTcField(fr.c, col - 9, val);
    } else if(col >= 13 && col <= 16) {
        setTcField(fr.d, col - 13, val);
    }
}
