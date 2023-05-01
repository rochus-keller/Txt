#ifndef TEXTHTMLPARSER_P_H
#define TEXTHTMLPARSER_P_H

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

#include "QtCore/qvector.h"
#include "QtGui/qbrush.h"
#include "QtGui/qcolor.h"
#include "QtGui/qfont.h"
#include "QtGui/qtextdocument.h"
#include "QtGui/qtextcursor.h"

namespace Txt
{
	enum TextHTMLElements {
	    Html_unknown = -1,
	    Html_qt = 0,
	    Html_body,
	
	    Html_a,
	    Html_em,
	    Html_i,
	    Html_big,
	    Html_small,
	    Html_strong,
	    Html_b,
	    Html_cite,
	    Html_address,
	    Html_var,
	    Html_dfn,
	
	    Html_h1,
	    Html_h2,
	    Html_h3,
	    Html_h4,
	    Html_h5,
	    Html_h6,
	    Html_p,
	    Html_center,
	
	    Html_font,
	
	    Html_ul,
	    Html_ol,
	    Html_li,
	
	    Html_code,
	    Html_tt,
	    Html_kbd,
	    Html_samp,
	
	    Html_img,
	    Html_br,
	    Html_hr,
	
	    Html_sub,
	    Html_sup,
	
	    Html_pre,
	    Html_blockquote,
	    Html_head,
	    Html_div,
	    Html_span,
	    Html_dl,
	    Html_dt,
	    Html_dd,
	    Html_u,
	    Html_s,
	    Html_nobr,
	
	    // tables
	    Html_table,
	    Html_tr,
	    Html_td,
	    Html_th,
	    Html_thead,
	    Html_tbody,
	    Html_tfoot,
	    Html_caption,
	
	    // misc...
	    Html_html,
	    Html_style,
	    Html_title,
	    Html_meta,
	    Html_link,
	    Html_script,
	
	    Html_NumElements
	};
	
	struct TextHtmlElement
	{
	    const char *name;
	    TextHTMLElements id;
	    enum DisplayMode { DisplayBlock, DisplayInline, DisplayTable, DisplayNone } displayMode;
	};
	
	class TextHtmlParser;
	
	struct TextHtmlParserNode
	{
		enum WhiteSpaceMode
		{
	        WhiteSpaceNormal,
	        WhiteSpacePre,
	        WhiteSpaceNoWrap,
	        WhiteSpacePreWrap,
	        WhiteSpaceModeUndefined = -1
	    };
	
	    TextHtmlParserNode();
	    QString tag;
	    QString text;
	    QStringList attributes;
		int parent; // Index of the parent node
	    QVector<int> children;
	    TextHTMLElements id;
	    QTextCharFormat charFormat;
	    QTextBlockFormat blockFormat;
	    uint cssFloat : 2;
	    uint hasOwnListStyle : 1;
	    uint hasCssListIndent : 1;
	    uint isEmptyParagraph : 1;
	    uint isTextFrame : 1;
	    uint isRootFrame : 1;
	    uint displayMode : 3; // TextHtmlElement::DisplayMode
	    uint hasHref : 1;
	    QTextListFormat::Style listStyle;
	    QString textListNumberPrefix;
	    QString textListNumberSuffix;
	    QString imageName;
	    qreal imageWidth;
	    qreal imageHeight;
	    QTextLength width;
	    QTextLength height;
	    qreal tableBorder;
	    int tableCellRowSpan;
	    int tableCellColSpan;
	    qreal tableCellSpacing;
	    qreal tableCellPadding;
	    QBrush borderBrush;
	    QTextFrameFormat::BorderStyle borderStyle;
	    int userState;
	
	    int cssListIndent;
	
	    WhiteSpaceMode wsm;
	
	    inline bool isListStart() const
	    { return id == Html_ol || id == Html_ul; }
	    inline bool isTableCell() const
	    { return id == Html_td || id == Html_th; }
	    inline bool isBlock() const
	    { return displayMode == TextHtmlElement::DisplayBlock; }
	
