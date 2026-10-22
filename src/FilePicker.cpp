#include "novu/ui/FilePicker.h"

#include "novu/ui/Button.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Splitter.h"
#include "novu/ui/TableView.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/TreeView.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

constexpr int kNameCellGap = 8;
constexpr int kNameCellGlyph = 16;

// The one cell TableView's own Text role cannot draw by itself: a leading
// glyph before the name, the way every row in the reference reads. Painted
// rather than built from a label and an icon widget side by side - the same
// reasoning every other row in this library draws its own furniture with.
class NameCell : public QWidget
{
public:
    NameCell(Glyph glyph, const QString& text, QWidget* parent)
        : QWidget(parent)
        , m_glyph(glyph)
        , m_text(text)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    QSize sizeHint() const override
    {
        const QFontMetrics fm(roleFont(this, TypeRole::Body));
        return QSize(kNameCellGlyph + kNameCellGap + fm.horizontalAdvance(m_text),
                    qMax(kNameCellGlyph, fm.height()));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF glyphBox(0, (height() - kNameCellGlyph) / 2.0,
                              kNameCellGlyph, kNameCellGlyph);
        paintGlyph(&painter, m_glyph, glyphBox, themeFor(this).colors.textSecondary);
        painter.setFont(roleFont(this, TypeRole::Body));
        painter.setPen(themeFor(this).colors.textPrimary);
        const QRect textBox(kNameCellGlyph + kNameCellGap, 0,
                            width() - kNameCellGlyph - kNameCellGap, height());
        painter.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter,
                         fontMetrics().elidedText(m_text, Qt::ElideRight, textBox.width()));
    }

private:
    Glyph m_glyph;
    QString m_text;
};

} // namespace

FilePicker::FilePicker(QWidget* parent)
    : QWidget(parent)
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(themeFor(this).spacing.md);

    m_splitter = new Splitter(Qt::Horizontal, this);

    m_tree = new TreeView(m_splitter);
    m_tree->setMinimumWidth(160);
    m_splitter->addWidget(m_tree);

    m_table = new TableView(m_splitter);
    m_table->setColumns({{tr("Name"), TableView::ColumnRole::Text},
                         {tr("Date modified"), TableView::ColumnRole::Content},
                         {tr("Type"), TableView::ColumnRole::Content},
                         {tr("Size"), TableView::ColumnRole::Content}});
    m_table->setMinimumWidth(280);
    m_splitter->addWidget(m_table);
    m_splitter->setSizes({220, 560});

    column->addWidget(m_splitter, 1);

    auto* bottom = new QHBoxLayout();
    bottom->setContentsMargins(0, 0, 0, 0);
    bottom->setSpacing(themeFor(this).spacing.sm);

    m_newFolderButton = new Button(tr("New folder"), Button::Variant::Outlined, this);
    m_newFolderButton->setGlyph(Glyph::Plus);
    m_newFolderButton->setObjectName(QStringLiteral("FilePickerNewFolder"));
    m_newFolderButton->setVisible(false);
    bottom->addWidget(m_newFolderButton);

    m_nameField = lineEdit(QString(), this);
    bottom->addWidget(m_nameField, 1);

    m_cancelButton = new Button(tr("Cancel"), Button::Variant::Secondary, this);
    bottom->addWidget(m_cancelButton);

    m_confirmButton = new Button(tr("Select"), Button::Variant::Primary, this);
    bottom->addWidget(m_confirmButton);

    column->addLayout(bottom);

    connect(m_tree, &TreeView::activated, this, &FilePicker::handleLocationActivated);
    connect(m_table, &TableView::rowActivated, this, &FilePicker::handleTableActivated);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            &FilePicker::handleSelectionChanged);
    connect(m_cancelButton, &Button::clicked, this, &FilePicker::cancelled);
    connect(m_newFolderButton, &Button::clicked, this, &FilePicker::newFolderRequested);

    // Right-clicks and renames are passed on rather than acted on: FilePicker
    // does not know what a location is, so it cannot know what a menu on one
    // should offer, or what renaming one means.
    connect(m_tree, &TreeView::contextMenuRequested, this, &FilePicker::locationMenuRequested);
    connect(m_tree, &TreeView::renameCommitted, this, &FilePicker::locationRenamed);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        // QAbstractScrollArea reports the position in its viewport's
        // coordinates, which is also what rowAt() measures in.
        const int row = m_table->rowAt(position.y());
        emit entryMenuRequested(row < 0 || row >= m_entries.size() ? -1 : row,
                                m_table->viewport()->mapToGlobal(position));
    });
    connect(m_confirmButton, &Button::clicked, this, [this] {
        emit confirmed(m_nameField->text());
    });
}

