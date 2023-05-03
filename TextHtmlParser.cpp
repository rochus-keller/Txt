/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Copyright 2013-2017 Rochus Keller <mailto:me@rochus-keller.ch>
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "TextHtmlParser.h"

#include <qbytearray.h>
#include <qtextcodec.h>
#include <qstack.h>
#include <qdebug.h>
#include <qthread.h>
#include <QApplication>
#include <QPalette>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextCursor>
#ifdef _with_resource_provider
#include "private/qtextdocument_p.h"
#endif

#include <algorithm>

//#define NO_CSSPARSER // TEST

// 1:1 QTextHtmlParser von Qt 5.2.1 aus qtexthtmlparser.cpp
// Fazit der Übung: immer noch derselbe Fehler wie in 4.4.3: zuviele Zeilenumbrüche

namespace Txt
{

// see also tst_qtextdocumentfragment.cpp
#define MAX_ENTITY 258
static const struct TextHtmlEntity { const char *name; quint16 code; } entities[MAX_ENTITY]= {
    { "AElig", 0x00c6 },
    { "AMP", 38 },
    { "Aacute", 0x00c1 },
    { "Acirc", 0x00c2 },
    { "Agrave", 0x00c0 },
    { "Alpha", 0x0391 },
    { "Aring", 0x00c5 },
    { "Atilde", 0x00c3 },
    { "Auml", 0x00c4 },
    { "Beta", 0x0392 },
    { "Ccedil", 0x00c7 },
    { "Chi", 0x03a7 },
    { "Dagger", 0x2021 },
    { "Delta", 0x0394 },
    { "ETH", 0x00d0 },
    { "Eacute", 0x00c9 },
    { "Ecirc", 0x00ca },
    { "Egrave", 0x00c8 },
    { "Epsilon", 0x0395 },
    { "Eta", 0x0397 },
    { "Euml", 0x00cb },
    { "GT", 62 },
    { "Gamma", 0x0393 },
    { "Iacute", 0x00cd },
    { "Icirc", 0x00ce },
    { "Igrave", 0x00cc },
    { "Iota", 0x0399 },
    { "Iuml", 0x00cf },
    { "Kappa", 0x039a },
    { "LT", 60 },
    { "Lambda", 0x039b },
    { "Mu", 0x039c },
    { "Ntilde", 0x00d1 },
    { "Nu", 0x039d },
    { "OElig", 0x0152 },
    { "Oacute", 0x00d3 },
    { "Ocirc", 0x00d4 },
    { "Ograve", 0x00d2 },
    { "Omega", 0x03a9 },
    { "Omicron", 0x039f },
    { "Oslash", 0x00d8 },
    { "Otilde", 0x00d5 },
    { "Ouml", 0x00d6 },
    { "Phi", 0x03a6 },
    { "Pi", 0x03a0 },
    { "Prime", 0x2033 },
    { "Psi", 0x03a8 },
    { "QUOT", 34 },
    { "Rho", 0x03a1 },
    { "Scaron", 0x0160 },
    { "Sigma", 0x03a3 },
    { "THORN", 0x00de },
    { "Tau", 0x03a4 },
    { "Theta", 0x0398 },
    { "Uacute", 0x00da },
    { "Ucirc", 0x00db },
    { "Ugrave", 0x00d9 },
    { "Upsilon", 0x03a5 },
    { "Uuml", 0x00dc },
    { "Xi", 0x039e },
    { "Yacute", 0x00dd },
    { "Yuml", 0x0178 },
    { "Zeta", 0x0396 },
    { "aacute", 0x00e1 },
    { "acirc", 0x00e2 },
    { "acute", 0x00b4 },
    { "aelig", 0x00e6 },
    { "agrave", 0x00e0 },
    { "alefsym", 0x2135 },
    { "alpha", 0x03b1 },
    { "amp", 38 },
    { "and", 0x22a5 },
    { "ang", 0x2220 },
    { "apos", 0x0027 },
    { "aring", 0x00e5 },
    { "asymp", 0x2248 },
    { "atilde", 0x00e3 },
    { "auml", 0x00e4 },
    { "bdquo", 0x201e },
    { "beta", 0x03b2 },
    { "brvbar", 0x00a6 },
    { "bull", 0x2022 },
    { "cap", 0x2229 },
    { "ccedil", 0x00e7 },
    { "cedil", 0x00b8 },
    { "cent", 0x00a2 },
    { "chi", 0x03c7 },
    { "circ", 0x02c6 },
    { "clubs", 0x2663 },
    { "cong", 0x2245 },
    { "copy", 0x00a9 },
    { "crarr", 0x21b5 },
    { "cup", 0x222a },
    { "curren", 0x00a4 },
    { "dArr", 0x21d3 },
    { "dagger", 0x2020 },
    { "darr", 0x2193 },
    { "deg", 0x00b0 },
    { "delta", 0x03b4 },
    { "diams", 0x2666 },
    { "divide", 0x00f7 },
    { "eacute", 0x00e9 },
    { "ecirc", 0x00ea },
    { "egrave", 0x00e8 },
    { "empty", 0x2205 },
    { "emsp", 0x2003 },
    { "ensp", 0x2002 },
    { "epsilon", 0x03b5 },
    { "equiv", 0x2261 },
    { "eta", 0x03b7 },
    { "eth", 0x00f0 },
    { "euml", 0x00eb },
    { "euro", 0x20ac },
    { "exist", 0x2203 },
    { "fnof", 0x0192 },
    { "forall", 0x2200 },
    { "frac12", 0x00bd },
    { "frac14", 0x00bc },
    { "frac34", 0x00be },
    { "frasl", 0x2044 },
    { "gamma", 0x03b3 },
    { "ge", 0x2265 },
    { "gt", 62 },
    { "hArr", 0x21d4 },
    { "harr", 0x2194 },
    { "hearts", 0x2665 },
    { "hellip", 0x2026 },
    { "iacute", 0x00ed },
    { "icirc", 0x00ee },
    { "iexcl", 0x00a1 },
    { "igrave", 0x00ec },
    { "image", 0x2111 },
    { "infin", 0x221e },
    { "int", 0x222b },
    { "iota", 0x03b9 },
    { "iquest", 0x00bf },
    { "isin", 0x2208 },
    { "iuml", 0x00ef },
    { "kappa", 0x03ba },
    { "lArr", 0x21d0 },
    { "lambda", 0x03bb },
    { "lang", 0x2329 },
    { "laquo", 0x00ab },
    { "larr", 0x2190 },
    { "lceil", 0x2308 },
    { "ldquo", 0x201c },
    { "le", 0x2264 },
    { "lfloor", 0x230a },
    { "lowast", 0x2217 },
    { "loz", 0x25ca },
    { "lrm", 0x200e },
    { "lsaquo", 0x2039 },
    { "lsquo", 0x2018 },
    { "lt", 60 },
    { "macr", 0x00af },
    { "mdash", 0x2014 },
    { "micro", 0x00b5 },
    { "middot", 0x00b7 },
    { "minus", 0x2212 },
    { "mu", 0x03bc },
    { "nabla", 0x2207 },
    { "nbsp", 0x00a0 },
    { "ndash", 0x2013 },
    { "ne", 0x2260 },
    { "ni", 0x220b },
    { "not", 0x00ac },
    { "notin", 0x2209 },
    { "nsub", 0x2284 },
    { "ntilde", 0x00f1 },
    { "nu", 0x03bd },
    { "oacute", 0x00f3 },
    { "ocirc", 0x00f4 },
    { "oelig", 0x0153 },
    { "ograve", 0x00f2 },
    { "oline", 0x203e },
    { "omega", 0x03c9 },
    { "omicron", 0x03bf },
    { "oplus", 0x2295 },
    { "or", 0x22a6 },
    { "ordf", 0x00aa },
    { "ordm", 0x00ba },
    { "oslash", 0x00f8 },
    { "otilde", 0x00f5 },
    { "otimes", 0x2297 },
    { "ouml", 0x00f6 },
    { "para", 0x00b6 },
    { "part", 0x2202 },
    { "percnt", 0x0025 },
    { "permil", 0x2030 },
    { "perp", 0x22a5 },
    { "phi", 0x03c6 },
    { "pi", 0x03c0 },
    { "piv", 0x03d6 },
    { "plusmn", 0x00b1 },
    { "pound", 0x00a3 },
    { "prime", 0x2032 },
    { "prod", 0x220f },
    { "prop", 0x221d },
    { "psi", 0x03c8 },
    { "quot", 34 },
    { "rArr", 0x21d2 },
    { "radic", 0x221a },
    { "rang", 0x232a },
    { "raquo", 0x00bb },
    { "rarr", 0x2192 },
    { "rceil", 0x2309 },
    { "rdquo", 0x201d },
    { "real", 0x211c },
    { "reg", 0x00ae },
    { "rfloor", 0x230b },
    { "rho", 0x03c1 },
    { "rlm", 0x200f },
    { "rsaquo", 0x203a },
    { "rsquo", 0x2019 },
    { "sbquo", 0x201a },
    { "scaron", 0x0161 },
    { "sdot", 0x22c5 },
    { "sect", 0x00a7 },
    { "shy", 0x00ad },
    { "sigma", 0x03c3 },
    { "sigmaf", 0x03c2 },
    { "sim", 0x223c },
    { "spades", 0x2660 },
    { "sub", 0x2282 },
    { "sube", 0x2286 },
    { "sum", 0x2211 },
    { "sup", 0x2283 },
    { "sup1", 0x00b9 },
    { "sup2", 0x00b2 },
    { "sup3", 0x00b3 },
    { "supe", 0x2287 },
    { "szlig", 0x00df },
    { "tau", 0x03c4 },
    { "there4", 0x2234 },
    { "theta", 0x03b8 },
    { "thetasym", 0x03d1 },
    { "thinsp", 0x2009 },
    { "thorn", 0x00fe },
    { "tilde", 0x02dc },
    { "times", 0x00d7 },
    { "trade", 0x2122 },
    { "uArr", 0x21d1 },
    { "uacute", 0x00fa },
    { "uarr", 0x2191 },
    { "ucirc", 0x00fb },
    { "ugrave", 0x00f9 },
    { "uml", 0x00a8 },
    { "upsih", 0x03d2 },
    { "upsilon", 0x03c5 },
    { "uuml", 0x00fc },
    { "weierp", 0x2118 },
    { "xi", 0x03be },
    { "yacute", 0x00fd },
    { "yen", 0x00a5 },
    { "yuml", 0x00ff },
    { "zeta", 0x03b6 },
    { "zwj", 0x200d },
    { "zwnj", 0x200c }
};

#if defined(Q_CC_MSVC) && _MSC_VER < 1600
static inline bool operator<(const TextHtmlEntity &entity1, const TextHtmlEntity &entity2)
{
    return QLatin1String(entity1.name) < QLatin1String(entity2.name);
}
#endif

static inline bool operator<(const QString &entityStr, const TextHtmlEntity &entity)
{
    return entityStr < QLatin1String(entity.name);
}

static inline bool operator<(const TextHtmlEntity &entity, const QString &entityStr)
{
    return QLatin1String(entity.name) < entityStr;
}

static QChar resolveEntity(const QString &entity)
{
    const TextHtmlEntity *start = &entities[0];
    const TextHtmlEntity *end = &entities[MAX_ENTITY];
    const TextHtmlEntity *e = std::lower_bound(start, end, entity);
    if (e == end || (entity < *e))
        return QChar();
    return e->code;
}

static const ushort windowsLatin1ExtendedCharacters[0xA0 - 0x80] = {
    0x20ac, // 0x80
    0x0081, // 0x81 direct mapping
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02C6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8A
    0x2039, // 0x8B
    0x0152, // 0x8C
    0x008D, // 0x8D direct mapping
    0x017D, // 0x8E
    0x008F, // 0x8F directmapping
    0x0090, // 0x90 directmapping
    0x2018, // 0x91
    0x2019, // 0x92
    0x201C, // 0x93
    0X201D, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02DC, // 0x98
    0x2122, // 0x99
    0x0161, // 0x9A
    0x203A, // 0x9B
    0x0153, // 0x9C
    0x009D, // 0x9D direct mapping
    0x017E, // 0x9E
    0x0178  // 0x9F
};

// the displayMode value is according to the what are blocks in the piecetable, not
// what the w3c defines.
static const TextHtmlElement elements[Html_NumElements]= {
    { "a",          Html_a,          TextHtmlElement::DisplayInline },
    { "address",    Html_address,    TextHtmlElement::DisplayInline },
    { "b",          Html_b,          TextHtmlElement::DisplayInline },
    { "big",        Html_big,        TextHtmlElement::DisplayInline },
    { "blockquote", Html_blockquote, TextHtmlElement::DisplayBlock },
    { "body",       Html_body,       TextHtmlElement::DisplayBlock },
    { "br",         Html_br,         TextHtmlElement::DisplayInline },
    { "caption",    Html_caption,    TextHtmlElement::DisplayBlock },
    { "center",     Html_center,     TextHtmlElement::DisplayBlock },
    { "cite",       Html_cite,       TextHtmlElement::DisplayInline },
    { "code",       Html_code,       TextHtmlElement::DisplayInline },
    { "dd",         Html_dd,         TextHtmlElement::DisplayBlock },
    { "dfn",        Html_dfn,        TextHtmlElement::DisplayInline },
    { "div",        Html_div,        TextHtmlElement::DisplayBlock },
    { "dl",         Html_dl,         TextHtmlElement::DisplayBlock },
    { "dt",         Html_dt,         TextHtmlElement::DisplayBlock },
    { "em",         Html_em,         TextHtmlElement::DisplayInline },
    { "font",       Html_font,       TextHtmlElement::DisplayInline },
    { "h1",         Html_h1,         TextHtmlElement::DisplayBlock },
    { "h2",         Html_h2,         TextHtmlElement::DisplayBlock },
    { "h3",         Html_h3,         TextHtmlElement::DisplayBlock },
    { "h4",         Html_h4,         TextHtmlElement::DisplayBlock },
    { "h5",         Html_h5,         TextHtmlElement::DisplayBlock },
    { "h6",         Html_h6,         TextHtmlElement::DisplayBlock },
    { "head",       Html_head,       TextHtmlElement::DisplayNone },
    { "hr",         Html_hr,         TextHtmlElement::DisplayBlock },
    { "html",       Html_html,       TextHtmlElement::DisplayInline },
    { "i",          Html_i,          TextHtmlElement::DisplayInline },
    { "img",        Html_img,        TextHtmlElement::DisplayInline },
    { "kbd",        Html_kbd,        TextHtmlElement::DisplayInline },
    { "li",         Html_li,         TextHtmlElement::DisplayBlock },
    { "link",       Html_link,       TextHtmlElement::DisplayNone },
    { "meta",       Html_meta,       TextHtmlElement::DisplayNone },
    { "nobr",       Html_nobr,       TextHtmlElement::DisplayInline },
    { "ol",         Html_ol,         TextHtmlElement::DisplayBlock },
    { "p",          Html_p,          TextHtmlElement::DisplayBlock },
    { "pre",        Html_pre,        TextHtmlElement::DisplayBlock },
    { "qt",         Html_body /*deliberate mapping*/, TextHtmlElement::DisplayBlock },
    { "s",          Html_s,          TextHtmlElement::DisplayInline },
    { "samp",       Html_samp,       TextHtmlElement::DisplayInline },
    { "script",     Html_script,     TextHtmlElement::DisplayNone },
    { "small",      Html_small,      TextHtmlElement::DisplayInline },
    { "span",       Html_span,       TextHtmlElement::DisplayInline },
    { "strong",     Html_strong,     TextHtmlElement::DisplayInline },
    { "style",      Html_style,      TextHtmlElement::DisplayNone },
    { "sub",        Html_sub,        TextHtmlElement::DisplayInline },
    { "sup",        Html_sup,        TextHtmlElement::DisplayInline },
    { "table",      Html_table,      TextHtmlElement::DisplayTable },
    { "tbody",      Html_tbody,      TextHtmlElement::DisplayTable },
    { "td",         Html_td,         TextHtmlElement::DisplayBlock },
    { "tfoot",      Html_tfoot,      TextHtmlElement::DisplayTable },
    { "th",         Html_th,         TextHtmlElement::DisplayBlock },
    { "thead",      Html_thead,      TextHtmlElement::DisplayTable },
    { "title",      Html_title,      TextHtmlElement::DisplayNone },
    { "tr",         Html_tr,         TextHtmlElement::DisplayTable },
    { "tt",         Html_tt,         TextHtmlElement::DisplayInline },
    { "u",          Html_u,          TextHtmlElement::DisplayInline },
    { "ul",         Html_ul,         TextHtmlElement::DisplayBlock },
    { "var",        Html_var,        TextHtmlElement::DisplayInline },
};

#if defined(Q_CC_MSVC) && _MSC_VER < 1600
static inline bool operator<(const TextHtmlElement &e1, const TextHtmlElement &e2)
{
    return QLatin1String(e1.name) < QLatin1String(e2.name);
}
#endif

static inline bool operator<(const QString &str, const TextHtmlElement &e)
{
    return str < QLatin1String(e.name);
}

static inline bool operator<(const TextHtmlElement &e, const QString &str)
{
    return QLatin1String(e.name) < str;
}

static const TextHtmlElement *lookupElementHelper(const QString &element)
{
    const TextHtmlElement *start = &elements[0];
    const TextHtmlElement *end = &elements[Html_NumElements];
    const TextHtmlElement *e = std::lower_bound(start, end, element);
    if ((e == end) || (element < *e))
        return 0;
    return e;
}

int TextHtmlParser::lookupElement(const QString &element)
{
    const TextHtmlElement *e = lookupElementHelper(element);
    if (!e)
        return -1;
    return e->id;
}

// quotes newlines as "\\n"
static QString quoteNewline(const QString &s)
{
    QString n = s;
    n.replace(QLatin1Char('\n'), QLatin1String("\\n"));
    return n;
}

TextHtmlParserNode::TextHtmlParserNode()
    : parent(0), id(Html_unknown),
      cssFloat(QTextFrameFormat::InFlow), hasOwnListStyle(false),
      hasCssListIndent(false), isEmptyParagraph(false), isTextFrame(false), isRootFrame(false),
      displayMode(TextHtmlElement::DisplayInline), hasHref(false),
      listStyle(QTextListFormat::ListStyleUndefined), imageWidth(-1), imageHeight(-1), tableBorder(0),
      tableCellRowSpan(1), tableCellColSpan(1), tableCellSpacing(2), tableCellPadding(0),
      borderBrush(Qt::darkGray), borderStyle(QTextFrameFormat::BorderStyle_Outset),
      userState(-1), cssListIndent(0), wsm(WhiteSpaceModeUndefined)
{
    margin[TextHtmlParser::MarginLeft] = 0;
    margin[TextHtmlParser::MarginRight] = 0;
    margin[TextHtmlParser::MarginTop] = 0;
    margin[TextHtmlParser::MarginBottom] = 0;
}

void TextHtmlParser::dumpHtml()
{
    for (int i = 0; i < count(); ++i) {
        qDebug().nospace() << qPrintable(QString(depth(i)*4, QLatin1Char(' ')))
                           << qPrintable(at(i).tag) << ':'
                           << quoteNewline(at(i).text);
            ;
    }
}

TextHtmlParserNode *TextHtmlParser::newNode(int parent)
{
    TextHtmlParserNode *lastNode = &nodes.last();
    TextHtmlParserNode *newNode = 0;

    bool reuseLastNode = true;

    if (nodes.count() == 1) {
        reuseLastNode = false;
    } else if (lastNode->tag.isEmpty()) {

        if (lastNode->text.isEmpty()) {
            reuseLastNode = true;
        } else { // last node is a text node (empty tag) with some text

            if (lastNode->text.length() == 1 && lastNode->text.at(0).isSpace()) {

                int lastSibling = count() - 2;
                while (lastSibling
                       && at(lastSibling).parent != lastNode->parent
                       && at(lastSibling).displayMode == TextHtmlElement::DisplayInline) {
                    lastSibling = at(lastSibling).parent;
                }

                if (at(lastSibling).displayMode == TextHtmlElement::DisplayInline) {
                    reuseLastNode = false;
                } else {
                    reuseLastNode = true;
                }
            } else {
                // text node with real (non-whitespace) text -> nothing to re-use
                reuseLastNode = false;
            }

        }

    } else {
        // last node had a proper tag -> nothing to re-use
        reuseLastNode = false;
    }

    if (reuseLastNode) {
        newNode = lastNode;
        newNode->tag.clear();
        newNode->text.clear();
        newNode->id = Html_unknown;
    } else {
        nodes.resize(nodes.size() + 1);
        newNode = &nodes.last();
    }

    newNode->parent = parent;
    return newNode;
}

void TextHtmlParser::parse(const QString &text, const QTextDocument *_resourceProvider)
{
    nodes.clear();
    nodes.resize(1);
    txt = text;
    pos = 0;
    len = txt.length();
    textEditMode = false;
    resourceProvider = _resourceProvider;
    parse();
    //dumpHtml();
}

int TextHtmlParser::depth(int i) const
{
    int depth = 0;
    while (i) {
        i = at(i).parent;
        ++depth;
    }
    return depth;
}

int TextHtmlParser::margin(int i, int mar) const {
    int m = 0;
    const TextHtmlParserNode *node;
    if (mar == MarginLeft
        || mar == MarginRight) {
        while (i) {
            node = &at(i);
            if (!node->isBlock() && node->id != Html_table)
                break;
            if (node->isTableCell())
                break;
            m += node->margin[mar];
            i = node->parent;
        }
    }
    return m;
}

int TextHtmlParser::topMargin(int i) const
{
    if (!i)
        return 0;
    return at(i).margin[MarginTop];
}

int TextHtmlParser::bottomMargin(int i) const
{
    if (!i)
        return 0;
    return at(i).margin[MarginBottom];
}

void TextHtmlParser::eatSpace()
{
    while (pos < len && txt.at(pos).isSpace() && txt.at(pos) != QChar::ParagraphSeparator)
        pos++;
}

void TextHtmlParser::parse()
{
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c == QLatin1Char('<')) {
            parseTag();
        } else if (c == QLatin1Char('&')) {
            nodes.last().text += parseEntity();
        } else {
            nodes.last().text += c;
        }
    }
}

