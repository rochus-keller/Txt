/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Copyright 2013-2017 Rochus Keller <mailto:me@rochus-keller.info>
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

#include "TextHtmlImporter.h"
#include "LinkRendererInterface.h"
#include "TextCursor.h"
#include "ImageGlyph.h"
#include <QtGui/QTextTable>
#include <QtGui/QTextList>
#include <QtCore/QUrl>
#include <QVarLengthArray>
#include <QtDebug>
using namespace Txt;

// 1:1 QTextHtmlImporter von Qt 5.2.1 aus qtextdocumentfragment.cpp

static QTextListFormat::Style nextListStyle(QTextListFormat::Style style)
{
    if (style == QTextListFormat::ListDisc)
        return QTextListFormat::ListCircle;
    else if (style == QTextListFormat::ListCircle)
        return QTextListFormat::ListSquare;
    return style;
}

TextHtmlImporter::TextHtmlImporter(QTextDocument *_doc, const QString &_html, ImportMode mode, const QTextDocument *resourceProvider)
	: indent(0), compressNextWhitespace(PreserveWhiteSpace), doc(_doc), importMode(mode),d_lri(0)
{
    cursor = QTextCursor(doc);
    wsm = TextHtmlParserNode::WhiteSpaceNormal;

    QString html = _html;
    const int startFragmentPos = html.indexOf(QLatin1String("<!--StartFragment-->"));
    if (startFragmentPos != -1) {
        QString qt3RichTextHeader(QLatin1String("<meta name=\"qrichtext\" content=\"1\" />"));

        // Hack for Qt3
        const bool hasQtRichtextMetaTag = html.contains(qt3RichTextHeader);

        const int endFragmentPos = html.indexOf(QLatin1String("<!--EndFragment-->"));
        if (startFragmentPos < endFragmentPos)
            html = html.mid(startFragmentPos, endFragmentPos - startFragmentPos);
        else
            html = html.mid(startFragmentPos);

        if (hasQtRichtextMetaTag)
            html.prepend(qt3RichTextHeader);
    }

    parse(html, resourceProvider ? resourceProvider : doc);
	// dumpHtml();
//	for( int i = 0; i < count(); i++ )
//	{
//		qDebug() << "Nr." << i << at(i).tag << at(i).parent << at(i).text;
//	}
}

