#pragma once

// What a release did, for the person who opened the window to find out.
//
// Not a changelog viewer. A changelog is the engineering record - every
// commit, in the words the person who made the change wrote for the people
// who maintain it. This is the other document: a short, curated summary of
// what a release changed for its users, which is why these entries are
// shorter than the changelog entries they come from and why there is no
// automatic way to get from one to the other.
//
// ── why it is here and not in each application ──────────────────────────
//
// It was in NovuSpace, which is where the drawing was settled: one scrolling
// column, newest release first, each release stating its version and its date
// and under it only the sections that have anything in them. NovuAI then drew
// the same thing from its own code, and the two started to drift - a badge
// here, a navigator there, a different heading colour. Two drawings of one
// document is exactly what this library exists to prevent.
//
// So the construction lives here and the releases stay with whoever ships
// them. This file holds no release note, knows no product and reads no file.

#include "novu/ui/Text.h"

#include <QList>
#include <QString>
#include <QStringList>

class QWidget;

namespace novu::ui {

// One release, as a release summary states it.
//
// A section with nothing in it is not a section: the page draws no heading
// for an empty list, so a release that only fixed things shows only Fixed.
struct ReleaseNote
{
    QString version;
    QString date;
    QStringList added;
    QStringList improved;
    QStringList fixed;
};

// The documentation window's item: a hairline, a name with an optional key
// beside it, and the explanation under.
//
// The release notes page draws its three sections with this, and the help and
// shortcut windows draw their entries with it, which is the reason it is
// shared rather than private to one of them.
QWidget* documentationItem(const QString& name, const QString& shortcut,
                           const QString& body, Ink nameInk = Ink::Default,
                           QWidget* parent = nullptr);

// The footer these windows wear: a hairline, then something on the left and
// the actions on the right. Either side may be empty.
QWidget* windowFoot(QWidget* leading, const QList<QWidget*>& actions,
                    QWidget* parent = nullptr);

// The release notes page: newest release first, each with only the sections it
// actually has.
//
// `intro` is the line that says whose versions these are - a module that ships
// inside a host application has no release line of its own, and a page that
// leaves that unsaid invites the reader to assume it has one. `lead` is an
// optional widget above the list, scrolling with it.
//
// An empty list is the canonical "no release notes yet" state and not an
// invitation to invent some.
QWidget* releaseNotesPage(QWidget* parent, const QString& intro,
                          const QList<ReleaseNote>& releases,
                          QWidget* lead = nullptr);

} // namespace novu::ui