// parses a tag after "<"
void TextHtmlParser::parseTag()
{
    eatSpace();

    // handle comments and other exclamation mark declarations
    if (hasPrefix(QLatin1Char('!'))) {
        parseExclamationTag();
        if (nodes.last().wsm != TextHtmlParserNode::WhiteSpacePre
            && nodes.last().wsm != TextHtmlParserNode::WhiteSpacePreWrap
            && !textEditMode)
            eatSpace();
        return;
    }

    // if close tag just close
    if (hasPrefix(QLatin1Char('/'))) {
        if (nodes.last().id == Html_style) {
        }
        parseCloseTag();
        return;
    }

    int p = last();
    while (p && at(p).tag.size() == 0)
        p = at(p).parent;

    TextHtmlParserNode *node = newNode(p);

    // parse tag name
    node->tag = parseWord().toLower();

    const TextHtmlElement *elem = lookupElementHelper(node->tag);
    if (elem) {
        node->id = elem->id;
        node->displayMode = elem->displayMode;
    } else {
        node->id = Html_unknown;
    }

    node->attributes.clear();
    // _need_ at least one space after the tag name, otherwise there can't be attributes
    if (pos < len && txt.at(pos).isSpace())
        node->attributes = parseAttributes();

    // resolveParent() may have to change the order in the tree and
    // insert intermediate nodes for buggy HTML, so re-initialize the 'node'
    // pointer through the return value
    node = resolveParent();
    resolveNode();

    const int nodeIndex = nodes.count() - 1; // this new node is always the last
    applyAttributes(node->attributes);

    // finish tag
    bool tagClosed = false;
    while (pos < len && txt.at(pos) != QLatin1Char('>')) {
        if (txt.at(pos) == QLatin1Char('/'))
            tagClosed = true;

        pos++;
    }
    pos++;

    // in a white-space preserving environment strip off a initial newline
    // since the element itself already generates a newline
    if ((node->wsm == TextHtmlParserNode::WhiteSpacePre
         || node->wsm == TextHtmlParserNode::WhiteSpacePreWrap)
        && node->isBlock()) {
        if (pos < len - 1 && txt.at(pos) == QLatin1Char('\n'))
            ++pos;
    }

    if (node->mayNotHaveChildren() || tagClosed) {
        newNode(node->parent);
        resolveNode();
    }
}