void TextHtmlImporter::import()
{
    cursor.beginEditBlock();
    hasBlock = true;
    forceBlockMerging = false;
    compressNextWhitespace = RemoveWhiteSpace;
    blockTagClosed = false;
	for( currentNodeIdx = 0; currentNodeIdx < count(); ++currentNodeIdx )
	{
		currentNode = &at(currentNodeIdx);
        wsm = textEditMode ? TextHtmlParserNode::WhiteSpacePreWrap : currentNode->wsm;

        /*
         * process each node in three stages:
         * 1) check if the hierarchy changed and we therefore passed the
         *    equivalent of a closing tag -> we may need to finish off
         *    some structures like tables
         *
         * 2) check if the current node is a special node like a
		 *    <table>, <ul> or <img> tag that requires special processing
         *
         * 3) if the node should result in a QTextBlock create one and
         *    finally insert text that may be attached to the node
         */

        /* emit 'closing' table blocks or adjust current indent level
         * if we
         *  1) are beyond the first node
         *  2) the current node not being a child of the previous node
         *      means there was a tag closing in the input html
         */
		if( currentNodeIdx > 0 && ( currentNode->parent != currentNodeIdx - 1 ) )
		{
            blockTagClosed = closeTag();
            // visually collapse subsequent block tags, but if the element after the closed block tag
            // is for example an inline element (!isBlock) we have to make sure we start a new paragraph by setting
            // hasBlock to false.
            if (blockTagClosed
                && !currentNode->isBlock()
                && currentNode->id != Html_unknown)
            {
                hasBlock = false;
			} else if ( hasBlock )
			{
                // when collapsing subsequent block tags we need to clear the block format
                QTextBlockFormat blockFormat = currentNode->blockFormat;
                blockFormat.setIndent(indent);

                QTextBlockFormat oldFormat = cursor.blockFormat();
				if (oldFormat.hasProperty(QTextFormat::PageBreakPolicy))
				{
                    QTextFormat::PageBreakFlags pageBreak = oldFormat.pageBreakPolicy();
                    if (pageBreak == QTextFormat::PageBreak_AlwaysAfter)
                        /* We remove an empty paragrah that requested a page break after.
                           moving that request to the next paragraph means we also need to make
                            that a pagebreak before to keep the same visual appearance.
                        */
                        pageBreak = QTextFormat::PageBreak_AlwaysBefore;
                    blockFormat.setPageBreakPolicy(pageBreak);
                }

                cursor.setBlockFormat(blockFormat);
            }
        }

		if( currentNode->displayMode == TextHtmlElement::DisplayNone )
		{
			if( currentNode->id == Html_title )
                doc->setMetaInformation(QTextDocument::DocumentTitle, currentNode->text);
            // ignore explicitly 'invisible' elements
            continue;
        }

		if( processSpecialNodes() == ContinueWithNextNode )
            continue;

		if( d_lri != 0 && currentNode->id == Html_a && currentNode->charFormat.anchorHref().startsWith(
				QLatin1String(Styles::s_linkSchema), Qt::CaseInsensitive ) )
		{
			const QByteArray link = QByteArray::fromBase64( currentNode->charFormat.anchorHref().mid(
																::strlen( Styles::s_linkSchema ) ).toAscii() );
			TextCursor cur(cursor);
			if( !d_lri->renderLink( cur, link ) )
				cur.insertUrl( d_lri->renderHref( link ), true, currentNode->text ); // falls extern wandle in xoid
			const int oldcur = currentNodeIdx;
			while( ++currentNodeIdx < count() && at(currentNodeIdx).parent >= oldcur )
				; // Fix 13.3.16, vorher hier currentNodeIdx++ // qDebug() << oldcur << currentNodeIdx << at(currentNodeIdx).parent;
			currentNodeIdx--; // eins zurück wegen for-incrementer
			cursor = cur.qt();
			compressNextWhitespace = CollapseWhiteSpace;
			continue;
		}

        // make sure there's a block for 'Blah' after <ul><li>foo</ul>Blah
		if( blockTagClosed
            && !hasBlock
            && !currentNode->isBlock()
            && !currentNode->text.isEmpty() && !currentNode->hasOnlyWhitespace()
			&& currentNode->displayMode == TextHtmlElement::DisplayInline )
		{

            QTextBlockFormat block = currentNode->blockFormat;
            block.setIndent(indent);

            appendBlock(block, currentNode->charFormat);

            hasBlock = true;
        }

		if( currentNode->isBlock() )
		{
            TextHtmlImporter::ProcessNodeResult result = processBlockNode();
			if (result == ContinueWithNextNode)
			{
                continue;
			} else if (result == ContinueWithNextSibling)
			{
                currentNodeIdx += currentNode->children.size();
                continue;
            }
        }

		if (currentNode->charFormat.isAnchor() && !currentNode->charFormat.anchorName().isEmpty())
		{
            namedAnchors.append(currentNode->charFormat.anchorName());
        }

		if( appendNodeText() )
            hasBlock = false; // if we actually appended text then we don't
                              // have an empty block anymore
    }

	cursor.endEditBlock();
}

void TextHtmlImporter::setBlockFormat(const QTextBlockFormat &format)
{
	cursor.setBlockFormat(format);
}

