#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/MonoLineView.h"

#include <QString>
#include <QVector>

namespace novu::ui {

// A unified diff.
//
// The approved NovuAI design's `.diff`: monospace, one tinted row per changed
// line, a hunk header in the accent, and context in the muted ink. It is the
// shape NovuAI's source-control view draws, and the one NovuSourceControl and
// any review panel will draw after it — which is why it is here rather than in
// an application.
//
// ── what belongs here and what does not ──────────────────────────────────
//
// This renders a diff. It does not *produce* one: there is no differ, no
// three-way merge and no notion of a repository. `parseUnified()` reads the
// text format git and every other tool emits, because parsing that string is
// the same work in every caller and getting the hunk header wrong is the same
// bug; anything beyond it is the application's.
//
// Line numbers are the diff's own, on both sides. A viewer that renumbers what
// it is shown is a viewer whose numbers cannot be quoted in a review comment,
// which is most of what the column is for.
class NOVUUI_API DiffView : public MonoLineView
{
    Q_OBJECT

public:
    // The design's four: `.add`, `.del`, `.ctx`, `.hunk`.
    enum class Kind {
        Context,  // unchanged, shown for orientation
        Added,
        Removed,
        Hunk,     // the @@ header
        File,     // a "diff --git" / "+++ b/..." header
    };

    struct Line
    {
        Kind kind = Kind::Context;
        QString text;
        // -1 where the side does not have this line: a removed line has no new
        // number and an added one has no old number.
        int oldNumber = -1;
        int newNumber = -1;
    };

    explicit DiffView(QWidget* parent = nullptr);

    void setLines(const QVector<Line>& lines);
    void addLine(const Line& line);
    void clear();

    int rowCount() const override { return m_lines.size(); }
    Line lineAt(int index) const;

    // Reads the unified format. Understands `@@ -a,b +c,d @@`, the `diff --git`
    // and `+++`/`---` headers, and the leading +/-/space of a body line. A line
    // it cannot classify is context, because showing it unstyled is better than
    // dropping it.
    static QVector<Line> parseUnified(const QString& text);
    void setUnifiedDiff(const QString& text);

    // How many lines each side gained and lost, for a header that wants to say
    // "+128 −34" without the caller walking the list again.
    int addedCount() const { return m_added; }
    int removedCount() const { return m_removed; }

    // The gutter with the two line numbers. Off for a narrow panel, where the
    // numbers cost more width than they are worth.
    bool showsLineNumbers() const { return m_numbers; }
    void setShowLineNumbers(bool show);

signals:
    void lineActivated(int index);

protected:
    void paintRow(QPainter* painter, int row, const QRect& box) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    double lineLeading() const override { return 1.65; }
    int contentPaddingH() const override { return 0; }
    int contentPaddingV() const override { return 0; }

private:
    int gutterWidth() const;

    QVector<Line> m_lines;
    int m_added = 0;
    int m_removed = 0;
    bool m_numbers = true;
};

} // namespace novu::ui