// parses a tag beginning with "/"
void TextHtmlParser::parseCloseTag()
{
    ++pos;
    QString tag = parseWord().toLower().trimmed();
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c == QLatin1Char('>'))
            break;
    }

    // find corresponding open node
    int p = last();
    if (p > 0
        && at(p - 1).tag == tag
        && at(p - 1).mayNotHaveChildren())
        p--;

    while (p && at(p).tag != tag)
        p = at(p).parent;

    // simply ignore the tag if we can't find
    // a corresponding open node, for broken
    // html such as <font>blah</font></font>
    if (!p)
        return;

    // in a white-space preserving environment strip off a trailing newline
    // since the closing of the opening block element will automatically result
    // in a new block for elements following the <pre>
    // ...foo\n</pre><p>blah -> foo</pre><p>blah
    if ((at(p).wsm == TextHtmlParserNode::WhiteSpacePre
         || at(p).wsm == TextHtmlParserNode::WhiteSpacePreWrap)
        && at(p).isBlock()) {
        if (at(last()).text.endsWith(QLatin1Char('\n')))
            nodes[last()].text.chop(1);
    }

    newNode(at(p).parent);
    resolveNode();
}

// parses a tag beginning with "!"
void TextHtmlParser::parseExclamationTag()
{
    ++pos;
    if (hasPrefix(QLatin1Char('-'),1) && hasPrefix(QLatin1Char('-'),2)) {
        pos += 3;
        // eat comments
        int end = txt.indexOf(QLatin1String("-->"), pos);
        pos = (end >= 0 ? end + 3 : len);
    } else {
        // eat internal tags
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == QLatin1Char('>'))
                break;
        }
    }
}