bool TextHtmlImporter::appendNodeText()
{
    const int initialCursorPosition = cursor.position();
    QTextCharFormat format = currentNode->charFormat;

    if(wsm == TextHtmlParserNode::WhiteSpacePre || wsm == TextHtmlParserNode::WhiteSpacePreWrap)
        compressNextWhitespace = PreserveWhiteSpace;

    QString text = currentNode->text;

    QString textToInsert;
    textToInsert.reserve(text.size());

	for( int i = 0; i < text.length(); ++i )
	{
        QChar ch = text.at(i);

		if( ch.isSpace() && ch != QChar::Nbsp && ch != QChar::ParagraphSeparator )
		{

            if (compressNextWhitespace == CollapseWhiteSpace)
                compressNextWhitespace = RemoveWhiteSpace; // allow this one, and remove the ones coming next.
            else if(compressNextWhitespace == RemoveWhiteSpace)
                continue;

			if( wsm == TextHtmlParserNode::WhiteSpacePre || textEditMode )
			{
				if( ch == QLatin1Char('\n') )
				{
					if( textEditMode )
                        continue;
				}else if( ch == QLatin1Char('\r') )
				{
                    continue;
                }
			}else if( wsm != TextHtmlParserNode::WhiteSpacePreWrap )
			{
                compressNextWhitespace = RemoveWhiteSpace;
                if (wsm == TextHtmlParserNode::WhiteSpaceNoWrap)
                    ch = QChar::Nbsp;
                else
                    ch = QLatin1Char(' ');
            }
		}else
		{
            compressNextWhitespace = PreserveWhiteSpace;
        }

		if( ch == QLatin1Char('\n') || ch == QChar::ParagraphSeparator)
		{
			if ( !textToInsert.isEmpty() )
			{
                cursor.insertText(textToInsert, format);
                textToInsert.clear();
            }

            QTextBlockFormat fmt = cursor.blockFormat();

			if (fmt.hasProperty(QTextFormat::BlockBottomMargin))
			{
                QTextBlockFormat tmp = fmt;
                tmp.clearProperty(QTextFormat::BlockBottomMargin);
                cursor.setBlockFormat(tmp);
            }

            fmt.clearProperty(QTextFormat::BlockTopMargin);
            appendBlock(fmt, cursor.charFormat());
		} else
		{
			if (!namedAnchors.isEmpty())
			{
				if (!textToInsert.isEmpty())
				{
                    cursor.insertText(textToInsert, format);
                    textToInsert.clear();
                }

                format.setAnchor(true);
                format.setAnchorNames(namedAnchors);
                cursor.insertText(ch, format);
                namedAnchors.clear();
                format.clearProperty(QTextFormat::IsAnchor);
                format.clearProperty(QTextFormat::AnchorName);
			} else
			{
                textToInsert += ch;
            }
        }
    }

	if( !textToInsert.isEmpty() )
	{
        cursor.insertText(textToInsert, format);
    }

    return cursor.position() != initialCursorPosition;
}