	    inline bool isNotSelfNesting() const
	    { return id == Html_p || id == Html_li; }
	
	    inline bool allowedInContext(int parentId) const
	    {
	        switch (id) {
	            case Html_dd:
	            case Html_dt: return (parentId == Html_dl);
	            case Html_tr: return (parentId == Html_table
	                                  || parentId == Html_thead
	                                  || parentId == Html_tbody
	                                  || parentId == Html_tfoot
	                                 );
	            case Html_th:
	            case Html_td: return (parentId == Html_tr);
	            case Html_thead:
	            case Html_tbody:
	            case Html_tfoot: return (parentId == Html_table);
	            case Html_caption: return (parentId == Html_table);
	            case Html_body: return parentId != Html_head;
	            default: break;
	        }
	        return true;
	    }
	
	    inline bool mayNotHaveChildren() const
		{ return id == Html_img || id == Html_hr || id == Html_br || id == Html_meta || id == Html_link; }
	
	    void initializeProperties(const TextHtmlParserNode *parent, const TextHtmlParser *parser);
	
	    inline int uncollapsedMargin(int mar) const { return margin[mar]; }
	
	    bool isNestedList(const TextHtmlParser *parser) const;
	
	    void parseStyleAttribute(const QString &value, const QTextDocument *resourceProvider);
		
	    void applyBackgroundImage(const QString &url, const QTextDocument *resourceProvider);
	
	    bool hasOnlyWhitespace() const;
	
	    int margin[4];
	    int padding[4];
	};
	
	
	class TextHtmlParser
	{
	public:
	    enum Margin {
	        MarginTop,
	        MarginRight,
	        MarginBottom,
	        MarginLeft
	    };
	
	    inline const TextHtmlParserNode &at(int i) const { return nodes.at(i); }
	    inline TextHtmlParserNode &operator[](int i) { return nodes[i]; }
	    inline int count() const { return nodes.count(); }
	    inline int last() const { return nodes.count()-1; }
	    int depth(int i) const;
	    int topMargin(int i) const;
	    int bottomMargin(int i) const;
	    inline int leftMargin(int i) const { return margin(i, MarginLeft); }
	    inline int rightMargin(int i) const { return margin(i, MarginRight); }
	
	    inline int topPadding(int i) const { return at(i).padding[MarginTop]; }
	    inline int bottomPadding(int i) const { return at(i).padding[MarginBottom]; }
	    inline int leftPadding(int i) const { return at(i).padding[MarginLeft]; }
	    inline int rightPadding(int i) const { return at(i).padding[MarginRight]; }
	
	    void dumpHtml();
	
	    void parse(const QString &text, const QTextDocument *resourceProvider);
	
	    static int lookupElement(const QString &element);
	protected:
	    TextHtmlParserNode *newNode(int parent);
	    QVector<TextHtmlParserNode> nodes;
	    QString txt;
	    int pos, len;
	
	    bool textEditMode;
	
	    void parse();
	    void parseTag();
	    void parseCloseTag();
	    void parseExclamationTag();
	    QString parseEntity();
	    QString parseWord();
	    TextHtmlParserNode *resolveParent();
	    void resolveNode();
	    QStringList parseAttributes();
	    void applyAttributes(const QStringList &attributes);
	    void eatSpace();
	    inline bool hasPrefix(QChar c, int lookahead = 0) const
	        {return pos + lookahead < len && txt.at(pos) == c; }
	    int margin(int i, int mar) const;
	
	    bool nodeIsChildOf(int i, TextHTMLElements id) const;
	
	
	    void importStyleSheet(const QString &href);
		
	    const QTextDocument *resourceProvider;
	};
}
Q_DECLARE_TYPEINFO(Txt::TextHtmlParserNode, Q_MOVABLE_TYPE);


#endif // TEXTHTMLPARSER_P_H