// parses an entity after "&", and returns it
QString TextHtmlParser::parseEntity()
{
    int recover = pos;
    QString entity;
    while (pos < len) {
        QChar c = txt.at(pos++);
        if (c.isSpace() || pos - recover > 9) {
            goto error;
        }
        if (c == QLatin1Char(';'))
            break;
        entity += c;
    }
    {
        QChar resolved = resolveEntity(entity);
        if (!resolved.isNull())
            return QString(resolved);
    }
    if (entity.length() > 1 && entity.at(0) == QLatin1Char('#')) {
        entity.remove(0, 1); // removing leading #

        int base = 10;
        bool ok = false;

        if (entity.at(0).toLower() == QLatin1Char('x')) { // hex entity?
            entity.remove(0, 1);
            base = 16;
        }

        uint uc = entity.toUInt(&ok, base);
        if (ok) {
            if (uc >= 0x80 && uc < 0x80 + (sizeof(windowsLatin1ExtendedCharacters)/sizeof(windowsLatin1ExtendedCharacters[0])))
                uc = windowsLatin1ExtendedCharacters[uc - 0x80];
            QString str;
//            if (QChar::requiresSurrogates(uc)) {
//                str += QChar(QChar::highSurrogate(uc));
//                str += QChar(QChar::lowSurrogate(uc));
//            } else {
                str = QChar(uc);
//            }
            return str;
        }
    }
error:
    pos = recover;
    return QLatin1String("&");
}