TextHtmlImporter::ProcessNodeResult TextHtmlImporter::processSpecialNodes()
{
	switch (currentNode->id)
	{
        case Html_body:
			if (currentNode->charFormat.background().style() != Qt::NoBrush)
			{
                QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
                fmt.setBackground(currentNode->charFormat.background());
                doc->rootFrame()->setFrameFormat(fmt);
                const_cast<TextHtmlParserNode *>(currentNode)->charFormat.clearProperty(QTextFormat::BackgroundBrush);
            }
            compressNextWhitespace = RemoveWhiteSpace;
            break;

        case Html_ol:
        case Html_ul: {
            QTextListFormat::Style style = currentNode->listStyle;

			if (currentNode->id == Html_ul && !currentNode->hasOwnListStyle && currentNode->parent)
			{
                const TextHtmlParserNode *n = &at(currentNode->parent);
                while (n) {
                    if (n->id == Html_ul) {
                        style = nextListStyle(currentNode->listStyle);
                    }
                    if (n->parent)
                        n = &at(n->parent);
                    else
                        n = 0;
                }
            }

            QTextListFormat listFmt;
            listFmt.setStyle(style);
            // Nicht unterstützt in Qt 4.4.3
//            if (!currentNode->textListNumberPrefix.isNull())
//                listFmt.setNumberPrefix(currentNode->textListNumberPrefix);
//            if (!currentNode->textListNumberSuffix.isNull())
//                listFmt.setNumberSuffix(currentNode->textListNumberSuffix);

            ++indent;
            if (currentNode->hasCssListIndent)
                listFmt.setIndent(currentNode->cssListIndent);
            else
                listFmt.setIndent(indent);

            List l;
            l.format = listFmt;
            l.listNode = currentNodeIdx;
            lists.append(l);
            compressNextWhitespace = RemoveWhiteSpace;

            // broken html: <ul>Text here<li>Foo
            const QString simpl = currentNode->text.simplified();
            if (simpl.isEmpty() || simpl.at(0).isSpace())
                return ContinueWithNextNode;
            break;
        }

		case Html_table:
		{
            Table t = scanTable(currentNodeIdx);
            tables.append(t);
            hasBlock = false;
            compressNextWhitespace = RemoveWhiteSpace;
            return ContinueWithNextNode;
        }

        case Html_tr:
            return ContinueWithNextNode;

		case Html_img:
		{
			if( currentNode->imageName.startsWith("data:") )
			{
				ImageGlyph::regHandler( cursor.block().document() );
				QTextCharFormat f = currentNode->charFormat;
				f.setObjectType( ImageGlyph::Type );
				f.setProperty( Styles::PropImage, QVariant::fromValue( ImageGlyph::parseDataSrc(currentNode->imageName) ) );
				if( currentNode->imageWidth != -1 && currentNode->imageHeight != -1 )
					f.setProperty( Styles::PropSize, QVariant::fromValue(
									   QSize( currentNode->imageWidth, currentNode->imageHeight ) ) );
				cursor.insertText( QString(QChar::ObjectReplacementCharacter), f );
			}else
			{
				QTextImageFormat fmt;
				fmt.setName(currentNode->imageName);

				fmt.merge(currentNode->charFormat);

				if (currentNode->imageWidth != -1)
					fmt.setWidth(currentNode->imageWidth);
				if (currentNode->imageHeight != -1)
					fmt.setHeight(currentNode->imageHeight);

				cursor.insertImage(fmt, QTextFrameFormat::Position(currentNode->cssFloat));
			}

            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(currentNode->charFormat);
            cursor.movePosition(QTextCursor::Right);
            compressNextWhitespace = CollapseWhiteSpace;

            hasBlock = false;
            return ContinueWithNextNode;
        }

		case Html_hr:
		{
            QTextBlockFormat blockFormat = currentNode->blockFormat;
            blockFormat.setTopMargin(topMargin(currentNodeIdx));
            blockFormat.setBottomMargin(bottomMargin(currentNodeIdx));
            blockFormat.setProperty(QTextFormat::BlockTrailingHorizontalRulerWidth, currentNode->width);
            if (hasBlock && importMode == ImportToDocument)
                cursor.mergeBlockFormat(blockFormat);
            else
                appendBlock(blockFormat);
            hasBlock = false;
            compressNextWhitespace = RemoveWhiteSpace;
            return ContinueWithNextNode;
        }

        default: break;
    }
    return ContinueWithCurrentNode;
}

// returns true if a block tag was closed
bool TextHtmlImporter::closeTag()
{
    const TextHtmlParserNode *closedNode = &at(currentNodeIdx - 1);
    const int endDepth = depth(currentNodeIdx) - 1;
    int depth = this->depth(currentNodeIdx - 1);
    bool blockTagClosed = false;

    while (depth > endDepth) {
        Table *t = 0;
        if (!tables.isEmpty())
            t = &tables.last();

        switch (closedNode->id) {
            case Html_tr:
                if (t && !t->isTextFrame) {
                    ++t->currentRow;

                    // for broken html with rowspans but missing tr tags
                    while (!t->currentCell.atEnd() && t->currentCell.row < t->currentRow)
                        ++t->currentCell;
                }

                blockTagClosed = true;
                break;

            case Html_table:
                if (!t)
                    break;
                indent = t->lastIndent;

                tables.resize(tables.size() - 1);
                t = 0;

                if (tables.isEmpty()) {
                    cursor = doc->rootFrame()->lastCursorPosition();
                } else {
                    t = &tables.last();
                    if (t->isTextFrame)
                        cursor = t->frame->lastCursorPosition();
                    else if (!t->currentCell.atEnd())
                        cursor = t->currentCell.cell().lastCursorPosition();
                }

                // we don't need an extra block after tables, so we don't
                // claim to have closed one for the creation of a new one
                // in import()
                blockTagClosed = false;
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_th:
            case Html_td:
                if (t && !t->isTextFrame)
                    ++t->currentCell;
                blockTagClosed = true;
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_ol:
            case Html_ul:
                if (lists.isEmpty())
                    break;
                lists.resize(lists.size() - 1);
                --indent;
                blockTagClosed = true;
                break;

            case Html_br:
                compressNextWhitespace = RemoveWhiteSpace;
                break;

            case Html_div:
                if (closedNode->children.isEmpty())
                    break;
                // fall through
            default:
                if (closedNode->isBlock())
                    blockTagClosed = true;
                break;
        }

        closedNode = &at(closedNode->parent);
        --depth;
    }

    return blockTagClosed;
}

