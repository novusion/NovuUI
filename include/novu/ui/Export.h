#pragma once

// ===========================================================================
// Export visibility
// ===========================================================================
// Empty in the default STATIC build, and the reason this header exists is the
// SHARED one.
//
// CMake's WINDOWS_EXPORT_ALL_SYMBOLS carries every FUNCTION out of the DLL
// without anything being annotated, which is why the free functions in this
// library need no macro. It does not carry DATA symbols - the documentation
// says so outright - and every Q_OBJECT class has one: the
// `static const QMetaObject staticMetaObject` that moc defines. A consumer that
// links against the DLL and instantiates one of these classes needs that
// symbol, so a class has to be exported explicitly even when its methods are
// not.
//
// That is the whole distinction: NOVUUI_API appears on class declarations, and
// nowhere else.
//
// ── why the static build defines it empty rather than omitting it ───────────
//
// Four applications link this library statically and must keep doing so with no
// DLL beside them. `__declspec(dllimport)` on a statically linked class is not
// merely unnecessary, it changes how the symbol is referenced, so the macro has
// to collapse to nothing when NOVUUI_SHARED is not defined - which is what the
// build does for every consumer that has not asked for SHARED.

#if defined(NOVUUI_SHARED)
#  if defined(NOVUUI_BUILDING)
#    define NOVUUI_API __declspec(dllexport)
#  else
#    define NOVUUI_API __declspec(dllimport)
#  endif
#else
#  define NOVUUI_API
#endif