// parses one word, possibly quoted, and returns it
QString TextHtmlParser::parseWord()
{
    QString word;
    if (hasPrefix(QLatin1Char('\"'))) { // double quotes
        ++pos;
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == QLatin1Char('\"'))
                break;
            else if (c == QLatin1Char('&'))
                word += parseEntity();
            else
                word += c;
        }
    } else if (hasPrefix(QLatin1Char('\''))) { // single quotes
        ++pos;
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == QLatin1Char('\''))
                break;
            else
                word += c;
        }
    } else { // normal text
        while (pos < len) {
            QChar c = txt.at(pos++);
            if (c == QLatin1Char('>')
                || (c == QLatin1Char('/') && hasPrefix(QLatin1Char('>'), 1))
                || c == QLatin1Char('<')
                || c == QLatin1Char('=')
                || c.isSpace()) {
                --pos;
                break;
            }
            if (c == QLatin1Char('&'))
                word += parseEntity();
            else
                word += c;
        }
    }
    return word;
}

// gives the new node the right parent
TextHtmlParserNode *TextHtmlParser::resolveParent()
{
    TextHtmlParserNode *node = &nodes.last();

    int p = node->parent;

    // Excel gives us buggy HTML with just tr without surrounding table tags
    // or with just td tags

    if (node->id == Html_td) {
        int n = p;
        while (n && at(n).id != Html_tr)
            n = at(n).parent;

        if (!n) {
            nodes.insert(nodes.count() - 1, TextHtmlParserNode());
            nodes.insert(nodes.count() - 1, TextHtmlParserNode());

            TextHtmlParserNode *table = &nodes[nodes.count() - 3];
            table->parent = p;
            table->id = Html_table;
            table->tag = QLatin1String("table");
            table->children.append(nodes.count() - 2); // add row as child

            TextHtmlParserNode *row = &nodes[nodes.count() - 2];
            row->parent = nodes.count() - 3; // table as parent
            row->id = Html_tr;
            row->tag = QLatin1String("tr");

            p = nodes.count() - 2;
            node = &nodes.last(); // re-initialize pointer
        }
    }

    if (node->id == Html_tr) {
        int n = p;
        while (n && at(n).id != Html_table)
            n = at(n).parent;

        if (!n) {
            nodes.insert(nodes.count() - 1, TextHtmlParserNode());
            TextHtmlParserNode *table = &nodes[nodes.count() - 2];
            table->parent = p;
            table->id = Html_table;
            table->tag = QLatin1String("table");
            p = nodes.count() - 2;
            node = &nodes.last(); // re-initialize pointer
        }
    }

    // permit invalid html by letting block elements be children
    // of inline elements with the exception of paragraphs:
    //
    // a new paragraph closes parent inline elements (while loop),
    // unless they themselves are children of a non-paragraph block
    // element (if statement)
    //
    // For example:
    //
    // <body><p><b>Foo<p>Bar <-- second <p> implicitly closes <b> that
    //                           belongs to the first <p>. The self-nesting
    //                           check further down prevents the second <p>
    //                           from nesting into the first one then.
    //                           so Bar is not bold.
    //
    // <body><b><p>Foo <-- Foo should be bold.
    //
    // <body><b><p>Foo<p>Bar <-- Foo and Bar should be bold.
    //
    if (node->id == Html_p) {
        while (p && !at(p).isBlock())
            p = at(p).parent;

        if (!p || at(p).id != Html_p)
            p = node->parent;
    }

    // some elements are not self nesting
    if (node->id == at(p).id
        && node->isNotSelfNesting())
        p = at(p).parent;

    // some elements are not allowed in certain contexts
    while ((p && !node->allowedInContext(at(p).id))
           // ### make new styles aware of empty tags
           || at(p).mayNotHaveChildren()
       ) {
        p = at(p).parent;
    }

    node->parent = p;

    // makes it easier to traverse the tree, later
    nodes[p].children.append(nodes.count() - 1);
    return node;
}