TextHtmlImporter::Table TextHtmlImporter::scanTable(int tableNodeIdx)
{
    Table table;
    table.columns = 0;

    QVector<QTextLength> columnWidths;

    int tableHeaderRowCount = 0;
    QVector<int> rowNodes;
    rowNodes.reserve(at(tableNodeIdx).children.count());
    foreach (int row, at(tableNodeIdx).children)
        switch (at(row).id) {
            case Html_tr:
                rowNodes += row;
                break;
            case Html_thead:
            case Html_tbody:
            case Html_tfoot:
                foreach (int potentialRow, at(row).children)
                    if (at(potentialRow).id == Html_tr) {
                        rowNodes += potentialRow;
                        if (at(row).id == Html_thead)
                            ++tableHeaderRowCount;
                    }
                break;
            default: break;
        }

    QVector<RowColSpanInfo> rowColSpans;
    QVector<RowColSpanInfo> rowColSpanForColumn;

    int effectiveRow = 0;
    foreach (int row, rowNodes) {
        int colsInRow = 0;

        foreach (int cell, at(row).children)
            if (at(cell).isTableCell()) {
                // skip all columns with spans from previous rows
                while (colsInRow < rowColSpanForColumn.size()) {
                    const RowColSpanInfo &spanInfo = rowColSpanForColumn[colsInRow];

                    if (spanInfo.row + spanInfo.rowSpan > effectiveRow) {
                        Q_ASSERT(spanInfo.col == colsInRow);
                        colsInRow += spanInfo.colSpan;
                    } else
                        break;
                }

                const TextHtmlParserNode &c = at(cell);
                const int currentColumn = colsInRow;
                colsInRow += c.tableCellColSpan;

                RowColSpanInfo spanInfo;
                spanInfo.row = effectiveRow;
                spanInfo.col = currentColumn;
                spanInfo.colSpan = c.tableCellColSpan;
                spanInfo.rowSpan = c.tableCellRowSpan;
                if (spanInfo.colSpan > 1 || spanInfo.rowSpan > 1)
                    rowColSpans.append(spanInfo);

                columnWidths.resize(qMax(columnWidths.count(), colsInRow));
                rowColSpanForColumn.resize(columnWidths.size());
                for (int i = currentColumn; i < currentColumn + c.tableCellColSpan; ++i) {
                    if (columnWidths.at(i).type() == QTextLength::VariableLength) {
                        QTextLength w = c.width;
                        if (c.tableCellColSpan > 1 && w.type() != QTextLength::VariableLength)
                            w = QTextLength(w.type(), w.value(100.) / c.tableCellColSpan);
                        columnWidths[i] = w;
                    }
                    rowColSpanForColumn[i] = spanInfo;
                }
            }

        table.columns = qMax(table.columns, colsInRow);

        ++effectiveRow;
    }
    table.rows = effectiveRow;

    table.lastIndent = indent;
    indent = 0;

    if (table.rows == 0 || table.columns == 0)
        return table;

    QTextFrameFormat fmt;
    const TextHtmlParserNode &node = at(tableNodeIdx);

    if (!node.isTextFrame) {
        QTextTableFormat tableFmt;
        tableFmt.setCellSpacing(node.tableCellSpacing);
        tableFmt.setCellPadding(node.tableCellPadding);
        if (node.blockFormat.hasProperty(QTextFormat::BlockAlignment))
            tableFmt.setAlignment(node.blockFormat.alignment());
        tableFmt.setColumns(table.columns);
        tableFmt.setColumnWidthConstraints(columnWidths);
        tableFmt.setHeaderRowCount(tableHeaderRowCount);
        fmt = tableFmt;
    }

    fmt.setTopMargin(topMargin(tableNodeIdx));
    fmt.setBottomMargin(bottomMargin(tableNodeIdx));
    fmt.setLeftMargin(leftMargin(tableNodeIdx)
                      + table.lastIndent * 40 // ##### not a good emulation
                      );
    fmt.setRightMargin(rightMargin(tableNodeIdx));

    // compatibility
    if (qFuzzyCompare(fmt.leftMargin(), fmt.rightMargin())
        && qFuzzyCompare(fmt.leftMargin(), fmt.topMargin())
        && qFuzzyCompare(fmt.leftMargin(), fmt.bottomMargin()))
        fmt.setProperty(QTextFormat::FrameMargin, fmt.leftMargin());

    fmt.setBorderStyle(node.borderStyle);
    fmt.setBorderBrush(node.borderBrush);
    fmt.setBorder(node.tableBorder);
    fmt.setWidth(node.width);
    fmt.setHeight(node.height);
    if (node.blockFormat.hasProperty(QTextFormat::PageBreakPolicy))
        fmt.setPageBreakPolicy(node.blockFormat.pageBreakPolicy());

    if (node.blockFormat.hasProperty(QTextFormat::LayoutDirection))
        fmt.setLayoutDirection(node.blockFormat.layoutDirection());
    if (node.charFormat.background().style() != Qt::NoBrush)
        fmt.setBackground(node.charFormat.background());
    fmt.setPosition(QTextFrameFormat::Position(node.cssFloat));

    if (node.isTextFrame) {
        if (node.isRootFrame) {
            table.frame = cursor.currentFrame();
            table.frame->setFrameFormat(fmt);
        } else
            table.frame = cursor.insertFrame(fmt);

        table.isTextFrame = true;
    } else {
        const int oldPos = cursor.position();
        QTextTable *textTable = cursor.insertTable(table.rows, table.columns, fmt.toTableFormat());
        table.frame = textTable;

        for (int i = 0; i < rowColSpans.count(); ++i) {
            const RowColSpanInfo &nfo = rowColSpans.at(i);
            textTable->mergeCells(nfo.row, nfo.col, nfo.rowSpan, nfo.colSpan);
        }

        table.currentCell = TableCellIterator(textTable);
        cursor.setPosition(oldPos); // restore for caption support which needs to be inserted right before the table
    }
    return table;
}