void FilePicker::addLocation(const QString& parentId, const QString& id,
                             const QString& label, Glyph glyph, bool expandable)
{
    m_tree->addNode(parentId, id, label, glyph);
    if (expandable) {
        m_tree->setNodeExpandable(id, true);
    }
}

void FilePicker::setLocationExpanded(const QString& id, bool expanded)
{
    m_tree->setExpanded(id, expanded);
}

void FilePicker::clearLocations()
{
    m_tree->clear();
    m_currentLocation.clear();
}

void FilePicker::setLocationLabel(const QString& id, const QString& label)
{
    m_tree->setLabel(id, label);
}

void FilePicker::renameLocation(const QString& id)
{
    m_tree->revealNode(id);
    m_tree->beginRename(id);
}

void FilePicker::setCurrentLocation(const QString& id)
{
    m_currentLocation = id;
    m_tree->setCurrentId(id);
}

void FilePicker::setEntries(const QVector<Entry>& entries)
{
    m_entries = entries;
    m_table->setRowCount(entries.size());
    for (int r = 0; r < entries.size(); ++r) {
        const Entry& entry = entries.at(r);
        Glyph glyph = entry.glyph;
        if (glyph == Glyph::None) {
            glyph = entry.kind == EntryKind::Folder ? Glyph::Folder : Glyph::File;
        }
        m_table->setWidget(r, 0, new NameCell(glyph, entry.name, m_table));
        m_table->setText(r, 1, entry.dateModified);
        m_table->setText(r, 2, entry.type);
        m_table->setText(r, 3, entry.size);
    }
    m_table->relayoutColumns();
}

void FilePicker::clearEntries()
{
    setEntries({});
}

QString FilePicker::selectedName() const
{
    return m_nameField->text();
}

void FilePicker::setSelectedName(const QString& name)
{
    m_nameField->setText(name);
}

void FilePicker::setConfirmLabel(const QString& text)
{
    m_confirmButton->setText(text);
}

void FilePicker::setConfirmEnabled(bool enabled)
{
    m_confirmButton->setEnabled(enabled);
}

void FilePicker::setNewFolderVisible(bool visible)
{
    m_newFolderButton->setVisible(visible);
}

void FilePicker::selectEntry(const QString& name)
{
    for (int r = 0; r < m_entries.size(); ++r) {
        if (m_entries.at(r).name == name) {
            m_table->selectRow(r);
            m_table->scrollTo(m_table->model()->index(r, 0));
            return;
        }
    }
}

void FilePicker::handleLocationActivated(const QString& id)
{
    emit locationActivated(id);
}

void FilePicker::handleTableActivated(int row)
{
    emit entryActivated(row);
}

void FilePicker::handleSelectionChanged()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_entries.size()) {
        return;
    }
    // A folder is somewhere to go, not a name to hand back - only a file
    // fills the field, the way a platform picker leaves it alone when the
    // highlight lands on a directory.
    if (m_entries.at(row).kind != EntryKind::File) {
        return;
    }
    const QString name = m_entries.at(row).name;
    m_nameField->setText(name);
    emit selectionChanged(name);
}

} // namespace novu::ui