// sets all properties on the new node
void TextHtmlParser::resolveNode()
{
    TextHtmlParserNode *node = &nodes.last();
    const TextHtmlParserNode *parent = &nodes.at(node->parent);
    node->initializeProperties(parent, this);
}

bool TextHtmlParserNode::isNestedList(const TextHtmlParser *parser) const
{
    if (!isListStart())
        return false;

    int p = parent;
    while (p) {
        if (parser->at(p).isListStart())
            return true;
        p = parser->at(p).parent;
    }
    return false;
}

void TextHtmlParserNode::initializeProperties(const TextHtmlParserNode *parent, const TextHtmlParser *parser)
{
    // inherit properties from parent element
    charFormat = parent->charFormat;

    if (id == Html_html)
        blockFormat.setLayoutDirection(Qt::LeftToRight); // HTML default
    else if (parent->blockFormat.hasProperty(QTextFormat::LayoutDirection))
        blockFormat.setLayoutDirection(parent->blockFormat.layoutDirection());

    if (parent->displayMode == TextHtmlElement::DisplayNone)
        displayMode = TextHtmlElement::DisplayNone;

    if (parent->id != Html_table || id == Html_caption) {
        if (parent->blockFormat.hasProperty(QTextFormat::BlockAlignment))
            blockFormat.setAlignment(parent->blockFormat.alignment());
        else
            blockFormat.clearProperty(QTextFormat::BlockAlignment);
    }
    // we don't paint per-row background colors, yet. so as an
    // exception inherit the background color here
    // we also inherit the background between inline elements
    if ((parent->id != Html_tr || !isTableCell())
        && (displayMode != TextHtmlElement::DisplayInline || parent->displayMode != TextHtmlElement::DisplayInline)) {
        charFormat.clearProperty(QTextFormat::BackgroundBrush);
    }

    listStyle = parent->listStyle;
    // makes no sense to inherit that property, a named anchor is a single point
    // in the document, which is set by the DocumentFragment
    charFormat.clearProperty(QTextFormat::AnchorName);
    wsm = parent->wsm;

    // initialize remaining properties
    margin[TextHtmlParser::MarginLeft] = 0;
    margin[TextHtmlParser::MarginRight] = 0;
    margin[TextHtmlParser::MarginTop] = 0;
    margin[TextHtmlParser::MarginBottom] = 0;
    cssFloat = QTextFrameFormat::InFlow;

    for (int i = 0; i < 4; ++i)
		padding[i] = -1;

    // set element specific attributes
    switch (id) {
        case Html_a:
            charFormat.setAnchor(true);
            for (int i = 0; i < attributes.count(); i += 2) {
                const QString key = attributes.at(i);
                if (key.compare(QLatin1String("href"), Qt::CaseInsensitive) == 0
                    && !attributes.at(i + 1).isEmpty()) {
                    hasHref = true;
                    charFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                    charFormat.setForeground(QApplication::palette().link());
                }
            }

            break;
        case Html_em:
        case Html_i:
        case Html_cite:
        case Html_address:
        case Html_var:
        case Html_dfn:
            charFormat.setFontItalic(true);
            break;
        case Html_big:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(1));
            break;
        case Html_small:
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(-1));
            break;
        case Html_strong:
        case Html_b:
            charFormat.setFontWeight(QFont::Bold);
            break;
        case Html_h1:
            charFormat.setFontWeight(QFont::Bold);
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(3));
            margin[TextHtmlParser::MarginTop] = 18;
            margin[TextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h2:
            charFormat.setFontWeight(QFont::Bold);
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(2));
            margin[TextHtmlParser::MarginTop] = 16;
            margin[TextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h3:
            charFormat.setFontWeight(QFont::Bold);
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(1));
            margin[TextHtmlParser::MarginTop] = 14;
            margin[TextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h4:
            charFormat.setFontWeight(QFont::Bold);
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(0));
            margin[TextHtmlParser::MarginTop] = 12;
            margin[TextHtmlParser::MarginBottom] = 12;
            break;
        case Html_h5:
            charFormat.setFontWeight(QFont::Bold);
            charFormat.setProperty(QTextFormat::FontSizeAdjustment, int(-1));
            margin[TextHtmlParser::MarginTop] = 12;
            margin[TextHtmlParser::MarginBottom] = 4;
            break;
        case Html_p:
            margin[TextHtmlParser::MarginTop] = 12;
            margin[TextHtmlParser::MarginBottom] = 12;
            break;
        case Html_center:
            blockFormat.setAlignment(Qt::AlignCenter);
            break;
        case Html_ul:
            listStyle = QTextListFormat::ListDisc;
            // nested lists don't have margins, except for the toplevel one
            if (!isNestedList(parser)) {
                margin[TextHtmlParser::MarginTop] = 12;
                margin[TextHtmlParser::MarginBottom] = 12;
            }
            // no left margin as we use indenting instead
            break;
        case Html_ol:
            listStyle = QTextListFormat::ListDecimal;
            // nested lists don't have margins, except for the toplevel one
            if (!isNestedList(parser)) {
                margin[TextHtmlParser::MarginTop] = 12;
                margin[TextHtmlParser::MarginBottom] = 12;
            }
            // no left margin as we use indenting instead
            break;
        case Html_code:
        case Html_tt:
        case Html_kbd:
        case Html_samp:
            charFormat.setFontFamily(QString::fromLatin1("Courier New,courier"));
            // <tt> uses a fixed font, so set the property
            charFormat.setFontFixedPitch(true);
            break;
        case Html_br:
            text = QChar(QChar::LineSeparator);
            wsm = TextHtmlParserNode::WhiteSpacePre;
            break;
        // ##### sub / sup
        case Html_pre:
            charFormat.setFontFamily(QString::fromLatin1("Courier New,courier"));
            wsm = WhiteSpacePre;
            margin[TextHtmlParser::MarginTop] = 12;
            margin[TextHtmlParser::MarginBottom] = 12;
            // <pre> uses a fixed font
            charFormat.setFontFixedPitch(true);
            break;
        case Html_blockquote:
            margin[TextHtmlParser::MarginTop] = 12;
            margin[TextHtmlParser::MarginBottom] = 12;
            margin[TextHtmlParser::MarginLeft] = 40;
            margin[TextHtmlParser::MarginRight] = 40;
            break;
        case Html_dl:
            margin[TextHtmlParser::MarginTop] = 8;
            margin[TextHtmlParser::MarginBottom] = 8;
            break;
        case Html_dd:
            margin[TextHtmlParser::MarginLeft] = 30;
            break;
        case Html_u:
            charFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            break;
        case Html_s:
            charFormat.setFontStrikeOut(true);
            break;
        case Html_nobr:
            wsm = WhiteSpaceNoWrap;
            break;
        case Html_th:
            charFormat.setFontWeight(QFont::Bold);
            blockFormat.setAlignment(Qt::AlignCenter);
            break;
        case Html_td:
            blockFormat.setAlignment(Qt::AlignLeft);
            break;
        case Html_sub:
            charFormat.setVerticalAlignment(QTextCharFormat::AlignSubScript);
            break;
        case Html_sup:
            charFormat.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
            break;
        default: break;
    }
}