TextHtmlImporter::ProcessNodeResult TextHtmlImporter::processBlockNode()
{
    QTextBlockFormat block;
    QTextCharFormat charFmt;
    bool modifiedBlockFormat = true;
    bool modifiedCharFormat = true;

    if (currentNode->isTableCell() && !tables.isEmpty()) {
        Table &t = tables.last();
        if (!t.isTextFrame && !t.currentCell.atEnd()) {
            QTextTableCell cell = t.currentCell.cell();
            if (cell.isValid()) {
                QTextTableCellFormat fmt = cell.format().toTableCellFormat();
                if (topPadding(currentNodeIdx) >= 0)
                    fmt.setTopPadding(topPadding(currentNodeIdx));
                if (bottomPadding(currentNodeIdx) >= 0)
                    fmt.setBottomPadding(bottomPadding(currentNodeIdx));
                if (leftPadding(currentNodeIdx) >= 0)
                    fmt.setLeftPadding(leftPadding(currentNodeIdx));
                if (rightPadding(currentNodeIdx) >= 0)
                    fmt.setRightPadding(rightPadding(currentNodeIdx));
                cell.setFormat(fmt);

                cursor.setPosition(cell.firstPosition());
            }
        }
        hasBlock = true;
        compressNextWhitespace = RemoveWhiteSpace;

        if (currentNode->charFormat.background().style() != Qt::NoBrush) {
            charFmt.setBackground(currentNode->charFormat.background());
            cursor.mergeBlockCharFormat(charFmt);
        }
    }

    if (hasBlock) {
        block = cursor.blockFormat();
        charFmt = cursor.blockCharFormat();
        modifiedBlockFormat = false;
        modifiedCharFormat = false;
    }

    // collapse
    {
        qreal tm = qreal(topMargin(currentNodeIdx));
        if (tm > block.topMargin()) {
            block.setTopMargin(tm);
            modifiedBlockFormat = true;
        }
    }

    int bottomMargin = this->bottomMargin(currentNodeIdx);

    // for list items we may want to collapse with the bottom margin of the
    // list.
    const TextHtmlParserNode *parentNode = currentNode->parent ? &at(currentNode->parent) : 0;
    if ((currentNode->id == Html_li || currentNode->id == Html_dt || currentNode->id == Html_dd)
        && parentNode
        && (parentNode->isListStart() || parentNode->id == Html_dl)
        && (parentNode->children.last() == currentNodeIdx)) {
        bottomMargin = qMax(bottomMargin, this->bottomMargin(currentNode->parent));
    }

    if (block.bottomMargin() != bottomMargin) {
        block.setBottomMargin(bottomMargin);
        modifiedBlockFormat = true;
    }

    {
        const qreal lm = leftMargin(currentNodeIdx);
        const qreal rm = rightMargin(currentNodeIdx);

        if (block.leftMargin() != lm) {
            block.setLeftMargin(lm);
            modifiedBlockFormat = true;
        }
        if (block.rightMargin() != rm) {
            block.setRightMargin(rm);
            modifiedBlockFormat = true;
        }
    }

    if (currentNode->id != Html_li
        && indent != 0
        && (lists.isEmpty()
            || !hasBlock
            || !lists.last().list
            || lists.last().list->itemNumber(cursor.block()) == -1
           )
       ) {
        block.setIndent(indent);
        modifiedBlockFormat = true;
    }

    if (currentNode->blockFormat.propertyCount() > 0) {
        modifiedBlockFormat = true;
        block.merge(currentNode->blockFormat);
    }

    if (currentNode->charFormat.propertyCount() > 0) {
        modifiedCharFormat = true;
        charFmt.merge(currentNode->charFormat);
    }

    // ####################
    //                block.setFloatPosition(node->cssFloat);

    if (wsm == TextHtmlParserNode::WhiteSpacePre) {
        block.setNonBreakableLines(true);
        modifiedBlockFormat = true;
    }

    if (currentNode->charFormat.background().style() != Qt::NoBrush && !currentNode->isTableCell()) {
        block.setBackground(currentNode->charFormat.background());
        modifiedBlockFormat = true;
    }

    if (hasBlock && (!currentNode->isEmptyParagraph || forceBlockMerging)) {
        if (modifiedBlockFormat)
            cursor.setBlockFormat(block);
        if (modifiedCharFormat)
            cursor.setBlockCharFormat(charFmt);
    } else {
        if (currentNodeIdx == 1 && cursor.position() == 0 && currentNode->isEmptyParagraph) {
            cursor.setBlockFormat(block);
            cursor.setBlockCharFormat(charFmt);
        } else {
            appendBlock(block, charFmt);
        }
    }

    if (currentNode->userState != -1)
        cursor.block().setUserState(currentNode->userState);

    if (currentNode->id == Html_li && !lists.isEmpty()) {
        List &l = lists.last();
        if (l.list) {
            l.list->add(cursor.block());
        } else {
            l.list = cursor.createList(l.format);
            const qreal listTopMargin = topMargin(l.listNode);
            if (listTopMargin > block.topMargin()) {
                block.setTopMargin(listTopMargin);
                cursor.mergeBlockFormat(block);
            }
        }
        if (hasBlock) {
            QTextBlockFormat fmt;
            fmt.setIndent(0);
            cursor.mergeBlockFormat(fmt);
        }
    }

    forceBlockMerging = false;
    if (currentNode->id == Html_body || currentNode->id == Html_html)
        forceBlockMerging = true;

    if (currentNode->isEmptyParagraph) {
        hasBlock = false;
        return ContinueWithNextSibling;
    }

    hasBlock = true;
    blockTagClosed = false;
    return ContinueWithCurrentNode;
}

void TextHtmlImporter::appendBlock(const QTextBlockFormat &format, QTextCharFormat charFmt)
{
    if (!namedAnchors.isEmpty()) {
        charFmt.setAnchor(true);
        charFmt.setAnchorNames(namedAnchors);
        namedAnchors.clear();
    }

    cursor.insertBlock(format, charFmt);

    if (wsm != TextHtmlParserNode::WhiteSpacePre && wsm != TextHtmlParserNode::WhiteSpacePreWrap)
        compressNextWhitespace = RemoveWhiteSpace;
}
