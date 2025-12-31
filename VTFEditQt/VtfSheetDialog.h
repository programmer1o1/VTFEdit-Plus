#pragma once

#include "VtfSheet.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;
class QTableWidget;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

class VtfSheetDialog final : public QDialog {
    Q_OBJECT

public:
    explicit VtfSheetDialog(const QByteArray &sheetBytes, QWidget *parent = nullptr);

    QByteArray resultSheetBytes() const;

private:
    void rebuildSequenceTree();
    void rebuildSequenceEditor();
    void rebuildFramesTable();
    void setupFramesColumns();

    VtfSheet::Sequence *currentSequence();
    const VtfSheet::Sequence *currentSequence() const;
    int currentFrameRow() const;

    void addSequence();
    void removeSequence();
    void renumberSequence(std::uint32_t newNum);

    void addFrame();
    void insertFrameAfter();
    void duplicateFrame();
    void removeFrame();
    void moveFrameUp();
    void moveFrameDown();
    void clampSelectedFramesUv01();
    void recalcTotalTime();

    void applySequenceWidgetsToModel();
    void applyCellToModel(int row, int col);

    std::uint32_t currentSeqNum_ = 0;
    bool hasCurrentSeq_ = false;

    VtfSheet sheet_{};
    QByteArray resultBytes_{};

    bool updatingUi_ = false;

    QComboBox *versionCombo_ = nullptr;

    QTreeWidget *seqTree_ = nullptr;
    QToolButton *addSeqBtn_ = nullptr;
    QToolButton *removeSeqBtn_ = nullptr;

    QSpinBox *seqNumSpin_ = nullptr;
    QCheckBox *seqClampCheck_ = nullptr;
    QDoubleSpinBox *seqTotalTimeSpin_ = nullptr;
    QToolButton *seqRecalcBtn_ = nullptr;

    QLabel *infoLabel_ = nullptr;

    QTableWidget *framesTable_ = nullptr;
    QToolButton *addFrameBtn_ = nullptr;
    QToolButton *insertFrameBtn_ = nullptr;
    QToolButton *dupFrameBtn_ = nullptr;
    QToolButton *removeFrameBtn_ = nullptr;
    QToolButton *upFrameBtn_ = nullptr;
    QToolButton *downFrameBtn_ = nullptr;
    QToolButton *clampUvBtn_ = nullptr;
};