void TextHtmlParserNode::applyBackgroundImage(const QString &url, const QTextDocument *resourceProvider)
{
#ifdef _with_resource_provider
    if (!url.isEmpty() && resourceProvider) {
        QVariant val = resourceProvider->resource(QTextDocument::ImageResource, url);

        if (QCoreApplication::instance()->thread() != QThread::currentThread()) {
            // must use images in non-GUI threads
            if (val.type() == QVariant::Image) {
                QImage image = qvariant_cast<QImage>(val);
                charFormat.setBackground(image);
            } else if (val.type() == QVariant::ByteArray) {
                QImage image;
                if (image.loadFromData(val.toByteArray())) {
                    charFormat.setBackground(image);
                }
            }
        } else {
            if (val.type() == QVariant::Image || val.type() == QVariant::Pixmap) {
                charFormat.setBackground(qvariant_cast<QPixmap>(val));
            } else if (val.type() == QVariant::ByteArray) {
                QPixmap pm;
                if (pm.loadFromData(val.toByteArray())) {
                    charFormat.setBackground(pm);
                }
            }
        }
    }
#endif
    if (!url.isEmpty())
        charFormat.setProperty(QTextFormat::BackgroundImageUrl, url);
}

bool TextHtmlParserNode::hasOnlyWhitespace() const
{
    for (int i = 0; i < text.count(); ++i)
        if (!text.at(i).isSpace() || text.at(i) == QChar::LineSeparator)
            return false;
    return true;
}

static bool setIntAttribute(int *destination, const QString &value)
{
    bool ok = false;
    int val = value.toInt(&ok);
    if (ok)
        *destination = val;

    return ok;
}

static bool setFloatAttribute(qreal *destination, const QString &value)
{
    bool ok = false;
    qreal val = value.toDouble(&ok);
    if (ok)
        *destination = val;

    return ok;
}

static void setWidthAttribute(QTextLength *width, QString value)
{
    bool ok = false;
    qreal realVal = value.toDouble(&ok);
    if (ok) {
        *width = QTextLength(QTextLength::FixedLength, realVal);
    } else {
        value = value.trimmed();
        if (!value.isEmpty() && value.endsWith(QLatin1Char('%'))) {
            value.chop(1);
            realVal = value.toDouble(&ok);
            if (ok)
                *width = QTextLength(QTextLength::PercentageLength, realVal);
        }
    }
}

void TextHtmlParserNode::parseStyleAttribute(const QString &value, const QTextDocument *resourceProvider)
{
	return;
}

QStringList TextHtmlParser::parseAttributes()
{
    QStringList attrs;

    while (pos < len) {
        eatSpace();
        if (hasPrefix(QLatin1Char('>')) || hasPrefix(QLatin1Char('/')))
            break;
        QString key = parseWord().toLower();
        QString value = QLatin1String("1");
        if (key.size() == 0)
            break;
        eatSpace();
        if (hasPrefix(QLatin1Char('='))){
            pos++;
            eatSpace();
            value = parseWord();
        }
        if (value.size() == 0)
            continue;
        attrs << key << value;
    }

    return attrs;
}

void TextHtmlParser::applyAttributes(const QStringList &attributes)
{
    // local state variable for qt3 textedit mode
    bool seenQt3Richtext = false;
    QString linkHref;
    QString linkType;

    if (attributes.count() % 2 == 1)
        return;

    TextHtmlParserNode *node = &nodes.last();

    for (int i = 0; i < attributes.count(); i += 2) {
        QString key = attributes.at(i);
        QString value = attributes.at(i + 1);

        switch (node->id) {
            case Html_font:
                // the infamous font tag
				if (key == QLatin1String("size") && value.size()) {
					int n = value.toInt();
					if (value.at(0) != QLatin1Char('+') && value.at(0) != QLatin1Char('-'))
						n -= 3;
					node->charFormat.setProperty(QTextFormat::FontSizeAdjustment, n);
				} else if (key == QLatin1String("face")) {
					node->charFormat.setFontFamily(value);
				} else if (key == QLatin1String("color")) {
					QColor c; c.setNamedColor(value);
					if (!c.isValid())
						qWarning("TextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
					node->charFormat.setForeground(c);
				}
				break;
            case Html_ol:
            case Html_ul:
                if (key == QLatin1String("type")) {
                    node->hasOwnListStyle = true;
                    if (value == QLatin1String("1")) {
                        node->listStyle = QTextListFormat::ListDecimal;
                    } else if (value == QLatin1String("a")) {
                        node->listStyle = QTextListFormat::ListLowerAlpha;
                    } else if (value == QLatin1String("A")) {
                        node->listStyle = QTextListFormat::ListUpperAlpha;
//                    } else if (value == QLatin1String("i")) {
//                        node->listStyle = QTextListFormat::ListLowerRoman;
//                    } else if (value == QLatin1String("I")) {
//                        node->listStyle = QTextListFormat::ListUpperRoman;
                    } else {
                        value = value.toLower();
                        if (value == QLatin1String("square"))
                            node->listStyle = QTextListFormat::ListSquare;
                        else if (value == QLatin1String("disc"))
                            node->listStyle = QTextListFormat::ListDisc;
                        else if (value == QLatin1String("circle"))
                            node->listStyle = QTextListFormat::ListCircle;
                    }
                }
                break;
            case Html_a:
                if (key == QLatin1String("href"))
                    node->charFormat.setAnchorHref(value);
                else if (key == QLatin1String("name"))
                    node->charFormat.setAnchorName(value);
                break;
            case Html_img:
                if (key == QLatin1String("src") || key == QLatin1String("source")) {
                    node->imageName = value;
                } else if (key == QLatin1String("width")) {
                    node->imageWidth = -2; // register that there is a value for it.
                    setFloatAttribute(&node->imageWidth, value);
                } else if (key == QLatin1String("height")) {
                    node->imageHeight = -2; // register that there is a value for it.
                    setFloatAttribute(&node->imageHeight, value);
                }
                break;
            case Html_tr:
            case Html_body:
                if (key == QLatin1String("bgcolor")) {
                    QColor c; c.setNamedColor(value);
                    if (!c.isValid())
                        qWarning("TextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == QLatin1String("background")) {
                    node->applyBackgroundImage(value, resourceProvider);
                }
                break;
            case Html_th:
            case Html_td:
                if (key == QLatin1String("width")) {
                    setWidthAttribute(&node->width, value);
                } else if (key == QLatin1String("bgcolor")) {
                    QColor c; c.setNamedColor(value);
                    if (!c.isValid())
                        qWarning("TextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == QLatin1String("background")) {
                    node->applyBackgroundImage(value, resourceProvider);
                } else if (key == QLatin1String("rowspan")) {
                    if (setIntAttribute(&node->tableCellRowSpan, value))
                        node->tableCellRowSpan = qMax(1, node->tableCellRowSpan);
                } else if (key == QLatin1String("colspan")) {
                    if (setIntAttribute(&node->tableCellColSpan, value))
                        node->tableCellColSpan = qMax(1, node->tableCellColSpan);
                }
                break;
            case Html_table:
                if (key == QLatin1String("border")) {
                    setFloatAttribute(&node->tableBorder, value);
                } else if (key == QLatin1String("bgcolor")) {
                    QColor c; c.setNamedColor(value);
                    if (!c.isValid())
                        qWarning("TextHtmlParser::applyAttributes: Unknown color name '%s'",value.toLatin1().constData());
                    node->charFormat.setBackground(c);
                } else if (key == QLatin1String("background")) {
                    node->applyBackgroundImage(value, resourceProvider);
                } else if (key == QLatin1String("cellspacing")) {
                    setFloatAttribute(&node->tableCellSpacing, value);
                } else if (key == QLatin1String("cellpadding")) {
                    setFloatAttribute(&node->tableCellPadding, value);
                } else if (key == QLatin1String("width")) {
                    setWidthAttribute(&node->width, value);
                } else if (key == QLatin1String("height")) {
                    setWidthAttribute(&node->height, value);
                }
                break;
            case Html_meta:
                if (key == QLatin1String("name")
                    && value == QLatin1String("qrichtext")) {
                    seenQt3Richtext = true;
                }

                if (key == QLatin1String("content")
                    && value == QLatin1String("1")
                    && seenQt3Richtext) {

                    textEditMode = true;
                }
                break;
            case Html_hr:
                if (key == QLatin1String("width"))
                    setWidthAttribute(&node->width, value);
                break;
            case Html_link:
                if (key == QLatin1String("href"))
                    linkHref = value;
                else if (key == QLatin1String("type"))
                    linkType = value;
                break;
            default:
                break;
        }

        if (key == QLatin1String("style")) {
            node->parseStyleAttribute(value, resourceProvider);
        } else if (key == QLatin1String("align")) {
            value = value.toLower();
            bool alignmentSet = true;

            if (value == QLatin1String("left"))
                node->blockFormat.setAlignment(Qt::AlignLeft|Qt::AlignAbsolute);
            else if (value == QLatin1String("right"))
                node->blockFormat.setAlignment(Qt::AlignRight|Qt::AlignAbsolute);
            else if (value == QLatin1String("center"))
                node->blockFormat.setAlignment(Qt::AlignHCenter);
            else if (value == QLatin1String("justify"))
                node->blockFormat.setAlignment(Qt::AlignJustify);
            else
                alignmentSet = false;

            if (node->id == Html_img) {
                // HTML4 compat
                if (alignmentSet) {
                    if (node->blockFormat.alignment() & Qt::AlignLeft)
                        node->cssFloat = QTextFrameFormat::FloatLeft;
                    else if (node->blockFormat.alignment() & Qt::AlignRight)
                        node->cssFloat = QTextFrameFormat::FloatRight;
                } else if (value == QLatin1String("middle")) {
                    node->charFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle);
                } else if (value == QLatin1String("top")) {
                    node->charFormat.setVerticalAlignment(QTextCharFormat::AlignTop);
                }
            }
        } else if (key == QLatin1String("valign")) {
            value = value.toLower();
            if (value == QLatin1String("top"))
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignTop);
            else if (value == QLatin1String("middle"))
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle);
            else if (value == QLatin1String("bottom"))
                node->charFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        } else if (key == QLatin1String("dir")) {
            value = value.toLower();
            if (value == QLatin1String("ltr"))
                node->blockFormat.setLayoutDirection(Qt::LeftToRight);
            else if (value == QLatin1String("rtl"))
                node->blockFormat.setLayoutDirection(Qt::RightToLeft);
        } else if (key == QLatin1String("title")) {
            node->charFormat.setToolTip(value);
        } else if (key == QLatin1String("id")) {
            node->charFormat.setAnchor(true);
            node->charFormat.setAnchorName(value);
        }
    }

    if (resourceProvider && !linkHref.isEmpty() && linkType == QLatin1String("text/css"))
        importStyleSheet(linkHref);
}

void TextHtmlParser::importStyleSheet(const QString &href)
{
#ifdef _with_resource_provider
    if (!resourceProvider)
        return;
    for (int i = 0; i < externalStyleSheets.count(); ++i)
        if (externalStyleSheets.at(i).url == href)
            return;

    QVariant res = resourceProvider->resource(QTextDocument::StyleSheetResource, href);
    QString css;
    if (res.type() == QVariant::String) {
        css = res.toString();
    } else if (res.type() == QVariant::ByteArray) {
        // #### detect @charset
        css = QString::fromUtf8(res.toByteArray());
    }
    if (!css.isEmpty()) {
        QCss::Parser parser(css);
        QCss::StyleSheet sheet;
        parser.parse(&sheet); //, Qt::CaseInsensitive);
        externalStyleSheets.append(ExternalStyleSheet(href, sheet));
        resolveStyleSheetImports(sheet);
    }
#endif
}

bool TextHtmlParser::nodeIsChildOf(int i, TextHTMLElements id) const
{
    while (i) {
        if (at(i).id == id)
            return true;
        i = at(i).parent;
    }
    return false;
}

}
